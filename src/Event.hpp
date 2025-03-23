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
#ifndef FLOW_SRC_EVENT_HPP
#define FLOW_SRC_EVENT_HPP
#include "Flower.hpp"
#include <Com/memory.hpp>

namespace Flow {

class Event
: Com::Unique
, public Com::EnableBindWeak<Event>
{
public:
	Event(Flower &flower);

	~Event();

	/// #better_on_flow_thread
	std::error_code create();

	/// #better_on_flow_thread
	std::error_code destroy();

	std::error_code emit();

	/// check this is valid event - successfully created.
	bool valid() const;

	/// callback
	void on_event(std::function<void(size_t count)> const &fn);

protected:
	/// called when event emitted. `count` is the number of
	/// emits that have occurred since last notify.
	/// override this to disable `on_event` callback
	virtual void on_event(size_t count);

private:
	Flower &_flower;
	int _fd = -1;
	std::function<void(size_t count)> _on_event;
};

}
#endif
