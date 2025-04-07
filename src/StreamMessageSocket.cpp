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
#include "StreamMessageSocket.hpp"
#include "StreamSocket.hpp"
#include <Com/BytesView.hpp>
#include <Com/log.hpp>
#include <cstdint>

namespace Flow {

StreamMessageSocket::StreamMessageSocket(Flower &flower)
: StreamSocket(flower)
{
}

void StreamMessageSocket::send_message(Com::Bytes &&msg)
{
	if (msg.size() == 0) {
		LOG_WARN << "zero size message, ignored!";
		return;
	}
	auto size = Com::Bytes(sizeof(_msg_size), 0);
	*(uint32_t*)size.data() = (uint32_t)msg.size();
	send_bytes(std::move(size));
	send_bytes(std::move(msg));
}

void StreamMessageSocket::max_message_size(uint32_t size)
{
	_max_msg_size = size;
}

uint32_t StreamMessageSocket::max_message_size() const
{
	return _max_msg_size;
}

void StreamMessageSocket::on_receive_bytes(Com::Bytes &&bytes)
{
	Com::BytesView view(bytes);
	while (view.size()) {
		if (!try_complete_message_size(view)) {
			return;
		}
		try_complete_message(view);
	}
}

void StreamMessageSocket::on_receive_message(std::function<void(Com::BytesView)> const &receiver)
{
	_flower.add_task(bind_weak([this, receiver]()
	{
		_receiver = receiver;
	}));
}

void StreamMessageSocket::on_receive_message(Com::BytesView msg)
{
	if (_receiver) {
		_receiver(msg);
	}
}

bool StreamMessageSocket::try_complete_message_size(Com::BytesView &bytes)
{
	if (_msg_size) {
		return true;
	}
	auto const uint_size = sizeof(_msg_size);
	size_t size = uint_size;
	if (_buffer.size() <= uint_size) {
		size -= _buffer.size();
	}
	auto copy_size = std::min(size, bytes.size());
	_buffer.insert(_buffer.end(), bytes.begin(), bytes.begin() + copy_size);
	bytes = bytes.subview(copy_size);
	if (_buffer.size() == uint_size) {
		_msg_size = *(uint32_t*)_buffer.data();
		_buffer.clear();
		if (!_msg_size) {
			LOG_WARN << "received a zero size message, ignored!";
		}
	}
	return true;
}

void StreamMessageSocket::try_complete_message(Com::BytesView &bytes)
{
	if (!_msg_size || bytes.empty()) {
		return;
	}
	auto len = _msg_size - _buffer.size();
	auto copy_size = std::min(len, bytes.size());
	_buffer.insert(_buffer.end(), bytes.begin(), bytes.begin() + copy_size);
	bytes = bytes.subview(copy_size);
	if (_buffer.size() == _msg_size) { // message completes
		on_receive_message(_buffer);
		_msg_size = 0;
		_buffer.clear();
	}
}


}
