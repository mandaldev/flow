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
#ifndef _FLOW_SRC_TCPSOCKET_HPP_
#define _FLOW_SRC_TCPSOCKET_HPP_
#include "StreamSocket.hpp"

namespace Flow {

class TCPSocket
: public virtual StreamSocket
{
public:
	TCPSocket(Flower &flower);

	/// create tcp socket
	std::error_code create();

	/// setup server socket
	std::error_code listen_on(IPv4Addr const &addr, int backlog);

protected:

private:
};

}
#endif
