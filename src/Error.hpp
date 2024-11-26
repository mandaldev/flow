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
#ifndef _FLOW_SRC_ERROR_HPP_
#define _FLOW_SRC_ERROR_HPP_
#include <string>
#include <system_error>
#include <Com/raii.hpp>

namespace Flow {

enum class ErrType
{
	NoError,
	/// failed to setup event loop implementation (epoll)
	SetupFlowerFailed,
	/// error occured while shutdonw flower
	ShutdownFlowerFailed,
	/// failed to add a flow (fd) to event loop
	AddFlowFailed,
	RemoveFlowFailed,
	/// error occured while waiting flow events
	FlowEvent,
};


class Error
: public Com::Unique
{
public:
	virtual ErrType type() const;

	void message(std::string const &msg);

	std::string message() const;

private:
	std::string _msg;
};

class SystemError
: public Error
{
public:
	void code(std::error_code ec);

	std::error_code code() const;

private:
	std::error_code _ec;
};

class SetupFlowerFailedError
: public SystemError
{
public:
	ErrType type() const override;
};

class ShutdownFlowerFailedError
: public SystemError
{
public:
	ErrType type() const override;
};

class FlowError
: public SystemError
{
public:
	/// the flow fd failed to add
	void flow(int fd);

	/// the flow fd failed to add
	int flow() const;

private:
	int _fd = -1;
};

class AddFlowFailedError
: public FlowError
{
public:
	ErrType type() const override;
};

class RemoveFlowFailedError
: public FlowError
{
public:
	ErrType type() const override;
};

class FlowEventError
: public FlowError
{
public:
	ErrType type() const override;
};

}
#endif
