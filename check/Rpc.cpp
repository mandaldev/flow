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
#include "Rpc.hpp"
#include <Com/log.hpp>
#include <future>
#include <thread>

RpcTestService::RpcTestService(Flower &flower)
: RpcService(flower)
{
}

void RpcTestService::on_new_client(std::weak_ptr<RpcSocket> cnt)
{
	auto sock = cnt.lock();
	if (!sock) {
		return;
	}
	// 'ask' from client
	sock->set_rpc_receiver("ask", [cnt](Com::BytesView data)
	{
		std::string msg;
		Com::Mab::deserialize(data, msg);
		LOG_INFO << "message from client: " << msg;
		if (auto sock = cnt.lock()) {
			msg = "hi, my code is 2025.";
			sock->call("ask", msg);
		}
	});
}

RpcTestClient::RpcTestClient(Flower &flower)
: RpcClient(flower)
{
}

void RpcTestClient::on_connect(std::weak_ptr<RpcSocket> cnt)
{
	auto sock = cnt.lock();
	if (!sock) {
		return;
	}
	// answer from server
	sock->set_rpc_receiver("ask", [](Com::BytesView data)
	{
		std::string msg;
		Com::Mab::deserialize(data, msg);
		LOG_INFO << "answer from server: " << msg;
	});
}

int main(int argc, const char *argv[])
{
	const auto ADDR = "flow.check.rpc_test_service";
	Flower flower;
	RpcTestService service(flower);
	RpcService::Config conf;
	conf._local_addrs.push_back(ADDR);
	service.init(conf);

	RpcTestClient client(flower);
	auto fut = std::async(std::launch::async, [&]()
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		RpcClient::Config conf;
		conf.server_addr = ADDR;
		client.init(conf);

		std::string_view msg = "hello, what's your code?";
		client.request("ask", msg);
	});
	flower.run();
	fut.get();
	return 0;
}
