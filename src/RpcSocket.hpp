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
#include <Com/log.hpp>

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

	/// call a rpc or responses it
	template <typename Arg>
	bool call(std::string_view rpc_name, Arg &&arg)
	{
		Com::Bytes out;
		if (!Com::Mab::serialize(out, rpc_name)
		|| !Com::Mab::serialize(out, std::forward<decltype(arg)>(arg))) {
			LOG_ERROR << "failed to serialize arg";
			return false;
		}
		send_rpc_call(std::move(out));
		return true;
	}

	/// handle different rpc calls
	void set_rpc_receiver(std::string_view rpc_name, std::function<void(Com::BytesView)> const &receiver);

	/// send an encoded rpc call to peer
	void send_rpc_call(Com::Bytes &&data);

	/// same as `BytesView`, by bytes can be fractions, avoid bytes copyness
	void send_rpc_call(std::vector<Com::Bytes> &&datas);

protected:
	void on_receive_bytes(Com::Bytes &&bytes) override;

private:
	void on_receive_encoded_message(Head const &head, Com::BytesView msg);

	bool try_complete_head(Com::BytesView &bytes);

	void try_complete_message(Com::BytesView &bytes);

	std::map<std::string, std::function<void(Com::BytesView msg)>> _receivers;

	std::optional<Head> _head;
	// received part-message buffer (after head)
	Com::Bytes _buffer;
};

}
#endif
