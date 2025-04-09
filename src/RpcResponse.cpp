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
#include "RpcResponse.hpp"

namespace Flow {

RpcResponse::RpcResponse()
: _version(rpc_version_code())
{
}

uint32_t RpcResponse::version() const
{
	return _version;
}

void RpcResponse::version(uint32_t value)
{
	_version = value;
}

uint64_t RpcResponse::unique_id() const
{
	return _unique_id;
}

void RpcResponse::unique_id(uint64_t value)
{
	_unique_id = value;
}

RpcStatusCode const &RpcResponse::status() const
{
	return _status;
}

void RpcResponse::status(RpcStatusCode const &value)
{
	_status = value;
}

RpcCodecChain const &RpcResponse::returns_format() const
{
	return _returns_format;
}

void RpcResponse::returns_format(RpcCodecChain const &value)
{
	_returns_format = value;
}

Com::BytesView RpcResponse::const_returns() const
{
	return _const_returns;
}

void RpcResponse::const_returns(Com::BytesView value)
{
	_const_returns = value;
}

Com::Bytes &RpcResponse::returns()
{
	return _returns;
}

bool RpcResponse::serialize_to_mab_except_returns(Com::Bytes &out) const
{
	return Com::Mab::serialize_kvs(out,
		"version", _version,
		"unique_id", _unique_id,
		"status", _status,
		"returns_format", _returns_format
	);
}

bool RpcResponse::deserialize_from_mab(Com::BytesView in, size_t &pos)
{
	auto ret = Com::Mab::deserialize_kvs(in, pos,
		"version", _version,
		"unique_id", _unique_id,
		"status", _status,
		"returns_format", _returns_format
	);
	if (ret) {
		_const_returns = in.subview(pos);
	}
	return ret;
}

}
