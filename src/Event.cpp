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
#include "Event.hpp"
#include "Flower.hpp"
#include <Com/log.hpp>

extern "C" {
#include <cassert>
#include <sys/eventfd.h>
#include <unistd.h>
}

namespace Flow {
Event::Event(Flower &flower)
: _flower(flower)
{
}

Event::~Event()
{
	[[maybe_unused]]
	auto ec = destroy();
}

std::error_code Event::emit()
{
	uint64_t value = 1;
	auto n = ::write(_fd, &value, sizeof(uint64_t));
	if (n != sizeof(uint64_t)) {
		assert(n == -1);
		auto ec = get_error_code();
		if (ec.value() == EAGAIN) {
			// NOTE overflow
		} else {
			LOG_ERROR << "emit event error: " << ec.message();
		}
		return ec;
	}
	return {};
}


bool Event::valid() const
{
	return _fd != -1;
}

bool Event::flowing() const
{
	return valid() && _flower.has_flow(_fd);
}

void Event::on_event(std::function<void(size_t count)> const &fn)
{
	_flower.post_task(bind_weak([this, fn]()
	{
		_on_event = fn;
	}));
}

void Event::on_event(size_t count)
{
	if (_on_event) {
		_on_event(count);
	}
}

std::error_code Event::create()
{
	if (valid()) {
		return {};
	}
	_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (_fd == -1) {
		return get_error_code();
	}
	// add flow
	Flower::Action action;
	action.on_read = bind_weak([this]()
	{
		uint64_t count = 0;
		auto n = ::read(_fd, &count, sizeof(uint64_t));
		if (n == 8) {
			// notify
			on_event(count);
		} else if (n == -1) {
			auto ec = get_error_code();
			if (ec.value() != EAGAIN) {
				LOG_ERROR << "read event error: " << ec.message();
			}
		}
	});
	_flower.add_flow(_fd, action, [](std::shared_ptr<Error> err)
	{
		if (err && err->type() != ErrType::NoError) {
			LOG_ERROR << "Failed add event flow, event will not working: " << err->message();
		}
	});
	return {};
}

std::error_code Event::destroy()
{
	if (!valid()) {
		return {};
	}
	_flower.remove_flow(_fd, [](std::shared_ptr<Error> err)
	{
		if (err && err->type() != ErrType::NoError) {
			LOG_ERROR << "Failed remove event flow: " << err->message();
		}
	});
	if (::close(_fd) == 0) {
		_fd = -1;
		return {};
	} else {
		auto ec = get_error_code();
		LOG_ERROR << "Failed to close event fd: " << ec.message();
		return ec;
	}
}

}
