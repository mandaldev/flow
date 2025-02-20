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
#include "StreamServer.hpp"
#include <Com/log.hpp>
#include <memory>
#include <Com/Xo/CommandLine.hpp>
#include <string_view>
#include <sys/socket.h>

class StreamServerSocket
: public Flow::StreamSocket
{
public:
	StreamServerSocket(Flow::Flower &flower, StreamListenSocket &owner)
	: Flow::StreamSocket(flower)
	, _owner(owner)
	{}

protected:
	void on_peer_close(bool only_read) override;

	void on_close() override;

	void on_error(std::shared_ptr<Flow::Error> err) override;

	void on_receive_bytes(std::string_view bytes) override;

	void on_flowing(std::shared_ptr<Flow::Error> err) override;

private:
	StreamListenSocket &_owner;
	Flow::IPv4Addr _tcp_peer;
	std::string _local_peer;
};

void StreamServerSocket::on_peer_close(bool only_read)
{
	if (domain() == AF_INET) {
		LOG_DEBUG << "peer close [" << _tcp_peer.ip << ":" << _tcp_peer.port << "], only_read: " << only_read;
	} else if (domain() == AF_LOCAL) {
		LOG_DEBUG << "peer close [" << _local_peer << "], only_read: " << only_read;
	}
}

void StreamServerSocket::on_close()
{
	if (domain() == AF_INET) {
		LOG_DEBUG << "server socket [" << _tcp_peer.ip << ":" << _tcp_peer.port << "] closed";
	} else if (domain() == AF_LOCAL) {
		LOG_DEBUG << "server socket [" << _local_peer << "] closed";
	}
	_owner.remove_client(shared_from_this());
}

void StreamServerSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "server socket flow error: " << err->message();
	}
}

void StreamServerSocket::on_receive_bytes(std::string_view bytes)
{
	LOG_DEBUG << "message from client: " << bytes;
	send_bytes(bytes);
}

void StreamServerSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (domain() == AF_INET) {
		auto ec = get_peer_name(_tcp_peer);
		if (ec) {
			LOG_ERROR << "failed to get new client peer name: " << ec.message();
		}
		LOG_DEBUG << "new client: [" << _tcp_peer.ip << ":" << _tcp_peer.port << "]";
	} else if (domain() == AF_LOCAL) {
		auto ec = get_peer_name(_local_peer);
		if (ec) {
			LOG_ERROR << "failed to get new client peer name: " << ec.message();
		}
		LOG_DEBUG << "new client: [" << _local_peer << "]";
	}
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "add listen socket flow error: " << err->message();
	} else {
		LOG_DEBUG << "client socket is flowing";
	}
}

StreamListenSocket::StreamListenSocket(Flow::Flower &flower)
: Flow::StreamSocket(flower)
{
}

void StreamListenSocket::remove_client(std::shared_ptr<Flow::AsyncSocket> c)
{
	_clients.erase(c);
}

void StreamListenSocket::on_accept(Flow::Socket client_socket)
{
	auto client = std::make_shared<StreamServerSocket>(_flower, *this);
	auto ec = client->Attach(client_socket);
	if (ec) {
		LOG_ERROR << "failed to attach new client socket: " << ec.message();
		return;
	}
	client->setup_client_flow();
	_clients.insert(client);
}

void StreamListenSocket::on_close()
{
	LOG_DEBUG << "listen socket closed";
}

void StreamListenSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "listen socket flow error: " << err->message();
	}
}

void StreamListenSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "add listen socket flow error: " << err->message();
	} else {
		LOG_DEBUG << "listen socket is flowing";
	}
}

int main(int argc, const char *argv[])
{
	Com::CommandLine cmd(argc, argv);
	auto type = cmd.get_switch("type");
	Flow::Flower flower;
	auto server = std::make_shared<StreamListenSocket>(flower);
	flower.add_task([&]()
	{
		if (type && type.value() == "tcp") {
			auto ec = server->create_tcp_socket();
			if (ec) {
				flower.quit();
				LOG_ERROR << "failed to create server socket: " << ec.message();
				return;
			}
			Flow::IPv4Addr addr;
			addr.ip = "127.0.0.1";
			addr.port = 5000;
			ec = server->listen_on(addr, 1024);
			if (ec) {
				flower.quit();
				[[maybe_unused]]
				auto ec_close = server->Socket::close();
				LOG_ERROR << "server socket failed to listen on 5000: " << ec.message();
				return;
			}
			LOG_DEBUG << "server socket listen on 5000";
		} else if (!type || type.value() == "local") {
			auto ec = server->create_local_socket();
			if (ec) {
				flower.quit();
				LOG_ERROR << "failed to create server socket: " << ec.message();
				return;
			}
			// abstract local socket
			char const addr[] = "\0flow_check_server_addr";
			ec = server->listen_on(std::string_view(addr, sizeof(addr) - 1), 1024);
			if (ec) {
				flower.quit();
				[[maybe_unused]]
				auto ec_close = server->Socket::close();
				LOG_ERROR << "server socket failed to listen on abstract address: " << ec.message();
				return;
			}
			LOG_DEBUG << "server socket listen on abstract address";
		}
	});
	auto err = flower.run();
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "flower run returns error: " << err->message();
		return 1;
	}
	return 0;
}
