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
#ifndef FLOW_SRC_SOCKET_HPP_
#define FLOW_SRC_SOCKET_HPP_
#include <Com/BytesView.hpp>
#include <Com/raii.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

extern "C" {
#include <sys/socket.h>
}

namespace Flow {

std::error_code get_error_code(int e = errno);

struct IPv4Addr
{
	std::string ip;
	uint16_t port = 0;

	std::string tostr() const
	{
		return ip + ':' + std::to_string(port);
	}
};

bool is_abstract_local_addr(std::string_view name);

bool is_unamed_local_addr(std::string_view name);

std::string build_abstract_local_addr(std::string_view name);

std::string_view format_local_addr(std::string_view name);

std::string format_addr(std::variant<IPv4Addr, std::string> const &addr);

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
	std::error_code read(Com::Bytes &buf, size_t count) const;

	std::error_code write(Com::BytesView bytes, size_t &writen_count) const;

	std::error_code get_name(sockaddr &addr, socklen_t &addr_len) const;

	std::error_code get_peer_name(sockaddr &addr, socklen_t &addr_len) const;

	std::error_code bind(IPv4Addr const &addr) const;

	/// ipv4 socket accept.
	/// if accept succeed but address converting fails, error is returned
	/// and client socket is closed
	std::variant<std::error_code, Socket> accept(IPv4Addr &addr, int flags) const;

	std::error_code connect_ipv4(IPv4Addr const &addr);

	std::error_code get_name(IPv4Addr &addr) const;

	std::error_code get_peer_name(IPv4Addr &addr) const;


	/// local (unix) socket accept
	/// - empty string: unamed local socket
	/// - zero-byte leading string: abstract local socket
	/// - other: named local socket
	std::variant<std::error_code, Socket> accept(std::string &addr, int flags) const;

	std::error_code connect_local(std::string_view addr);

	std::error_code get_name(std::string &addr) const;

	std::error_code get_peer_name(std::string &addr) const;

	std::error_code bind(std::string_view addr) const;

	std::error_code get_option(int level, int name, void *value, socklen_t &value_size);

	std::error_code set_option(int level, int name, void const *value, socklen_t value_size);

	template <typename T>
	std::error_code get_option(int level, int name, T &value);

	template <typename T>
	std::error_code set_option(int level, int name, T const &value);

	/// get socket error (use `get_option`)
	/// since `get_option` can cause error itself, this api return two error code,
	/// one for `get_option`, second for socket error
	std::pair<std::error_code, std::error_code> get_error();

	/// domain passed to last call to `create`
	int domain() const;

	/// type passed to last call to `create`
	int type() const;

private:
	int _fd = -1;
	int _domain = 0;
	int _type = 0;
};

template <typename T>
std::error_code Socket::get_option(int level, int name, T &value)
{
	socklen_t size = sizeof(value);
	auto ec = get_option(level, name, &value, size);
	if (ec) {
		return ec;
	}
	if (size != sizeof(value)) {
		return std::make_error_code(std::errc::invalid_argument);
	}
	return {};
}

template <typename T>
std::error_code Socket::set_option(int level, int name, T const &value)
{
	socklen_t size = sizeof(value);
	return set_option(level, name, &value, size);
}

}
#endif
