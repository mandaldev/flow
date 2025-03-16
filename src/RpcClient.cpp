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
#include <Com/log.hpp>

namespace Flow {

RpcClient::RpcClient(Flower &flower)
: _flower(flower)
{
}

void RpcClient::init(Config const &conf)
{
	_conf = conf;
	_flower.add_task([this]()
	{
		_sock = std::make_shared<ClientSocket>(_flower, *this);
		auto ec = _sock->setup(_conf.server_addr);
		if (ec) {
			_sock.reset();
		}
		// TODO
	});
}


RpcClient::ClientSocket::ClientSocket(Flower &flower, RpcClient &owner)
: RpcSocket(flower)
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

void RpcClient::ClientSocket::on_close()
{
	LOG_INFO << "Client socket closed: '" << format_addr(_addr) << "'";
	_owner._sock.reset();
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
	_owner.on_connect(_owner._sock);
}

}
