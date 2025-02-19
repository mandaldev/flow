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
#include "TCPClient.hpp"
#include <Com/log.hpp>

TCPClient::TCPClient(Flow::Flower &flower, std::string_view msg)
: Flow::StreamSocket(flower)
, _msg(msg)
{
}

void TCPClient::on_connect(std::error_code ec)
{
	if (ec) {
		LOG_ERROR << "connect failed: " << ec.message();
		ec = close();
	} else {
		Flow::IPv4Addr addr;
		ec = get_name(addr);
		LOG_ERROR << "client connected at: [" << addr.ip << ":" << addr.port << "]";

		send_bytes(_msg);
	}
}

void TCPClient::on_peer_close(bool only_read)
{
	LOG_DEBUG << "peer socket closed only read: " << only_read;
}

void TCPClient::on_close()
{
	LOG_DEBUG << "client socket closed, shutdown";
	_flower.quit();
}

void TCPClient::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "client socket flow error: " << err->message();
	}
}

void TCPClient::on_receive_bytes(std::string_view bytes)
{
	LOG_DEBUG << "message from server: " << bytes;
	auto ec = close();
	LOG_DEBUG << "closing connection: " << ec;
}

void TCPClient::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "add client socket flow error: " << err->message();
	} else {
		LOG_ERROR << "client socket is flowing";
	}
}

int main(int argc, const char *argv[])
{
	Flow::Flower flower;
	std::string msg = argc > 1 ? argv[1] : "hello!";
	auto client = std::make_shared<TCPClient>(flower, msg);
	flower.add_task([&]()
	{
		auto ec = client->create_tcp_socket();
		if (ec) {
			flower.quit();
			LOG_ERROR << "failed to create client socket: " << ec.message();
			return;
		}
		Flow::IPv4Addr addr;
		addr.ip = "127.0.0.1";
		addr.port = 5000;
		ec = client->connect_ipv4(addr);
		if (ec) {
			flower.quit();
			[[maybe_unused]]
			auto ec_close = client->Socket::close();
			LOG_ERROR << "client socket failed to connect server: " << ec.message();
			return;
		}
	});
	auto err = flower.run();
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "flower run returns error: " << err->message();
		return 1;
	}
	return 0;
}
