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
#ifndef _FLOW_SRC_STREAMSOCKET_HPP_
#define _FLOW_SRC_STREAMSOCKET_HPP_
#include "AsyncSocket.hpp"
#include <list>
#include <mutex>
#include <string_view>

namespace Flow {

/// async stream socket
class StreamSocket
: public AsyncSocket
{
public:
	StreamSocket(Flower &flower);

	/// crete async stream socket
	std::error_code create_stream_socket(int domain, int protocol = 0);

	void send_bytes(std::string_view bytes);

protected:
	/// override this to receive data
	virtual void on_receive_bytes(std::string_view bytes) {}

	void on_write() override;

	void on_read() override;

private:
	std::mutex _mtx_data;
	std::list<std::string> _data;
	size_t _bytes_sent = 0;
};

}
#endif
