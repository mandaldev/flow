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

class RpcSocket
: public StreamSocket
{
public:
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

	explicit RpcSocket(Flower &flower);

	size_t get_rpc_version_code() const;

	/// call a rpc or response
	template <typename... Args>
	bool call(std::string_view rpc_name, Args&&... args)
	{
		Com::Mab::Bytes out;
		if (!Com::Mab::serialize(out, rpc_name)
		|| !Com::Mab::serialize(out, std::forward<decltype(args)>(args)...)) {
			return false;
		}
		send_encoded_message(out);
		return true;
	}

	/// handle different rpc calls
	void set_rpc_receiver(std::string_view rpc_name, std::function<void(Com::Mab::BytesView)> const &receiver);

protected:
	void on_receive_bytes(std::string_view bytes) override;

private:
	void on_receive_encoded_message(Head const &head, Com::Mab::BytesView msg);

	/// send a message to peer
	void send_encoded_message(Com::Mab::BytesView msg);

	bool try_complete_head(std::string_view &bytes);

	void try_complete_message(std::string_view &bytes);

	std::map<std::string, std::function<void(Com::Mab::BytesView msg)>> _receivers;

	std::optional<Head> _head;
	// received part-message buffer (after head)
	Com::Mab::Bytes _buffer;
};

}
#endif
