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
#include <string>
#include <system_error>
#include <string_view>
#include <variant>
#include <Com/raii.hpp>

extern "C" {
#include <sys/socket.h>
}

namespace Flow {

std::error_code get_error(int e = errno);

/// socket interface - wrapper of system socket api
class Socket
: public virtual Com::Unique
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

	int socket_fd() const;

	/// create new socket no matter of if this `Socket` is created or not
	std::error_code create(int domain, int type, int protocol);

	/// always use this api to close a socket, although the destructor will make sure of it
	virtual std::error_code close();

	std::error_code bind(sockaddr const &addr, socklen_t addr_len) const;

	/// The behavior of the `backlog` argument on TCP sockets changed with Linux 2.2.
	/// Now it specifies the queue length for completely established sockets waiting
	/// to be accepted, instead of the number of incomplete connection requests.
	virtual std::error_code listen(int backlog);

	/// as accept4
	std::variant<std::error_code, Socket> accept(
		sockaddr &addr,
		/// in & out
		socklen_t &addr_len,
		int flags
	) const;

	/// if you dont need know the address
	std::variant<std::error_code, Socket> accept(int flags) const;

	virtual std::error_code connect(sockaddr const &addr, socklen_t addr_len);

	bool is_created() const;

	/// read max `count` bytes to buf
	std::error_code read(std::string &buf, size_t count) const;

	std::error_code write(std::string_view const &bytes, size_t &writen_count) const;

	std::error_code get_name(sockaddr &addr, socklen_t &addr_len) const;

	std::error_code get_peer_name(sockaddr &addr, socklen_t &addr_len) const;

private:
	int _fd = -1;
};

}
#endif
