// Flow - a network io lib
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
#ifndef FLOW_SRC_RPCRESPONSE_HPP
#define FLOW_SRC_RPCRESPONSE_HPP
#include "Rpc.hpp"
#include <Com/BytesView.hpp>
#include <Com/Mab/serialize.hpp>
#include <cstdint>
#include <string_view>

namespace Flow {

/// returns的序列化设计同RpcRequest的args。
class RpcResponse
{
public:
	/// only these fields will be filled in constructor
	/// - version: current version code
	/// - codec chain: `{ Mab }`
	RpcResponse();

	uint32_t version() const;

	void version(uint32_t value);

	/// unique id is 0 if this response is invalid (means error occured)
	uint64_t unique_id() const;

	void unique_id(uint64_t value);

	RpcStatusCode const &status() const;

	void status(RpcStatusCode const &value);

	RpcCodecChain const &returns_format() const;

	void returns_format(RpcCodecChain const &value);

	Com::BytesView const_returns() const;

	void const_returns(Com::BytesView value);

	Com::Bytes &returns();

	bool serialize_to_mab_except_returns(Com::Bytes &out) const;

	bool deserialize_from_mab(Com::BytesView in);

private:
	uint32_t _version = 0;
	uint64_t _unique_id = 0;
	RpcStatusCode _status;
	RpcCodecChain _returns_format = { RpcCodec::Mab };

	Com::Bytes _returns;
	Com::BytesView _const_returns = std::string_view();
};

}
#endif
