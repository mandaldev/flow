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
#include "Timer.hpp"
#include "Error.hpp"
#include <Com/log.hpp>
#include <cassert>
#include <ctime>

extern "C" {
#include <sys/timerfd.h>
#include <unistd.h>
}

namespace Flow {

using namespace std::chrono;

static inline clockid_t convert_clock(Timer::Clock c)
{
	switch (c)
	{
	case Timer::Clock::Realtime:
		return CLOCK_REALTIME;
	case Timer::Clock::Monotonic:
		return CLOCK_MONOTONIC;
	case Timer::Clock::Boottime:
		return CLOCK_BOOTTIME;
	default:
		LOG_ERROR << "invalid clock type!";
		assert(0);
		return 0;
	}
}

static inline milliseconds convert_time(timespec const &ts)
{
	return milliseconds(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static inline timespec convert_time(milliseconds const &ms)
{
	timespec ts;
	ts.tv_sec = ms.count() / 1000;
	ts.tv_nsec = (ms - milliseconds(ts.tv_sec * 1000)).count() * 1000000;
	return ts;
}

std::chrono::milliseconds Timer::get_time(Clock type)
{
	milliseconds t;
	auto c = convert_clock(type);
	timespec ts;
	if (0 == clock_gettime(c, &ts)) {
		t = convert_time(ts);
	}
	return t;
}

Timer::Timer(Flower &flower, Clock clock)
: _flower(flower)
, _clock(clock)
{
}

Timer::~Timer()
{
	[[maybe_unused]]
	auto ec = destroy();
}

int Timer::timer_fd() const
{
	return _fd;
}

std::error_code Timer::set(std::chrono::milliseconds first, std::chrono::milliseconds interval)
{
	auto ec = create();
	if (ec) {
		return ec;
	}
	itimerspec its;
	its.it_value = convert_time(first);
	its.it_interval = convert_time(interval);
	if (timerfd_settime(_fd, 0, &its, nullptr) != 0) {
		auto ec = get_error_code();
		return ec;
	}
	if (_flower.has_flow(_fd)) {
		if (first == 0ms && interval == 0ms) {
			// reset
			_flower.remove_flow(_fd, [](std::shared_ptr<Error> err)
			{
				if (err && err->type() != ErrType::NoError) {
					LOG_ERROR << "Failed remove timer flow: " << err->message();
				}
			});
		}
		return {};
	}

	// add flow
	Flower::Action action;
	action.on_read = bind_weak([this]()
	{
		uint64_t count = 0;
		auto n = ::read(_fd, &count, sizeof(uint64_t));
		if (n == 8) {
			// notify
			on_timer(count);
		} else if (n == -1) {
			auto ec = get_error_code();
			if (ec.value() != EAGAIN) {
				LOG_ERROR << "read timer error: " << ec.message();
			}
		}
	});
	_flower.add_flow(_fd, action, [](std::shared_ptr<Error> err)
	{
		if (err && err->type() != ErrType::NoError) {
			LOG_ERROR << "Failed add timer flow, timer will not working: " << err->message();
		}
	});
	return {};
}

std::error_code Timer::get(std::chrono::milliseconds &until, std::chrono::milliseconds &interval)
{
	if (!valid()) {
		until = interval = {};
		return {};
	}
	itimerspec its;
	if (timerfd_gettime(_fd, &its) != 0) {
		return get_error_code();
	}
	until = convert_time(its.it_value);
	interval = convert_time(its.it_interval);
	return {};
}

bool Timer::valid() const
{
	return _fd != -1;
}

void Timer::on_timer(std::function<void(size_t count)> const &fn)
{
	_flower.add_task(bind_weak([this, fn]()
	{
		_on_timer = fn;
	}));
}

void Timer::on_timer(size_t count)
{
	if (_on_timer) {
		_on_timer(count);
	}
}

std::error_code Timer::create()
{
	if (valid()) {
		return {};
	}
	_fd = timerfd_create(convert_clock(_clock), TFD_CLOEXEC | TFD_NONBLOCK);
	if (!valid()) {
		return get_error_code();
	}
	return {};
}

std::error_code Timer::destroy()
{
	if (!valid()) {
		return {};
	}
	auto ec = set();
	if (::close(_fd) == 0) {
		_fd = -1;
		return {};
	} else {
		ec = get_error_code();
		LOG_ERROR << "Failed to close timer fd: " << ec.message();
		return ec;
	}
}

}
