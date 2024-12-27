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
#include "TCPServer.hpp"
#include <Com/log.hpp>
#include <memory>

class TCPServerSocket
: public Flow::TCPSocket
{
public:
	TCPServerSocket(Flow::Flower &flower, TCPListenSocket &owner)
	: Flow::TCPSocket(flower)
	, _owner(owner)
	{}

protected:
	void on_peer_close(bool only_read) override;

	void on_close() override;

	void on_error(std::shared_ptr<Flow::Error> err) override;

	void on_receive_bytes(std::string_view bytes) override;

	void on_flowing(std::shared_ptr<Flow::Error> err) override;

private:
	TCPListenSocket &_owner;
	Flow::IPv4Addr _peer;
};

void TCPServerSocket::on_peer_close(bool only_read)
{
	LOG_DEBUG << "peer close [" << _peer.ip << ":" << _peer.port << "], only_read: " << only_read;
}

void TCPServerSocket::on_close()
{
	LOG_DEBUG << "server socket [" << _peer.ip << ":" << _peer.port << "] closed";
	_owner.remove_client(shared_from_this());
}

void TCPServerSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "server socket flow error: " << err->message();
	}
}

void TCPServerSocket::on_receive_bytes(std::string_view bytes)
{
}

void TCPServerSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	auto &&log = LOG_ERROR;
	auto ec = get_peer_name(_peer);
	if (ec) {
		log << "{failed to get new client peer name: " << ec.message() << "}";
	}
	log << "{new client: [" << _peer.ip << ":" << _peer.port << "]}";
	if (err && err->type() != Flow::ErrType::NoError) {
		log << "{add listen socket flow error: " << err->message() << "}";
	} else {
		log << "{client socket is flowing}";
	}
}


TCPListenSocket::TCPListenSocket(Flow::Flower &flower)
: Flow::TCPSocket(flower)
{
}

void TCPListenSocket::remove_client(std::shared_ptr<Flow::AsyncSocket> c)
{
	_clients.erase(c);
}

void TCPListenSocket::on_accept(Flow::Socket client_socket)
{
	auto client = std::make_shared<TCPServerSocket>(_flower, *this);
	auto ec = client->Attach(client_socket);
	if (ec) {
		LOG_ERROR << "failed to attach new client socket: " << ec.message();
		return;
	}
	client->setup_client_flow();
	_clients.insert(client);
}

void TCPListenSocket::on_close()
{
	LOG_DEBUG << "listen socket closed";
}

void TCPListenSocket::on_error(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "listen socket flow error: " << err->message();
	}
}

void TCPListenSocket::on_flowing(std::shared_ptr<Flow::Error> err)
{
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "add listen socket flow error: " << err->message();
	} else {
		LOG_DEBUG << "listen socket is flowing";
	}
}

int main(int argc, const char *argv[])
{
	Flow::Flower flower;
	auto server = std::make_shared<TCPListenSocket>(flower);
	flower.add_task([&]()
	{
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
	});
	auto err = flower.run();
	if (err && err->type() != Flow::ErrType::NoError) {
		LOG_ERROR << "flower run returns error: " << err->message();
		return 1;
	}
	return 0;
}
