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
#include <Com/log.hpp>

namespace Flow {

RpcService::RpcService(Flower &flower)
: _flower(flower)
{
}

void RpcService::init(Config const &conf)
{
	_conf = conf;
	_flower.add_task([this]()
	{
		for (auto const &ipv4 : _conf._ip_addrs) {
			auto listen_sock = std::make_shared<ListenSocket>(_flower, *this);
			auto ec = listen_sock->setup(ipv4, _conf.backlog);
			if (!ec) {
				_listen_socks.insert(listen_sock);
			}
		}
		for (auto const &name : _conf._local_addrs) {
			auto listen_sock = std::make_shared<ListenSocket>(_flower, *this);
			auto ec = listen_sock->setup(name, _conf.backlog);
			if (!ec) {
				_listen_socks.insert(listen_sock);
			}
		}
	});
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
	_owner.on_new_client(server);
	server->setup_client_flow();
}

void RpcService::ListenSocket::on_close()
{
	LOG_INFO << "Listen socket closed: '" << format_addr(_addr) << "'";
	_owner._listen_socks.erase(shared_from_this());
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
: RpcSocket(flower)
, _owner(owner)
, _addr(addr)
{
}

void RpcService::ServerSocket::on_close()
{
	_owner._servers.erase(shared_from_this());
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

}
