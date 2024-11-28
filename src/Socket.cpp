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
#include "Socket.hpp"
#include <cstring>

extern "C" {
#include <unistd.h>
#include <arpa/inet.h>
}

namespace Flow {

std::error_code get_error(int e)
{
	return std::make_error_code(std::errc(e));
}

Socket::Socket(Socket && o)
: _fd(o._fd)
{
	o._fd = -1;
}

Socket::~Socket()
{
	if (is_created()) {
		close();
	}
}

std::error_code Socket::Attach(Socket &o)
{
	if (is_created()) {
		if (auto ec = close()) {
			return ec;
		}
	}
	_fd = o._fd;
	o._fd = -1;
	return {};
}

std::error_code Socket::create(int domain, int type, int protocol)
{
	_fd = socket(domain, type, protocol);
	return get_error(_fd >= 0 ? 0 : errno);
}

std::error_code Socket::close()
{
	if (::close(_fd) == 0) {
		_fd = -1;
		return {};
	}
	return get_error();
}

std::error_code Socket::bind(sockaddr const &addr, socklen_t addr_len) const
{
	return get_error(::bind(_fd, &addr, addr_len) == 0 ? 0 : errno);
}

std::error_code Socket::listen(int backlog)
{
	return get_error(::listen(_fd, backlog) == 0 ? 0 : errno);
}

std::variant<std::error_code, Socket> Socket::accept(sockaddr &addr, socklen_t &addr_len, int flags) const
{
	Socket s;
	s._fd = ::accept4(_fd, &addr, &addr_len, flags);
	if (s.is_created()) {
		return s;
	}
	return get_error();
}

std::variant<std::error_code, Socket> Socket::accept(int flags) const
{
	Socket s;
	s._fd = ::accept4(_fd, nullptr, nullptr, flags);
	if (s.is_created()) {
		return s;
	}
	return get_error();
}

std::error_code Socket::connect(sockaddr const &addr, socklen_t addr_len)
{
	return get_error(::connect(_fd, &addr, addr_len) == 0 ? 0 : errno);
}

bool Socket::is_created() const
{
	return _fd >= 0;
}

std::error_code Socket::read(std::string &buf, size_t count) const
{
	buf.resize(count);
	auto n = ::read(_fd, buf.data(), count);
	if (n >= 0) {
		buf.resize(n);
		return {};
	}
	return get_error();
}

std::error_code Socket::write(std::string_view const &bytes, size_t &writen_count) const
{
	writen_count = 0;
	auto n = ::write(_fd, bytes.data(), bytes.size());
	if (n >= 0) {
		writen_count = n;
		return {};
	}
	return get_error();
}

std::error_code Socket::get_name(sockaddr &addr, socklen_t &addr_len) const
{
	if (0 == getsockname(_fd, &addr, &addr_len)) {
		return get_error();
	}
	return {};
}

std::error_code Socket::get_peer_name(sockaddr &addr, socklen_t &addr_len) const
{
	if (0 == getpeername(_fd, &addr, &addr_len)) {
		return get_error();
	}
	return {};
}

}
