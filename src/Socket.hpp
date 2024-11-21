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
#include <functional>
#include <string>
#include <system_error>
#include <string_view>
#include <sys/socket.h>
#include <variant>

namespace Flow {

// struct IPv4Addr
// {
// 	std::string ip;
// 	uint16_t port = 0;
// };

/// socket interface - wrapper of system socket api
class Socket
{
public:
	/// create new socket no matter of if this `Socket` is created or not
	std::error_code create(int domain, int type, int protocol);

	std::error_code close();

	std::error_code bind(struct sockaddr const &addr, socklen_t addr_len) const;

	/// The behavior of the `backlog` argument on TCP sockets changed with Linux 2.2.
	/// Now it specifies the queue length for completely established sockets waiting
	/// to be accepted, instead of the number of incomplete connection requests.
	std::error_code listen(int backlog) const;

	/// as accept4
	std::variant<std::error_code, Socket> accept(
		struct sockaddr &addr,
		/// in & out
		socklen_t &addr_len,
		int flags
	) const;

	std::error_code connect(struct sockaddr const &addr, socklen_t addr_len) const;

	bool is_created() const;

	/// read max `count` bytes to buf
	std::error_code read(std::string &buf, size_t count) const;

	std::error_code write(std::string_view const &bytes, size_t &writen_count) const;

private:
	int _fd = -1;
};

}
#endif
