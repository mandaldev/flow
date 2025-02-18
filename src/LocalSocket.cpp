// flow - a network io lib
// Copyright (C) 2025  Mandal Dev
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
#include "LocalSocket.hpp"
#include "StreamSocket.hpp"
#include <sys/socket.h>

namespace Flow {

LocalSocket::LocalSocket(Flower &flower)
: StreamSocket(flower)
{
}

std::error_code LocalSocket::create_local_socket()
{
	return create_stream_socket(AF_LOCAL, 0);
}

std::error_code LocalSocket::listen_on(std::string_view addr, int backlog)
{
	auto ec = bind(addr);
	if (!ec) {
		return listen(backlog);
	}
	return ec;
}

}
