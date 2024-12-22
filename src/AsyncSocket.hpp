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
#ifndef _FLOW_SRC_ASYNCSOCKET_HPP_
#define _FLOW_SRC_ASYNCSOCKET_HPP_
#include "Socket.hpp"
#include "Flower.hpp"

namespace Flow {

class AsyncSocket
: public virtual Socket
, public virtual std::enable_shared_from_this<AsyncSocket>
{
public:
	AsyncSocket(Flower &flower);

	/// crete async socket
	std::error_code create(int domain, int type, int protocol);

	/// server socket api, on_accept is called when new client arriving
	std::error_code listen(int backlog) override;

	/// client socket api, on_connect is called when connection established.
	/// once connected, this socket is added to Flower
	std::error_code connect(sockaddr const &addr, socklen_t addr_len) override;

	/// async operation, on_close will be called
	std::error_code close() override;

	/// when this is a client socket (from on_accept), call this to add to Flower
	void setup_client_flow();

protected:
	/// new client arriving
	/// `sock` is not added to flower yet
	virtual void on_accept(Socket sock) = 0;

	/// connection established
	virtual void on_connect() = 0;

	virtual void on_write() = 0;

	virtual void on_read() = 0;

	virtual void on_peer_close(bool only_read) = 0;

	virtual void on_close() = 0;

	virtual void on_error(std::shared_ptr<Error> err) = 0;

	Flower &_flower;

private:
	void do_accept();

	// bool _listening = false;
	bool _connecting = false;
};

}
#endif
