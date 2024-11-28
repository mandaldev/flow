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
#include "IPv4Socket.hpp"

extern "C" {
#include <unistd.h>
#include <arpa/inet.h>
}

namespace Flow {

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

IPv4Socket::IPv4Socket(Socket &&o)
: Socket(std::move(o))
{
}

std::error_code IPv4Socket::bind(IPv4Addr const &addr) const
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return Socket::bind((sockaddr&)out, sizeof(out));
}

std::variant<std::error_code, IPv4Socket> IPv4Socket::accept(IPv4Addr &addr, int flags) const
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

std::error_code IPv4Socket::connect(IPv4Addr const &addr)
{
	sockaddr_in out{};
	if (auto ec = convert_addr(addr, out)) {
		return ec;
	}
	return Socket::connect((sockaddr&)out, sizeof(out));
}

std::error_code IPv4Socket::get_name(IPv4Addr &addr) const
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

std::error_code IPv4Socket::get_peer_name(IPv4Addr &addr) const
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


}
