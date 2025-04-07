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
#ifndef FLOW_SRC_RPCREQUEST_HPP
#define FLOW_SRC_RPCREQUEST_HPP
#include "Rpc.hpp"
#include <Com/BytesView.hpp>
#include <Com/Mab/serialize.hpp>
#include <cstdint>
#include <string>

namespace Flow {

class RpcRequest
{
public:
	/// only version will be filled in constructor
	RpcRequest();

	uint32_t version() const;

	void version(uint32_t value);

	uint64_t unique_id() const;

	void unique_id(uint64_t value);

	uint32_t timeout() const;

	void timeout(uint32_t value);

	std::string interface() const;

	void interface(std::string const &value);

	std::string method() const;

	void method(std::string const &value);

	Com::Bytes const &args() const;

	void args(Com::Bytes &&value);

	RpcCodecChain const &args_format() const;

	void args_format(RpcCodecChain const &value);

	bool serialize_to_mab(Com::Bytes &out) const;

	bool deserialize_from_mab(Com::BytesView in, size_t &pos);

private:
	uint32_t _version = 0;
	uint64_t _unique_id = 0;
	uint32_t _timeout = 0;
	std::string _interface;
	std::string _method;
	RpcCodecChain _args_format;
	Com::Bytes _args;
};

}

namespace Com::Mab {

template <>
inline bool serialize<Flow::RpcRequest>(Bytes &out, Flow::RpcRequest const &v)
{
	return v.serialize_to_mab(out);
}

template <>
inline bool deserialize<Flow::RpcRequest>(BytesView in, size_t &pos, Flow::RpcRequest &v)
{
	return v.deserialize_from_mab(in, pos);
}

}

#endif
