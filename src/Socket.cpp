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

static inline std::error_code get_error(int e = errno)
{
	return std::make_error_code(std::errc(e));
}

static std::error_code convert_addr(sockaddr_in &in, IPv4Addr &out)
{
	char buf[INET_ADDRSTRLEN] = "";
	if (!inet_ntop(in.sin_family, &in.sin_addr, buf, INET_ADDRSTRLEN)) {
		return get_error();
	}
	out.ip = buf;
	out.port = ntohs(in.sin_port);
	return {};
}

static std::error_code convert_addr(IPv4Addr const &in, sockaddr_in &out)
{
	out.sin_family = AF_INET;
	out.sin_port = htons(in.port);
	switch (inet_pton(AF_INET, in.ip.c_str(), &out.sin_addr)) {
	case 0:
		return {};
	case 1:
		return std::make_error_code(std::errc::invalid_argument);
	default:
		return get_error();
	}
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

std::error_code Socket::bind(IPv4Addr const &addr) const
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return bind((sockaddr&)out, sizeof(out));
}

std::error_code Socket::listen(int backlog) const
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

std::variant<std::error_code, Socket> Socket::accept(IPv4Addr &addr, int flags) const
{
	sockaddr_in in{};
	socklen_t len = sizeof(sockaddr_in);
	auto var = accept((sockaddr&)in, len, flags);
	if (std::holds_alternative<std::error_code>(var)) {
		return var;
	}
	convert_addr(in, addr);
	return var;
}

std::error_code Socket::connect(sockaddr const &addr, socklen_t addr_len) const
{
	return get_error(::connect(_fd, &addr, addr_len) == 0 ? 0 : errno);
}

std::error_code Socket::connect(IPv4Addr const &addr) const
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return connect((sockaddr&)out, sizeof(out));
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

}
