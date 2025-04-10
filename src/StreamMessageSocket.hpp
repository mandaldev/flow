// Flow - a network io lib
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
#ifndef FLOW_SRC_STREAMPACKETSOCKET_HPP
#define FLOW_SRC_STREAMPACKETSOCKET_HPP
#include "StreamSocket.hpp"
#include <Com/BytesView.hpp>
#include <atomic>
#include <cstdint>
#include <functional>

namespace Flow {

/// socket for streamed message
class StreamMessageSocket
: public StreamSocket
{
public:
	explicit StreamMessageSocket(Flower &flower);

	/// send message to peer
	void send_message(Com::Bytes &&msg);

	void send_message(std::vector<Com::Bytes> &&msg);

	/// add callback for receiving message
	void on_receive_message(std::function<void(Com::BytesView)> const &receiver);

	/// set max message size (in bytes), if meet one in or out message which has larger
	/// number of bytes than `size`, will be considerd an error, and connection will be
	/// closed.
	void max_message_size(uint32_t size);

	/// current max message size, default as 5MB (5 * (1 << 20))
	uint32_t max_message_size() const;

protected:
	/// override this to receive message instead of using callback
	virtual void on_receive_message(Com::BytesView msg);

private:

	void on_receive_bytes(Com::Bytes &&bytes) override;

	bool try_complete_message_size(Com::BytesView &bytes);

	void try_complete_message(Com::BytesView &bytes);

	std::map<std::string, std::function<void(Com::BytesView msg)>> _receivers;

	std::atomic_uint32_t _max_msg_size {5u * (1u << 20)};
	uint32_t _msg_size = 0;
	// received part of uint32_t or message buffer
	Com::Bytes _buffer;

	std::function<void(Com::BytesView)> _receiver;
};

}
#endif
