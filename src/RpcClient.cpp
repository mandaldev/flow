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
#include "RpcClient.hpp"
#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "StreamMessageSocket.hpp"
#include "Timer.hpp"
#include <Com/BytesView.hpp>
#include <Com/log.hpp>
#include <cstdint>
#include <future>
#include <memory>
#include <system_error>

using namespace std::chrono_literals;

namespace Flow {

RpcClient::RpcClient(Flower &flower)
: _flower(flower)
{
}

void RpcClient::init(Config const &conf, std::function<void(std::error_code)> const &on_init_failed)
{
	if (!_flower.is_flow_thread()) {
		_flower.add_task(bind_weak([this, conf, on_init_failed]()
		{
			init(conf, on_init_failed);
		}));
		return;
	}

	_conf = conf;
	_on_init_failed = on_init_failed;
	_init_ts = Timer::get_time(Timer::Boottime);
	_sock = std::make_shared<ClientSocket>(_flower, *this);
	auto ec = _sock->setup(_conf.server_addr);
	if (ec) {
		_sock.reset();
		if (_on_init_failed) {
			_on_init_failed(ec);
		}
		return;
	}

	_timer = std::make_shared<Timer>(_flower, Timer::Boottime);
	ec = _timer->set(1000ms, 1000ms);
	if (ec) {
		LOG_ERROR << "timer failed: " << ec;
	}
	_timer->on_timer(bind_weak([this](size_t)
	{
		check_connection_timeout();
		check_request_timeout();
	}));
}

void RpcClient::request(RpcRequest &&req, std::function<void(RpcResponse const&)> const &done)
{
	req.version(rpc_version_code());
	req.timeout((uint32_t)_conf.response_timeout.count());
	auto shared_req = std::make_shared<Request>();
	shared_req->req = std::move(req);
	shared_req->emit_ts = Timer::get_time(Timer::Boottime);
	shared_req->done = done;
	_flower.add_task(bind_weak([this, shared_req]()
	{
		shared_req->req.unique_id(new_unique_id());
		if (_connected && _sock) {
			if (!_sock->request(*shared_req)) {
				// sending request failed
				return;
			}
		}
		_requests.insert({shared_req->req.unique_id(), std::move(*shared_req)});
	}));
}

RpcResponse RpcClient::request(RpcRequest &&req)
{
	std::promise<RpcResponse> res;
	request(std::move(req), [&res](RpcResponse const &r)
	{
		res.set_value(r);
	});
	auto fut = res.get_future();
	return fut.get();
}

bool RpcClient::is_connected() const
{
	return _connected;
}

void RpcClient::flush_requests()
{
	check_request_timeout();
	if (!_sock || !_connected) {
		return;
	}
	for (auto it = _requests.begin(); it != _requests.end(); ) {
		auto &[uid, req] = *it;
		if (req.is_sent()) {
			continue;;
		}
		// sending request failed
		if (_sock->request(req)) {
			it++;
		} else {
			it = _requests.erase(it);
		}
	}
}

uint64_t RpcClient::new_unique_id()
{
	assert(_flower.is_flow_thread());
	_unique_id += 1;
	return _unique_id ? _unique_id : new_unique_id();
}

void RpcClient::check_request_timeout()
{
	auto now = Timer::get_time(Timer::Boottime);
	for (auto it = _requests.begin(); it != _requests.end(); ) {
		auto erase = false;
		auto &[uid, req] = *it;
		if (req.is_sent()) {
			// check response timeout
			if (now - req.sent_ts > _conf.response_timeout) {
				if (req.done) {
					RpcResponse res;
					res.unique_id(uid);
					res.status(RpcStatusCode::ResponseTimeout);
					req.done(res);
				}
				LOG_WARN << "response timeout: " << uid;
				erase = true;
			}
		} else if (now - req.emit_ts > _conf.request_timeout) {
			// check request timeout
			if (req.done) {
				RpcResponse res;
				res.unique_id(uid);
				res.status(RpcStatusCode::RequestTimeout);
				req.done(res);
			}
			LOG_WARN << "request timeout: " << uid;
			erase = true;
		}
		if (erase) {
			it = _requests.erase(it);
		} else {
			++it;
		}
	}
}

void RpcClient::check_connection_timeout()
{
	if (!_sock) {
		// failed to create socket, shutdown client
		[[maybe_unused]]
		auto ec = _timer->set();
		return;
	}
	if (_connected) {
		return;
	}
	auto now = Timer::get_time(Timer::Boottime);
	if (now - _init_ts > _conf.connection_timeout) {
		if (_on_init_failed) {
			_on_init_failed(std::make_error_code(std::errc::timed_out));
		}
		[[maybe_unused]]
		auto ec = _timer->set();
	}
}

void RpcClient::on_response(Com::BytesView message)
{
	RpcResponse res;
	if (!res.deserialize_from_mab(message)) {
		LOG_ERROR << "failed to deserialize response, size : " << message.size();
		return;
	}

	auto it = _requests.find(res.unique_id());
	if (it == _requests.end()) {
		LOG_WARN << "received a no-owner response: " << res.unique_id();
		return;
	}
	auto done = it->second.done;
	_requests.erase(it);

	if (done) {
		done(res);
	}
}

RpcClient::ClientSocket::ClientSocket(Flower &flower, RpcClient &owner)
: StreamMessageSocket(flower)
, _owner(owner)
{
}

std::error_code RpcClient::ClientSocket::setup(std::variant<IPv4Addr, std::string> const &addr)
{
	_addr = addr;
	if (std::holds_alternative<IPv4Addr>(addr)) {
		auto &ipv4 = std::get<IPv4Addr>(addr);
		auto ec = create_tcp_socket();
		if (ec) {
			LOG_ERROR << "failed to create tcp client socket";
			return ec;
		}
		ec = connect_ipv4(ipv4);
		if (ec) {
			LOG_ERROR << "failed to connect to '" << ipv4.tostr() << "': " << ec.message();
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
		LOG_ERROR << "failed to create local client socket: " << ec.message();
		return ec;
	}
	std::string abs_addr = local_addr;
	if (!is_abstract_local_addr(local_addr)) {
		abs_addr = build_abstract_local_addr(local_addr);
	}
	ec = connect_local(abs_addr);
	if (ec) {
		LOG_ERROR << "failed to connect to '" << format_local_addr(abs_addr) << "': " << ec.message();
	}
	return ec;
}

bool RpcClient::ClientSocket::request(Request &req)
{
	std::vector<Com::Bytes> msg(2);
	if (req.req.serialize_to_mab_except_args(msg[0])) {
		req.sent_ts = Timer::get_time(Timer::Boottime);
		msg[1] = std::move(req.req.args());
		send_message(std::move(msg));
		return true;
	} else {
		LOG_ERROR << "failed to serialize request: " << req.req.unique_id();
		if (req.done) {
			req.done({});
		}
		return false;
	}
}

void RpcClient::ClientSocket::on_close()
{
	LOG_INFO << "Client socket closed: '" << format_addr(_addr) << "'";
	_owner._sock.reset();
	_owner._connected = false;
	// TODO
}

void RpcClient::ClientSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Client socket has an error: '" << format_addr(_addr) << "': " << err->message();
	}
}

void RpcClient::ClientSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != ErrType::NoError) {
		LOG_ERROR << "Client socket error: '" << format_addr(_addr) << "': " << err->message();
	} else {
		LOG_INFO << "Client socket flowing: '" << format_addr(_addr) << "'";
	}
}

void RpcClient::ClientSocket::on_connect(std::error_code ec)
{
	if (ec) {
		LOG_ERROR << "Client socket connection error: '" << format_addr(_addr) << "': " << ec.message();
		return;
	}
	_owner._connected = true;
	_owner.flush_requests();
}

void RpcClient::ClientSocket::on_receive_message(Com::BytesView msg)
{
	_owner.on_response(msg);
}

}
