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

/// RPC Client class
/// all public api can be called outside of Flow thread
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
		/// request timeout if response is not received from server
		std::chrono::milliseconds request_timeout{5000};
	};

	RpcClient(Flower &flower);

	void init(Config const &conf);

	/// send a request. request will be sent when connected to server
	template <typename Arg>
	bool request(std::string_view rpc_name, Arg &&arg)
	{
		Com::Mab::Bytes out;
		if (!Com::Mab::serialize(out, rpc_name)
		|| !Com::Mab::serialize(out, std::forward<decltype(arg)>(arg))) {
			LOG_ERROR << "failed to serialize arg";
			return false;
		}
		request(std::move(out));
		return true;
	}

	bool is_connected() const;

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

	void request(Com::Mab::Bytes data);

	void flush_requests();

private:
	Flower &_flower;

	Config _conf;
	std::shared_ptr<ClientSocket> _sock;

	std::atomic_bool _connected{false};
	std::vector<Com::Mab::Bytes> _requests;
};

}
#endif
