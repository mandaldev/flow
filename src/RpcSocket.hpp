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
#ifndef FLOW_SRC_RPCSOCKET_HPP
#define FLOW_SRC_RPCSOCKET_HPP
#include "StreamSocket.hpp"
#include <Com/Mab/serialize.hpp>

namespace Flow {

enum class MsgType : int
{
	Heartbeat,
	Shakehand,
	UserMessage,
};

struct Head
{
	int version_code = 0;
	MsgType type = MsgType::Heartbeat;
	size_t msg_size = 0;
};

class RpcSocket
: public StreamSocket
{
public:
	explicit RpcSocket(Flower &flower);

	void send_encoded_message(Com::Mab::BytesView msg);

	void on_receive_encoded_message(std::function<void(Com::Mab::BytesView msg)> const &receiver);

	size_t get_rpc_version_code() const;

protected:
	void on_receive_bytes(std::string_view bytes) override;

private:
	bool try_complete_head(std::string_view &bytes);

	void try_complete_message(std::string_view &bytes);

	std::function<void(Com::Mab::BytesView msg)> _receiver;
	std::optional<Head> _head;
	// received part-message buffer (after head)
	Com::Mab::Bytes _buffer;
};

}
#endif
