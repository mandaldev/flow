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
#ifndef FLOW_SRC_RPCSERVICE_HPP
#define FLOW_SRC_RPCSERVICE_HPP
#include "RpcSocket.hpp"
#include <set>

namespace Flow {

class RpcService
: public Com::Unique
{
public:
	struct Config
	{
		/// ip address of the service listens on
		std::vector<IPv4Addr> _ip_addrs;
		/// local address (abstract) of the service listens on
		std::vector<std::string> _local_addrs;
		/// clients wait queue size
		int backlog = 100;
	};

	RpcService(Flower &flower);

	void init(Config const &conf);

protected:
	/// this is the moment when you add rpc receivers
	virtual void on_new_client(std::weak_ptr<RpcSocket> cnt) = 0;

private:
	/// socket that listens to an address
	class ListenSocket
	: public StreamSocket
	{
	public:
		ListenSocket(Flower &flower, RpcService &owner);

		std::error_code setup(std::variant<IPv4Addr, std::string> const &addr, int backlog);

	protected:
		void on_accept(Flow::Socket client_socket) override;

		void on_close() override;

		void on_error(std::shared_ptr<Flow::Error> err) override;

		void on_flowing(std::shared_ptr<Flow::Error> err) override;

	private:
		RpcService &_owner;
		std::variant<IPv4Addr, std::string> _addr;
	};

	/// socket that serving a client
	class ServerSocket
	: public RpcSocket
	{
	public:
		ServerSocket(Flower &flower, RpcService &owner, std::variant<IPv4Addr, std::string> const &addr);

	protected:
		void on_close() override;

		void on_error(std::shared_ptr<Flow::Error> err) override;

		void on_flowing(std::shared_ptr<Flow::Error> err) override;
	private:
		RpcService &_owner;
		std::variant<IPv4Addr, std::string> _addr;
	};

private:
	Config _conf;
	Flower &_flower;

	std::set<std::shared_ptr<AsyncSocket>> _listen_socks;
	std::set<std::shared_ptr<AsyncSocket>> _servers;
};

}
#endif
