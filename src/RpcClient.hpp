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
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "StreamMessageSocket.hpp"
#include "Timer.hpp"
#include <Com/BytesView.hpp>
#include <Com/memory.hpp>
#include <Com/raii.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <system_error>

namespace Flow {

/// RPC Client class
/// all public api can be called outside of Flow thread
class RpcClient
: public Com::Unique
, public Com::EnableBindWeak<RpcClient>
{
public:
	struct Config
	{
		std::variant<IPv4Addr, std::string> server_addr;
		/// timeout if wait too long to connect (than retry to connect)
		std::chrono::milliseconds connection_timeout{10000};
		/// request timeout if pending too long
		std::chrono::milliseconds request_timeout{5000};
		/// response timeout if server spent too mush time since
		/// client sent out request
		std::chrono::milliseconds response_timeout{5000};
	};

	RpcClient(Flower &flower);

	/// init may fail cause of different reasons:
	/// - failed to create socket
	/// - failed to connect to server
	/// - connection timeout
	void init(Config const &conf, std::function<void(std::error_code)> const &on_init_failed);

	/// send a rpc request, call `done` once response is received
	void request(RpcRequest &&req, std::function<void(RpcResponse&&)> const &done);

	bool is_connected() const;

private:
	struct Request
	{
		RpcRequest req;
		/// when this request is added
		std::chrono::milliseconds emit_ts;
		/// when this request is sent to peer
		std::chrono::milliseconds sent_ts;
		std::function<void(RpcResponse&&)> done;

		bool is_emitted() const { return emit_ts.count(); }
		bool is_sent() const { return sent_ts.count(); }
	};

	/// socket that serving a client
	class ClientSocket
	: public StreamMessageSocket
	{
	public:
		ClientSocket(Flower &flower, RpcClient &owner);

		std::error_code setup(std::variant<IPv4Addr, std::string> const &addr);

		bool request(Request &req);

	protected:
		void on_close() override;

		void on_error(std::shared_ptr<Flow::Error> err) override;

		void on_flowing(std::shared_ptr<Flow::Error> err) override;

		void on_connect(std::error_code ec) override;

		void on_receive_message(Com::BytesView msg) override;

	private:
		RpcClient &_owner;
		std::variant<IPv4Addr, std::string> _addr;
	};

	/// send all pending requests
	void flush_requests();

	uint64_t new_unique_id();

	void check_request_timeout();

	void check_connection_timeout();

	void on_response(Com::BytesView message);

	Flower &_flower;

	Config _conf;
	std::shared_ptr<ClientSocket> _sock;
	std::atomic_bool _connected{false};
	std::chrono::milliseconds _init_ts;
	std::function<void(std::error_code)> _on_init_failed;

	uint64_t _unique_id = 0;
	std::map<uint64_t, Request> _requests;
	std::shared_ptr<Timer> _timer;
};

}
#endif
