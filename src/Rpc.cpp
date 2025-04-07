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
#include "Rpc.hpp"

namespace Flow {

uint32_t rpc_version_code()
{
	const auto major = 0u;
	const auto minor = 1u;
	const auto release = 0u;
	return major * 10000 + minor * 100 + release;
}

char const *rpc_version_string()
{
	return "0.1.0";
}

}
