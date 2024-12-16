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
#ifndef _FLOW_SRC_IPV4SOCKET_HPP_
#define _FLOW_SRC_IPV4SOCKET_HPP_
#include "Socket.hpp"
#include <cstdint>

namespace Flow {

struct IPv4Addr
{
	std::string ip;
	uint16_t port = 0;
};

class IPv4Socket
: public virtual Socket
{
public:
	/// AF_INET, for IPv4 domain
	static const int domain;

	IPv4Socket() = default;

	IPv4Socket(Socket &&o);

	std::error_code bind(IPv4Addr const &addr) const;

	/// ipv4 socket accept.
	/// if accept succeed but address converting fails, error is returned
	/// and client socket is closed
	std::variant<std::error_code, IPv4Socket> accept(IPv4Addr &addr, int flags) const;

	std::error_code connect_ipv4(IPv4Addr const &addr);

	std::error_code get_name(IPv4Addr &addr) const;

	std::error_code get_peer_name(IPv4Addr &addr) const;

};

}
#endif
