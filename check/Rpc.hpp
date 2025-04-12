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
#ifndef FLOW_CHECK_RPC_HPP
#define FLOW_CHECK_RPC_HPP
#include "../src/RpcClient.hpp"
#include "../src/RpcService.hpp"

using namespace Flow;

class RpcTestService
: public RpcService
{
public:
	RpcTestService(Flower &flower);

private:
	void on_request(RpcRequest const &req, RpcResponse &res) override;
};

class RpcTestClient
: public RpcClient
{
public:
	RpcTestClient(Flower &flower);

private:
};

#endif
