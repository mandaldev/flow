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
#include "StreamClient.hpp"
#include <Com/Xo/CommandLine.hpp>
#include <Com/log.hpp>
#include <sys/socket.h>

StreamClient::StreamClient(Flow::Flower &flower, Com::Bytes msg)
: Flow::StreamSocket(flower)
, _msg(msg)
{
}

void StreamClient::on_connect(std::error_code ec)
{
	if (ec) {
		LOG_ERROR << "connect failed: " << ec.message();
		ec = close();
	} else if (domain() == AF_INET) {
		Flow::IPv4Addr addr;
		ec = get_name(addr);
		LOG_ERROR << "client connected at: [" << addr.ip << ":" << addr.port << "]";

		send_bytes(std::move(_msg));
	} else if (domain() == AF_LOCAL) {
		std::string addr;
		ec = get_name(addr);
		LOG_ERROR << "client connected at: [" << addr << "]";

		send_bytes(std::move(_msg));
	}
}

void StreamClient::on_peer_close(bool only_read)
{
	LOG_DEBUG << "peer socket closed only read: " << only_read;
}

void StreamClient::on_close()
{
	LOG_DEBUG << "client socket closed, shutdown";
	_flower.quit();
}

void StreamClient::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "client socket flow error: " << err->message();
	}
}

void StreamClient::on_receive_bytes(Com::Bytes &&bytes)
{
	LOG_DEBUG << "message from server: " << Com::bytes_to_str(bytes);
	auto ec = close();
	LOG_DEBUG << "closing connection: " << ec;
}

void StreamClient::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "add client socket flow error: " << err->message();
	} else {
		LOG_ERROR << "client socket is flowing";
	}
}

int main(int argc, const char *argv[])
{
	Com::CommandLine cmd(argc, argv);
	auto type = cmd.get_switch("type");
	auto messages = cmd.get_arguments();
	Flow::Flower flower;
	std::string msg = messages.size() ? messages[0] : "hello!";
	auto client = std::make_shared<StreamClient>(flower, Com::str_to_bytes(msg));
	flower.add_task([&]()
	{
		if (type && type.value() == "tcp") {
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
		} else if (!type || type.value() == "local") {
			auto ec = client->create_local_socket();
			if (ec) {
				flower.quit();
				LOG_ERROR << "failed to create client socket: " << ec.message();
				return;
			}
			char const addr[] = "\0flow_check_server_addr";
			ec = client->connect_local(std::string_view(addr, sizeof(addr) - 1));
			if (ec) {
				flower.quit();
				[[maybe_unused]]
				auto ec_close = client->Socket::close();
				LOG_ERROR << "client socket failed to connect server: " << ec.message();
				return;
			}
		}
	});
	auto err = flower.run();
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "flower run returns error: " << err->message();
		return 1;
	}
	return 0;
}
