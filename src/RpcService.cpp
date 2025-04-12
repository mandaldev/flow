// flow - a network io lib
// Copyright (C) 2025  Mandal Dev
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "RpcService.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "StreamMessageSocket.hpp"
#include "Timer.hpp"
#include <Com/BytesView.hpp>
#include <Com/log.hpp>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace Flow {

RpcService::RpcService(Flower &flower)
: _flower(flower)
{
}

RpcService::~RpcService()
{
	if (_tp) {
		_tp->stop(true);
	}
}

void RpcService::init(Config const &conf, std::function<void(size_t addr_index, std::error_code)> const &on_init_failed)
{
	if (!_flower.is_flow_thread()) {
		_flower.add_task(bind_weak([this, conf, on_init_failed]()
		{
			init(conf, on_init_failed);
		}));
		return;
	}

	_conf = conf;
	size_t addr_index = 0;
	for (auto const &ipv4 : _conf._ip_addrs) {
		addr_index += 1;
		auto listen_sock = std::make_shared<ListenSocket>(_flower, *this);
		auto ec = listen_sock->setup(ipv4, _conf.backlog);
		if (ec) {
			if (on_init_failed) {
				on_init_failed(addr_index, ec);
			}
		} else {
			_listen_socks.insert(listen_sock);
		}
	}
	for (auto const &name : _conf._local_addrs) {
		addr_index += 1;
		auto listen_sock = std::make_shared<ListenSocket>(_flower, *this);
		auto ec = listen_sock->setup(name, _conf.backlog);
		if (ec) {
			if (on_init_failed) {
				on_init_failed(addr_index, ec);
			}
		} else {
			_listen_socks.insert(listen_sock);
		}
	}

	if (_conf.threads <= 1) {
		return;
	} else {
		if (_tp) {
			_tp->stop(false);
		}
		_tp = std::make_shared<Com::ThreadPool>(_conf.threads);
	}

	_timer = std::make_shared<Timer>(_flower, Timer::Boottime);
	auto ec = _timer->set(1000ms, 1000ms);
	if (ec) {
		LOG_ERROR << "timer failed: " << ec;
	}
	_timer->on_timer(bind_weak([this](size_t)
	{
		check_response_timeout();
	}));
}

void RpcService::check_response_timeout()
{
	for (auto &server : _servers) {
		server->check_response_timeout();
	}
}

RpcService::ListenSocket::ListenSocket(Flower &flower, RpcService &owner)
: StreamSocket(flower)
, _owner(owner)
{
}

std::error_code RpcService::ListenSocket::setup(std::variant<IPv4Addr, std::string> const &addr, int backlog)
{
	_addr = addr;
	if (std::holds_alternative<IPv4Addr>(addr)) {
		auto &ipv4 = std::get<IPv4Addr>(addr);
		auto ec = create_tcp_socket();
		if (ec) {
			LOG_ERROR << "failed to create tcp server socket";
			return ec;
		}
		ec = listen_on(ipv4, backlog);
		if (ec) {
			LOG_ERROR << "failed to listen at '" << ipv4.tostr() << "': " << ec.message();
			return ec;
		}
		return ec;
	}
	auto &local_addr = std::get<std::string>(addr);
	if (is_unamed_local_addr(local_addr)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	auto ec = create_local_socket();
	if (ec) {
		LOG_ERROR << "failed to create local server socket: " << ec.message();
		return ec;
	}
	std::string abs_addr = local_addr;
	if (!is_abstract_local_addr(local_addr)) {
		abs_addr = build_abstract_local_addr(local_addr);
	}
	ec = listen_on(abs_addr, backlog);
	if (ec) {
		LOG_ERROR << "failed to listen at '" << format_local_addr(abs_addr) << "': " << ec.message();
	}
	return ec;
}

void RpcService::ListenSocket::on_accept(Flow::Socket client_socket)
{
	auto server = std::make_shared<ServerSocket>(_flower, _owner, _addr);
	auto ec = server->Attach(client_socket);
	if (ec) {
		LOG_ERROR << "failed to attach new server socket: " << ec.message();
		return;
	}
	_owner._servers.insert(server);
	server->setup_client_flow();
}

void RpcService::ListenSocket::on_close()
{
	LOG_INFO << "Listen socket closed: '" << format_addr(_addr) << "'";
	_owner._listen_socks.erase(std::dynamic_pointer_cast<ListenSocket>(shared_from_this()));
}

void RpcService::ListenSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Listen socket has an error: '" << format_addr(_addr) << "': " << err->message();
	}
}

void RpcService::ListenSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Flow listen socket error: '" << format_addr(_addr) << "': " << err->message();
	} else {
		LOG_INFO << "Listen socket flowing: '" << format_addr(_addr) << "'";
	}
}

RpcService::ServerSocket::ServerSocket(Flower &flower, RpcService &owner, std::variant<IPv4Addr, std::string> const &addr)
: StreamMessageSocket(flower)
, _owner(owner)
, _addr(addr)
{
}

void RpcService::ServerSocket::on_close()
{
	_owner._servers.erase(std::dynamic_pointer_cast<ServerSocket>(shared_from_this()));
}

void RpcService::ServerSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Server socket '" << format_addr(_addr) << "' has an error: " << err->message();
	}
}

void RpcService::ServerSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Flow server socket error: '" << format_addr(_addr) << "': " << err->message();
	} else {
		LOG_INFO << "Server socket flowing: '" << format_addr(_addr) << "'";
	}
}

void RpcService::ServerSocket::on_receive_message(Com::BytesView msg)
{
	Request req;
	req.req = std::make_shared<RpcRequest>();
	if (!req.req->deserialize_from_mab(msg)) {
		LOG_ERROR << "failed to deserialize request, size : " << msg.size();
		return;
	}
	if (_owner._conf.threads <= 1) {
		dispatch(*req.req);
		return;
	}

	req.arrive_ts = Timer::get_time(Timer::Boottime);
	req.task_id = _owner._tp->add_task(bind_weak([this, req = req.req]()
	{
		dispatch(*req);
	}));
	_requests.insert({req.req->unique_id(), std::move(req)});
}

void RpcService::ServerSocket::check_response_timeout()
{
	auto now = Timer::get_time(Timer::Boottime);
	for (auto it = _requests.begin(); it != _requests.end(); ) {
		if (now - it->second.arrive_ts > std::chrono::milliseconds(it->second.req->timeout())) {
			LOG_WARN << "response timeout: " << it->first;
			_owner._tp->cancel_task(it->second.task_id);
			it = _requests.erase(it);
		} else {
			++it;
		}
	}
}

void RpcService::ServerSocket::dispatch(RpcRequest const &req)
{
	auto res = std::make_shared<RpcResponse>();
	_owner.on_request(req, *res);

	res->unique_id(req.unique_id());
	_flower.add_task(bind_weak([this, res]
	{
		if (_owner._conf.threads > 1) {
			auto it = _requests.find(res->unique_id());
			if (it == _requests.end()) {
				LOG_WARN << "response is too late: " << res->unique_id();
				return;
			}
			_requests.erase(it);
		}

		std::vector<Com::Bytes> msg(2);
		if (res->serialize_to_mab_except_returns(msg[0])) {
			msg[1] = std::move(res->returns());
			send_message(std::move(msg));
		} else {
			LOG_ERROR << "failed to serialize response: " << res->unique_id();
		}
	}));
}

}
