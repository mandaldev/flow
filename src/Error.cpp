// flow - a network io lib
// Copyright (C) 2024  Mandal Dev
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
#include "Error.hpp"
#include <cerrno>

namespace Flow {

ErrType Flow::Error::type() const
{
	return ErrType::NoError;
}

void Error::message(std::string const &msg)
{
	_msg = msg;
}

std::string Error::message() const
{
	return _msg;
}

void SystemError::code(std::error_code ec)
{
	_ec = ec;
}

std::error_code SystemError::code() const
{
	return _ec;
}

ErrType SetupFlowerFailedError::type() const
{
	return ErrType::SetupFlowerFailed;
}

ErrType ShutdownFlowerFailedError::type() const
{
	return ErrType::ShutdownFlowerFailed;
}

void FlowError::flow(int fd)
{
	_fd = fd;
}

int FlowError::flow() const
{
	return _fd;
}

ErrType AddFlowFailedError::type() const
{
	return ErrType::AddFlowFailed;
}

ErrType RemoveFlowFailedError::type() const
{
	return ErrType::RemoveFlowFailed;
}

ErrType FlowEventError::type() const
{
	return ErrType::FlowEvent;
}

}
