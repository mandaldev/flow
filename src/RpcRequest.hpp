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
#include <cstdint>
#include <string>

namespace Flow {

/// 一般情况下 `args` 可能比较大，为了避免在发送请求和接受请求的过程中不必要的拷贝，
/// 提供两个版本的参数变量，可修改的版本用于发送请求，不可变的用于接受请求。
/// 发送请求的时候，整个请求序列化成两个Bytes，一个是除args的其他所有数据，另一个是由用户序列化
/// 的args，二者顺序必须正确
class RpcRequest
{
public:
	/// only these fields will be filled in constructor
	/// - version: current version code
	/// - codec chain: `{ Mab }`
	RpcRequest();

	uint32_t version() const;

	void version(uint32_t value);

	uint64_t unique_id() const;

	void unique_id(uint64_t value);

	uint32_t timeout() const;

	void timeout(uint32_t value);

	std::string const &interface() const;

	void interface(std::string const &value);

	std::string const &method() const;

	void method(std::string const &value);

	RpcCodecChain const &args_format() const;

	void args_format(RpcCodecChain const &value);

	Com::Bytes &args();

	Com::BytesView const_args() const;

	void const_args(Com::BytesView value);

	/// to avoid copy, `args` is not serilized to `out`
	bool serialize_to_mab_except_args(Com::Bytes &out) const;

	/// 反序列化除args的其他所有数据后，剩下的Bytes被认为是args，之后可以通过
	/// `const_args` 访问。
	bool deserialize_from_mab(Com::BytesView in);

private:
	uint32_t _version = 0;
	uint64_t _unique_id = 0;
	uint32_t _timeout = 0;
	std::string _interface;
	std::string _method;
	RpcCodecChain _args_format = { RpcCodec::Mab };

	Com::Bytes _args;
	Com::BytesView _const_args = std::string_view();
};

}
#endif
