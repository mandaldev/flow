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
#ifndef FLOW_SRC_RPCCLIENT_HPP
#define FLOW_SRC_RPCCLIENT_HPP
#include "RpcSocket.hpp"

namespace Flow {

class RpcClient
{
public:
	struct Config
	{
		std::variant<IPv4Addr, std::string> server_addr;
		/// timeout if wait too long to connect (than retry to connect)
		std::chrono::milliseconds connection_timeout{10000};
		/// connection retry timeout if fails to connect server
		std::chrono::milliseconds retry_timeout{5000};
	};

	RpcClient(Flower &flower);

	void init(Config const &conf);

protected:
	/// this is the moment when you add rpc receivers
	virtual void on_connect(std::weak_ptr<RpcSocket> cnt) = 0;

private:
	/// socket that serving a client
	class ClientSocket
	: public RpcSocket
	{
	public:
		ClientSocket(Flower &flower, RpcClient &owner);

		std::error_code setup(std::variant<IPv4Addr, std::string> const &addr);

	protected:
		void on_close() override;

		void on_error(std::shared_ptr<Flow::Error> err) override;

		void on_flowing(std::shared_ptr<Flow::Error> err) override;

		void on_connect(std::error_code ec) override;

	private:
		RpcClient &_owner;
		std::variant<IPv4Addr, std::string> _addr;
	};

private:
	Flower &_flower;

	Config _conf;
	std::shared_ptr<ClientSocket> _sock;
};

}
#endif
