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
#ifndef FLOW_SRC_RPC_HPP
#define FLOW_SRC_RPC_HPP
#include <cstdint>
#include <ostream>
#include <vector>

namespace Flow {

/// Flow RPC version code: MAJOR*100*100 + MINOR*100 + RELEASE
uint32_t rpc_version_code();

/// Flow RPC version string, like "1.20.3"
char const *rpc_version_string();

/// codec types used to encode data
enum class RpcCodec
: uint8_t
{
	/// Com::Mab binary format: any data type -> bytes
	Mab,
	/// Com::Mas text format: any data type -> string
	// Mas,
	/// JSON text format: any data type -> string
	// Json,
	/// zstd compressed: bytes or string -> bytes
	// Zstd
};

/// status code of rpc response
enum class RpcStatusCode
: uint8_t
{
	/// request is succeed responed
	Ok,
	/// bad interface (not such interface at server side)
	BadInterface,
	/// bad method (interface has not such method at server side)
	BadMethod,
	/// not supported rpc version
	BadVersion,
	/// request timeout if pending too long
	RequestTimeout,
	/// response timeout if server spent too mush time since
	/// client sent out request
	ResponseTimeout,
	/// bad arguments
	BadArguments,
};

static inline std::ostream &operator<<(std::ostream &o, RpcStatusCode c)
{
	switch (c) {
	case RpcStatusCode::Ok:
		return o << "Ok";
	case RpcStatusCode::BadInterface:
		return o << "BadInterface";
	case RpcStatusCode::BadMethod:
		return o << "BadMethod";
	case RpcStatusCode::BadVersion:
		return o << "BadVersion";
	case RpcStatusCode::RequestTimeout:
		return o << "RequestTimeout";
	case RpcStatusCode::ResponseTimeout:
		return o << "ResponseTimeout";
	case RpcStatusCode::BadArguments:
		return o << "BadArguments";
	default:
		return o << "";
	}
}

/// codecs those origin data has been encoded by.
/// for example a chain of "[Mab, Zstd]" means: origin data is serialized
/// by Mab and then compressed by zstd method
using RpcCodecChain = std::vector<RpcCodec>;

}
#endif
