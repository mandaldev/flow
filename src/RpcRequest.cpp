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
#include "RpcRequest.hpp"
#include "Rpc.hpp"
#include <Com/Mab/serialize.hpp>

namespace Flow {

RpcRequest::RpcRequest()
: _version(rpc_version_code())
{
}

uint32_t RpcRequest::version() const
{
	return _version;
}

void RpcRequest::version(uint32_t value)
{
	_version = value;
}

uint64_t RpcRequest::unique_id() const
{
	return _unique_id;
}

void RpcRequest::unique_id(uint64_t value)
{
	_unique_id = value;
}

uint32_t RpcRequest::timeout() const
{
	return _timeout;
}

void RpcRequest::timeout(uint32_t value)
{
	_timeout = value;
}

std::string RpcRequest::interface() const
{
	return _interface;
}

void RpcRequest::interface(std::string const &value)
{
	_interface = value;
}

std::string RpcRequest::method() const
{
	return _method;
}

void RpcRequest::method(std::string const &value)
{
	_method = value;
}

RpcCodecChain const &RpcRequest::args_format() const
{
	return _args_format;
}

void RpcRequest::args_format(RpcCodecChain const &value)
{
	_args_format = value;
}

Com::Bytes &RpcRequest::args()
{
	return _args;
}

Com::BytesView RpcRequest::const_args() const
{
	return _const_args;
}

void RpcRequest::const_args(Com::BytesView value)
{
	_const_args = value;
}

bool RpcRequest::serialize_to_mab_except_args(Com::Bytes &out) const
{
	return Com::Mab::serialize_kvs(out,
		"version", _version,
		"unique_id", _unique_id,
		"timeout", _timeout,
		"interface", _interface,
		"method", _method,
		"args_format", _args_format
	);
}

bool RpcRequest::deserialize_from_mab(Com::BytesView in, size_t &pos)
{
	auto ret = Com::Mab::deserialize_kvs(in, pos,
		"version", _version,
		"unique_id", _unique_id,
		"timeout", _timeout,
		"interface", _interface,
		"method", _method,
		"args_format", _args_format
	);
	if (ret) {
		_const_args = in.subview(pos);
	}
	return ret;
}

}
