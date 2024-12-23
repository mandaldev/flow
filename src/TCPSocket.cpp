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
#include "TCPSocket.hpp"
#include "StreamSocket.hpp"

namespace Flow {

TCPSocket::TCPSocket(Flower &flower)
: StreamSocket(flower)
{
}

std::error_code TCPSocket::create_tcp_socket()
{
	return create_stream_socket(AF_INET, 0);
}

std::error_code TCPSocket::listen_on(IPv4Addr const &addr, int backlog)
{
	auto ec = bind(addr);
	if (!ec) {
		return listen(backlog);
	}
	return ec;
}

}
