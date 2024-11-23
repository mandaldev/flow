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
#ifndef _FLOW_SRC_SOCKET_HPP_
#define _FLOW_SRC_SOCKET_HPP_
#include <memory>
#include <string>
#include <system_error>
#include <string_view>
#include <variant>
#include <Com/raii.hpp>

extern "C" {
#include <sys/socket.h>
}

namespace Flow {

struct IPv4Addr
{
	std::string ip;
	uint16_t port = 0;
};

/// socket interface - wrapper of system socket api
class Socket
: Com::Unique
{
public:
	Socket() = default;

	Socket(Socket const &) = delete;
	Socket &operator=(Socket const &) = delete;
	/// assigning need close itself first, and error can happen,
	/// so... use `Attach`
	Socket &operator=(Socket &&o) = delete;

	Socket(Socket &&o);

	/// close socket if it's created
	~Socket();

	/// attach other socket (like move-assign).
	/// if error happens (closing socket), `o` is untouched
	std::error_code Attach(Socket &o);

	/// create new socket no matter of if this `Socket` is created or not
	std::error_code create(int domain, int type, int protocol);

	std::error_code close();

	std::error_code bind(sockaddr const &addr, socklen_t addr_len) const;

	std::error_code bind(IPv4Addr const &addr) const;

	/// The behavior of the `backlog` argument on TCP sockets changed with Linux 2.2.
	/// Now it specifies the queue length for completely established sockets waiting
	/// to be accepted, instead of the number of incomplete connection requests.
	std::error_code listen(int backlog) const;

	/// as accept4
	std::variant<std::error_code, Socket> accept(
		sockaddr &addr,
		/// in & out
		socklen_t &addr_len,
		int flags
	) const;

	/// ipv4 socket accept.
	/// if accept succeed but address converting fails, error is returned
	/// and client socket is closed
	std::variant<std::error_code, Socket> accept(IPv4Addr &addr, int flags) const;

	std::error_code connect(sockaddr const &addr, socklen_t addr_len) const;

	std::error_code connect(IPv4Addr const &addr) const;

	bool is_created() const;

	/// read max `count` bytes to buf
	std::error_code read(std::string &buf, size_t count) const;

	std::error_code write(std::string_view const &bytes, size_t &writen_count) const;

	std::error_code get_name(sockaddr &addr, socklen_t &addr_len) const;

	std::error_code get_name(IPv4Addr &addr) const;

	std::error_code get_peer_name(sockaddr &addr, socklen_t &addr_len) const;

	std::error_code get_peer_name(IPv4Addr &addr) const;

private:
	int _fd = -1;
};

}
#endif
