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

namespace Flow {

class RpcResponse
{
public:
	/// only version will be filled in constructor
	RpcResponse();

	uint32_t version() const;

	void version(uint32_t value);

	uint64_t unique_id() const;

	void unique_id(uint64_t value);

	RpcStatusCode const &status() const;

	void status(RpcStatusCode const &value);

	Com::Bytes const &returns() const;

	void returns(Com::Bytes &&value);

	RpcCodecChain const &returns_format() const;

	void returns_format(RpcCodecChain const &value);

	bool serialize_to_mab(Com::Bytes &out) const;

	bool deserialize_from_mab(Com::BytesView in, size_t &pos);

private:
	uint32_t _version = 0;
	uint64_t _unique_id = 0;
	RpcStatusCode _status;
	RpcCodecChain _returns_format;
	Com::Bytes _returns;
};

}

namespace Com::Mab {

template <>
inline bool serialize<Flow::RpcResponse>(Bytes &out, Flow::RpcResponse const &v)
{
	return v.serialize_to_mab(out);
}

template <>
inline bool deserialize<Flow::RpcResponse>(BytesView in, size_t &pos, Flow::RpcResponse &v)
{
	return v.deserialize_from_mab(in, pos);
}

}
#endif
