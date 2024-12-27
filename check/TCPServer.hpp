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
#ifndef _FLOW_CHECK_TCPSERVER_HPP_
#define _FLOW_CHECK_TCPSERVER_HPP_
#include "../src/TCPSocket.hpp"
#include <memory>
#include <set>

class TCPListenSocket
: public Flow::TCPSocket
{
public:
	TCPListenSocket(Flow::Flower &flower);

	void remove_client(std::shared_ptr<Flow::AsyncSocket> c);

protected:
	void on_accept(Flow::Socket client_socket) override;

	void on_close() override;

	void on_error(std::shared_ptr<Flow::Error> err) override;

	void on_flowing(std::shared_ptr<Flow::Error> err) override;

private:
	std::set<std::shared_ptr<Flow::AsyncSocket>> _clients;
};

#endif
