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
#include "RpcSocket.hpp"
#include <Com/log.hpp>
#include <cassert>

namespace Com::Mab {
template <>
bool deserialize<Flow::RpcSocket::Head>(Com::BytesView in, size_t &pos, Flow::RpcSocket::Head &head)
{
	if (Com::Mab::deserialize_kvs(
		in, pos,
		COM_KV(head, version_code),
		COM_KV(head, type),
		COM_KV(head, msg_size)
	)) {
		return true;
	} else {
		LOG_DEBUG << "deserialize head failed!";
		return false;
	}
}
}

namespace Flow {

static Com::Bytes serialize_head(RpcSocket::Head const &head)
{
	Com::Bytes bytes;
	// rpc version code
	if (!Com::Mab::serialize_kvs(
		bytes,
		COM_KV(head, version_code),
		COM_KV(head, type),
		COM_KV(head, msg_size)
	)) {
		LOG_ERROR << "serialize rpc message head failed!";
		assert(0);
		bytes.clear();
	}
	return bytes;
}

static size_t get_head_binary_size()
{
	static auto size = serialize_head({}).size();
	return size;
}

RpcSocket::RpcSocket(Flower &flower)
: StreamSocket(flower)
{
}

void RpcSocket::send_rpc_call(Com::Bytes &&data)
{
	std::vector<Com::Bytes> datas;
	datas.push_back(std::move(data));
	send_rpc_call(std::move(datas));
}

void RpcSocket::send_rpc_call(std::vector<Com::Bytes> &&datas)
{
	Head head;
	head.version_code = 1;
	head.type = MsgType::UserMessage;
	for (auto &data : datas) {
		head.msg_size += data.size();
	}
	if (head.msg_size == 0) {
		LOG_DEBUG << "empty message will not be sent!";
		return;
	}
	Com::Bytes pack = serialize_head(head);
	if (pack.size()) {
		send_bytes(std::move(pack));
		for (auto &data : datas) {
			send_bytes(std::move(data));
		}
		LOG_DEBUG << "send user message, size: " << head.msg_size;
	}
}

size_t RpcSocket::get_rpc_version_code() const
{
	return 1;
}

void RpcSocket::on_receive_encoded_message(Head const &head, Com::BytesView msg)
{
	switch (head.type) {
	case MsgType::UserMessage: {
		std::string name;
		size_t pos = 0;
		if (Com::Mab::deserialize(msg, pos, name)) {
			auto it = _receivers.find(name);
			if (it != _receivers.end() && it->second) {
				it->second(msg.subview(pos));
			}
		} else {
			LOG_ERROR << "rpc message invalid: pos = " << pos << ", size = " << msg.size();
		}
		break;
	}
	default: // TODO
		break;
	}
}

void RpcSocket::set_rpc_receiver(std::string_view rpc_name, std::function<void(Com::BytesView)> const& receiver)
{
	auto name = std::string(rpc_name);
	_flower.add_task(bind_weak([this, name, receiver]{
		_receivers[name] = receiver;
	}));
}

void RpcSocket::on_receive_bytes(Com::Bytes &&bytes)
{
	Com::BytesView view(bytes);
	while (view.size()) {
		if (!try_complete_head(view)) {
			return;
		}
		try_complete_message(view);
	}
}

bool RpcSocket::try_complete_head(Com::BytesView &bytes)
{
	if (_head) {
		return true;
	}
	auto head_size = get_head_binary_size();
	size_t size = head_size;
	if (_buffer.size() <= head_size) {
		size -= _buffer.size();
	}
	auto copy_size = std::min(size, bytes.size());
	_buffer.insert(_buffer.end(), bytes.begin(), bytes.begin() + copy_size);
	bytes = bytes.subview(copy_size);
	if (_buffer.size() == head_size) {
		Head h;
		if (Com::Mab::deserialize(_buffer, h)) {
			_head = h;
			_buffer.clear();
		} else {
			return false;
		}
	}
	return true;
}

void RpcSocket::try_complete_message(Com::BytesView &bytes)
{
	if (!_head || bytes.empty()) {
		return;
	}
	auto &h = _head.value();
	auto len = h.msg_size - _buffer.size();
	auto copy_size = std::min(len, bytes.size());
	_buffer.insert(_buffer.end(), bytes.begin(), bytes.begin() + copy_size);
	bytes = bytes.subview(copy_size);
	if (_buffer.size() == h.msg_size) { // message completes
		on_receive_encoded_message(h, _buffer);
		_head.reset();
		_buffer.clear();
	}
}

}
