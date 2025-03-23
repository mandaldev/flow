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
#ifndef FLOW_SRC_TIMER_HPP
#define FLOW_SRC_TIMER_HPP
#include <Com/memory.hpp>
#include <Com/raii.hpp>
#include <chrono>
#include <system_error>

namespace Flow {

class Flower;

class Timer
: Com::Unique
, public Com::EnableBindWeak<Timer>
{
public:
	enum Clock {
		/// A settable system-wide real-time clock
		Realtime,
		/// A nonsettable monotonically increasing clock that measures
		/// time from some unspecified point in the past that does not
		/// change after system startup
		Monotonic,
		/// like `Monotonic`, but include the time during which the
		/// system is suspended
		Boottime,
	};

	/// get current time of `type` clock
	static std::chrono::milliseconds get_time(Clock type);

	/// create a timer of `type` clock
	Timer(Flower &flower, Clock clock);

	~Timer();

	Timer(Timer &&) = delete;
	Timer &operator=(Timer &&) = delete;

	/// set timer:
	/// `first`: first expiration
	/// `interval`: repeated expiration after first
	/// set both to zero to disarm this timer
	/// #better_on_flow_thread
	std::error_code set(std::chrono::milliseconds first = {}, std::chrono::milliseconds interval = {});

	/// get timer settings:
	/// `until`: the time until next expiration
	/// `interval`: interval set by `set`
	std::error_code get(std::chrono::milliseconds &until, std::chrono::milliseconds &interval);

	/// check this is valid timer - successfully created.
	/// timer will be created until it's first set
	bool valid() const;

	bool flowing() const;

	/// callback
	void on_timer(std::function<void(size_t count)> const &fn);

protected:
	/// called when timer expired. `count` is the number of
	/// expirations that have occurred since last notify.
	/// override this to disable `on_timer` callback
	virtual void on_timer(size_t count);

private:
	/// create a timer of `clock`
	std::error_code create();

	/// destroy timer
	std::error_code destroy();

	Flower &_flower;
	int _fd = -1;
	Clock _clock = Clock::Monotonic;
	std::function<void(size_t count)> _on_timer;
};

}
#endif
