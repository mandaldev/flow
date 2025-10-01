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
#include <Com/BytesView.hpp>
#include <Com/Mab/serialize.hpp>
#include <Com/log.hpp>
#include <memory>
#include <system_error>

RpcTestService::RpcTestService(Flower &flower)
: RpcService(flower)
{
}

void RpcTestService::on_request(Socket const &sock, RpcRequest const &req, RpcResponse &res)
{
	if (req.interface() != "TestService") {
		res.status(RpcStatusCode::BadInterface);
		return;
	}
	if (req.method() != "ask") {
		res.status(RpcStatusCode::BadMethod);
		return;
	}
	std::string msg;
	if (!Com::Mab::deserialize(req.const_args(), msg)) {
		res.status(RpcStatusCode::BadArguments);
		return;
	}

	LOG_INFO << "message from client: " << msg;

	msg = "Hi, my code is 2025.";
	Com::Mab::serialize(res.returns(), msg);
	res.status(RpcStatusCode::Ok);
}

RpcTestClient::RpcTestClient(Flower &flower)
: RpcClient(flower)
{
}

int main(int argc, const char *argv[])
{
	const auto ADDR = "flow.check.rpc_test_service";
	Flower flower;
	RpcService::Config conf;
	conf._local_addrs.push_back(ADDR);
	auto service = std::make_shared<RpcTestService>(flower);
	service->init(conf, [&flower](size_t addr_index, std::error_code ec)
	{
		flower.quit();
	});

	auto client = std::make_shared<RpcTestClient>(flower);
	{
		// std::this_thread::sleep_for(std::chrono::seconds(5));
		RpcClient::Config conf;
		conf.server_addr = ADDR;
		client->init(conf, [&flower](std::error_code ec)
		{
			flower.quit();
		});

		RpcRequest req;
		req.interface("TestService");
		req.method("ask");

		std::string_view msg = "Hello, what's your code?";
		Com::Mab::serialize(req.args(), msg);
		client->request(std::move(req), [](RpcResponse const &res)
		{
			if (res.status() != RpcStatusCode::Ok) {
				LOG_ERROR << "response is not ok: " << (int)res.status();
				return;
			}
			std::string msg;
			if (Com::Mab::deserialize(res.const_returns(), msg)) {
				LOG_INFO << "answer from server: " << msg;
			} else {
				LOG_ERROR << "invalid returns format of response";
			}

		});
	}

	flower.run();
	return 0;
}
