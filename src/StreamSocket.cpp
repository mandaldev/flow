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
#include "StreamSocket.hpp"
#include "AsyncSocket.hpp"
#include <cerrno>
#include <memory>
#include <mutex>
#include <Com/log.hpp>
#include <sys/socket.h>

namespace Flow {

StreamSocket::StreamSocket(Flower &flower)
: AsyncSocket(flower)
{
}

std::error_code StreamSocket::create(int domain, int protocol)
{
	return AsyncSocket::create(domain, SOCK_STREAM, protocol);
}

void StreamSocket::send_bytes(std::string_view bytes)
{
	auto weak_this = weak_from_this();
	std::lock_guard<std::mutex> locker(_mtx_data);
	_data.push_back(std::string(bytes.data(), bytes.size()));
	// try to write
	_flower.post_task([weak_this]()
	{
		if (auto ptr = std::dynamic_pointer_cast<StreamSocket>(weak_this.lock())) {
			ptr->on_write();
		}
	});
}

void StreamSocket::on_write()
{
	std::lock_guard<std::mutex> locker(_mtx_data);
	while (!_data.empty()) {
		auto &bytes = _data.front();
		auto count = bytes.size() - _bytes_sent;
		if (count == 0) {
			_data.pop_front();
			_bytes_sent =  0;
			continue;
		}
		size_t written = 0;
		auto ec = write({bytes.data() + _bytes_sent, count}, written);
		_bytes_sent += written;
		if (!ec) {
			continue;
		}
		if (ec.value() == EAGAIN) {
			break;
		}
		// TODO
		LOG_ERROR << "error while 'write': " << ec.message();
		break;
	}
}

void StreamSocket::on_read()
{
	const size_t BUF_SIZE = 4096;
	std::string data;
	std::string buf;
	while (true) {
		auto ec = read(buf, BUF_SIZE);
		if (!ec) {
			data += buf;
			continue;
		}
		if (ec.value() == EAGAIN) {
			break;
		}
		// TODO
		LOG_ERROR << "error while 'read': " << ec.message();
		break;
	}
	if (data.size()) {
		on_receive_bytes(data);
	}
}

}
