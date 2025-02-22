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
#include <Com/log.hpp>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <sys/socket.h>

extern "C" {
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
}

namespace Flow {

std::error_code get_error_code(int e)
{
	if (e) {
		return std::make_error_code(std::errc(e));
	}
	return std::error_code();
}

bool is_abstract_local_addr(std::string_view name)
{
	return name.size() > 1 && name[0] == 0;
}

bool is_unamed_local_addr(std::string_view name)
{
	return name.empty();
}

std::string build_abstract_local_addr(std::string_view name)
{
	return '\0' + std::string(name.begin(), name.end());
}

static std::error_code convert_addr(sockaddr_in const &in, IPv4Addr &out)
{
	char buf[INET_ADDRSTRLEN] = "";
	if (!inet_ntop(in.sin_family, &in.sin_addr, buf, INET_ADDRSTRLEN)) {
		return get_error_code();
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
		return std::make_error_code(std::errc::invalid_argument);
	case 1:
		return {};
	default:
		return get_error_code();
	}
}

static std::error_code convert_addr(sockaddr_un const &in, size_t in_size, std::string &out)
{
	const auto min_size = offsetof(sockaddr_un, sun_path);
	if (in_size < min_size) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	if (in_size == min_size) {
		// unnamed
		out.clear();
		return {};
	}
	if (in.sun_path[0] == '\0') {
		// abstract
		out = std::string(in.sun_path, in_size - min_size);
		return {};
	}
	out = in.sun_path;
	return {};
}

/// convert named or abstract local socket addr
static std::error_code convert_addr(std::string_view in, sockaddr_un &out)
{
	const auto max_size = sizeof(out.sun_path);
	if (in.empty() || in.size() > max_size) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	out.sun_family = AF_LOCAL;
	if (is_abstract_local_addr(in)) {
		// abstract
		std::memcpy(out.sun_path, in.data(), in.size());
		return {};
	}
	if (in.size() == max_size) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	std::memcpy(out.sun_path, in.data(), in.size());
	out.sun_path[in.size()] = 0;
	return {};
}

Socket::Socket(Socket && o)
: _fd(o._fd)
, _domain(o._domain)
, _type(o._type)
{
	o._fd = -1;
}

Socket::~Socket()
{
	if (is_created()) {
		auto ec = Socket::close();
		if (ec) {
			LOG_ERROR << "failed to close socket[" << _fd << "]: " << ec.message();
		}
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
	_domain = o._domain;
	_type = o._type;
	o._fd = -1;
	o._domain = 0;
	o._type = 0;
	return {};
}

int Socket::socket_fd() const
{
	return _fd;
}

std::error_code Socket::create(int domain, int type, int protocol)
{
	_fd = socket(domain, type, protocol);
	_domain = domain;
	_type = type;
	return get_error_code(_fd >= 0 ? 0 : errno);
}

std::error_code Socket::close()
{
	if (::close(_fd) == 0) {
		_fd = -1;
		return {};
	}
	return get_error_code();
}

std::error_code Socket::bind(sockaddr const &addr, socklen_t addr_len) const
{
	return get_error_code(::bind(_fd, &addr, addr_len) == 0 ? 0 : errno);
}

std::error_code Socket::listen(int backlog)
{
	return get_error_code(::listen(_fd, backlog) == 0 ? 0 : errno);
}

std::variant<std::error_code, Socket> Socket::accept(sockaddr &addr, socklen_t &addr_len, int flags) const
{
	Socket s;
	s._fd = ::accept4(_fd, &addr, &addr_len, flags);
	if (s.is_created()) {
		return s;
	}
	return get_error_code();
}

std::variant<std::error_code, Socket> Socket::accept(int flags) const
{
	Socket s;
	s._fd = ::accept4(_fd, nullptr, nullptr, flags);
	s._domain = _domain;
	s._type = _type;
	if (s.is_created()) {
		return s;
	}
	return get_error_code();
}

std::error_code Socket::connect(sockaddr const &addr, socklen_t addr_len)
{
	return get_error_code(::connect(_fd, &addr, addr_len) == 0 ? 0 : errno);
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
	buf.clear();
	return get_error_code();
}

std::error_code Socket::write(std::string_view const &bytes, size_t &writen_count) const
{
	writen_count = 0;
	auto n = ::write(_fd, bytes.data(), bytes.size());
	if (n >= 0) {
		writen_count = n;
		return {};
	}
	return get_error_code();
}

std::error_code Socket::get_name(sockaddr &addr, socklen_t &addr_len) const
{
	if (0 != getsockname(_fd, &addr, &addr_len)) {
		return get_error_code();
	}
	return {};
}

std::error_code Socket::get_peer_name(sockaddr &addr, socklen_t &addr_len) const
{
	if (0 != getpeername(_fd, &addr, &addr_len)) {
		return get_error_code();
	}
	return {};
}

std::error_code Socket::bind(IPv4Addr const &addr) const
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return Socket::bind((sockaddr&)out, sizeof(out));
}

std::variant<std::error_code, Socket> Socket::accept(IPv4Addr &addr, int flags) const
{
	sockaddr_in in{};
	socklen_t len = sizeof(in);
	auto var = Socket::accept((sockaddr&)in, len, flags);
	if (std::holds_alternative<std::error_code>(var)) {
		return std::get<std::error_code>(var);
	}
	if (len != sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	auto ec = convert_addr(in, addr);
	if (ec) {
		return ec;
	}
	return std::get<Socket>(std::move(var));
}

std::error_code Socket::connect_ipv4(IPv4Addr const &addr)
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return connect((sockaddr&)out, sizeof(out));
}

std::error_code Socket::get_name(IPv4Addr &addr) const
{
	sockaddr_in in{};
	socklen_t len = sizeof(in);
	auto ec = Socket::get_name((sockaddr&)in, len);
	if (ec) {
		return ec;
	}
	if (len != sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	return convert_addr(in, addr);
}

std::error_code Socket::get_peer_name(IPv4Addr &addr) const
{
	sockaddr_in in{};
	socklen_t len = sizeof(in);
	auto ec = Socket::get_peer_name((sockaddr&)in, len);
	if (ec) {
		return ec;
	}
	if (len != sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	return convert_addr(in, addr);
}

std::variant<std::error_code, Socket> Socket::accept(std::string &addr, int flags) const
{
	sockaddr_un in{};
	in.sun_family = AF_LOCAL;
	socklen_t len = sizeof(in);
	auto var = Socket::accept((sockaddr&)in, len, flags);
	if (std::holds_alternative<std::error_code>(var)) {
		return std::get<std::error_code>(var);
	}
	if (len > sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	auto ec = convert_addr(in, len, addr);
	if (ec) {
		return ec;
	}
	return std::get<Socket>(std::move(var));
}

std::error_code Socket::connect_local(std::string_view addr)
{
	sockaddr_un out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return connect((sockaddr&)out, sizeof(out));
}

std::error_code Socket::get_name(std::string &addr) const
{
	sockaddr_un in{};
	in.sun_family = AF_LOCAL;
	socklen_t len = sizeof(in);
	auto ec = Socket::get_name((sockaddr&)in, len);
	if (ec) {
		return ec;
	}
	if (len != sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	return convert_addr(in, len, addr);
}

std::error_code Socket::get_peer_name(std::string &addr) const
{
	sockaddr_un in{};
	in.sun_family = AF_LOCAL;
	socklen_t len = sizeof(in);
	auto ec = Socket::get_peer_name((sockaddr&)in, len);
	if (ec) {
		return ec;
	}
	if (len > sizeof(in)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	return convert_addr(in, len, addr);
}

std::error_code Socket::bind(std::string_view addr) const
{
	sockaddr_un out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return Socket::bind((sockaddr&)out, sizeof(out));
}

std::error_code Socket::get_option(int level, int name, void *value, socklen_t &value_size)
{
	if (0 != getsockopt(_fd, level, name, value, &value_size)) {
		return get_error_code();
	}
	return {};
}

std::error_code Socket::set_option(int level, int name, void const *value, socklen_t value_size)
{
	if (0 != setsockopt(_fd, level, name, value, value_size)) {
		return get_error_code();
	}
	return {};
}

std::pair<std::error_code, std::error_code> Socket::get_error()
{
	int err = 0;
	auto ec = get_option(SOL_SOCKET, SO_ERROR, err);
	return {ec, get_error_code(err)};
}

int Socket::domain() const
{
	return _domain;
}

int Socket::type() const
{
	return _type;
}

}
