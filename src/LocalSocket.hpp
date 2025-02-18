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
#ifndef _FLOW_SRC_LOCALSOCKET_HPP_
#define _FLOW_SRC_LOCALSOCKET_HPP_
#include "StreamSocket.hpp"

namespace Flow {

class LocalSocket
: public StreamSocket
{
public:
	LocalSocket(Flower &flower);

	/// create unix domain socket
	std::error_code create_local_socket();

	/// setup server socket
	std::error_code listen_on(std::string_view addr, int backlog);

private:
};

}
#endif
