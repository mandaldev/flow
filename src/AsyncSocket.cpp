// flow - a network io lib
// Copyright (C) 2024  Mandal Dev
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
#include "AsyncSocket.hpp"
#include <Com/log.hpp>

namespace Flow {

AsyncSocket::AsyncSocket(Flower &flower)
: _flower(flower)
{
}

std::error_code AsyncSocket::create(int domain, int type, int protocol)
{
	return Socket::create(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
}

std::error_code AsyncSocket::listen(int backlog)
{
	auto ec = Socket::listen(backlog);
	if (ec) {
		return ec;
	}

	// _listening = true;
	Flower::Action action;
	action.on_read = [this]()
	{
		do_accept();
	};
	action.on_error = [this](std::shared_ptr<Error> err)
	{
	};
	_flower.add_flow(socket_fd(), action, [this](std::shared_ptr<Error> err)
	{
		if (err && err->type() != ErrType::NoError) {
			LOG_INFO << err->message();
		}
	});
	return {};
}

std::error_code AsyncSocket::connect(sockaddr const &addr, socklen_t addr_len)
{
	auto ec = Socket::connect(addr, addr_len);
	if (ec) {
		if (ec.value() != EAGAIN && ec.value() != EINPROGRESS) {
			return ec;
		}
		_connecting = true;
	}
	setup_client_flow();
	return {};
}

std::error_code AsyncSocket::close()
{
	if (!is_created()) {
		return {};
	}
	_flower.remove_flow(socket_fd(), [this](std::shared_ptr<Error> err) {
		auto ec = Socket::close();
		if (ec) {
			// TODO
		}
		on_close();
	});
	return {};
}

void AsyncSocket::setup_client_flow()
{
	Flower::Action action;
	action.on_read = [this]() {
		on_read();
	};
	action.on_write = [this]() {
		if (_connecting) {
			// TODO
			on_connect();
		}
		on_write();
	};
	action.on_peer_close = [this](bool only_read) {
		on_peer_close(only_read);
	};
	action.on_error = [this](std::shared_ptr<Flow::Error> err)
	{
		on_error(err);
	};
	_flower.add_flow(socket_fd(), action, action.on_error);
}

void AsyncSocket::do_accept()
{
	auto res = Socket::accept(SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (std::holds_alternative<std::error_code>(res)) {
		auto err = std::get<std::error_code>(res);
		switch (err.value()) {
		case EAGAIN:
		// case EWOULDBLOCK:
			return;
		}
		LOG_ERROR << "error from 'accept': " << err.message();
		return;
	}
	auto &s = std::get<Socket>(res);
	on_accept(std::move(s));
}

}
