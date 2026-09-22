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
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "Socket.hpp"
#include "StreamMessageSocket.hpp"
#include "Timer.hpp"
#include <Com/ThreadPool.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <set>

namespace Flow {

/// RPC Service class
/// all public api can be called outside of Flow thread
class RpcService
: public Com::Unique
, public Com::EnableBindWeak<RpcService>
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
		unsigned threads = 1;
	};

	RpcService(Flower &flower);
	~RpcService();

	void init(Config const &conf, std::function<void(size_t addr_index, std::error_code)> const &on_init_failed);

protected:
	/// called when new client has connected
	virtual void on_client_connected(Socket const &sock) {}

	/// called when one client has closed
	virtual void on_client_closed(Socket const &sock) {}

	/// request hanlder
	virtual void on_request(Socket const &sock, RpcRequest const &req, RpcResponse &res) = 0;

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
	: public StreamMessageSocket
	{
	public:
		ServerSocket(Flower &flower, RpcService &owner, std::variant<IPv4Addr, std::string> const &addr);

		void check_response_timeout();

	protected:
		void on_close() override;

		void on_error(std::shared_ptr<Flow::Error> err) override;

		void on_flowing(std::shared_ptr<Flow::Error> err) override;

		void on_receive_message(Com::BytesView msg) override;

	private:
		struct Request
		{
			std::shared_ptr<RpcRequest> req;
			std::chrono::milliseconds arrive_ts;
			uint64_t task_id = 0;
		};

		void dispatch(RpcRequest const &req);

		RpcService &_owner;
		std::variant<IPv4Addr, std::string> _addr;
		std::map<uint64_t, Request> _requests;
	};

	void check_response_timeout();

private:
	Config _conf;
	Flower &_flower;
	std::shared_ptr<Com::ThreadPool> _tp;

	std::set<std::shared_ptr<ListenSocket>> _listen_socks;
	std::set<std::shared_ptr<ServerSocket>> _servers;
	std::shared_ptr<Timer> _timer;
};

}
#endif
