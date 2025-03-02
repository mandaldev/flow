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
bool deserialize<Flow::RpcSocket::Head>(Com::Mab::BytesView in, size_t &pos, Flow::RpcSocket::Head &head)
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

static Com::Mab::Bytes serialize_head(RpcSocket::Head const &head)
{
	Com::Mab::Bytes bytes;
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

void RpcSocket::send_encoded_message(Com::Mab::BytesView msg)
{
	if (msg.empty()) {
		LOG_DEBUG << "empty message will not be sent!";
		return;
	}
	Head head;
	head.version_code = 1;
	head.type = MsgType::UserMessage;
	head.msg_size = msg.size();
	Com::Mab::Bytes pack = serialize_head(head);
	if (pack.size()) {
		send_bytes({(const char*)pack.data(), pack.size()});
		LOG_DEBUG << "send user message, size: " << pack.size();
	}
}

void RpcSocket::on_receive_encoded_message(std::function<void(Com::Mab::BytesView msg)> const &receiver)
{
	_flower.post_task(bind_weak([this, receiver]{
		_receiver = receiver;
	}));
}

size_t RpcSocket::get_rpc_version_code() const
{
	return 1;
}

void RpcSocket::on_receive_bytes(std::string_view bytes)
{
	while (bytes.size()) {
		if (!try_complete_head(bytes)) {
			return;
		}
		try_complete_message(bytes);
	}
}

bool RpcSocket::try_complete_head(std::string_view &bytes)
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
	bytes = bytes.substr(copy_size);
	if (_buffer.size() == head_size) {
		Head h;
		if (Com::Mab::deserialize(bytes, h)) {
			_head = h;
			_buffer.clear();
		} else {
			return false;
		}
	}
	return true;
}

void RpcSocket::try_complete_message(std::string_view &bytes)
{
	if (!_head || bytes.empty()) {
		return;
	}
	auto &h = _head.value();
	auto len = h.msg_size - _buffer.size();
	auto copy_size = std::min(len, bytes.size());
	_buffer.insert(_buffer.end(), bytes.begin(), bytes.begin() + copy_size);
	bytes = bytes.substr(copy_size);
	if (_buffer.size() == h.msg_size) { // message completes
		if (_receiver) {
			_receiver(_buffer);
		}
		_head.reset();
		_buffer.clear();
	}
}

}
