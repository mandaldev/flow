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
#ifndef _FLOW_SRC_FLOWER_HPP_
#define _FLOW_SRC_FLOWER_HPP_
#include <Com/raii.hpp>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <system_error>
#include <functional>
#include <future>
#include <map>
#include <vector>
#include <thread>
#include <memory>
#include "Error.hpp"

namespace Flow {

/// the event loop
/// powered by epoll, edge-triggered
class Flower
: Com::Unique
{
public:
	/// called when an event occurs
	struct Action {
		std::function<void()> on_read;
		std::function<void()> on_write;
		/// if `only_read` is true, peer will never send data any more
		std::function<void(bool only_read)> on_peer_close;
		/// error notify
		std::function<void(std::shared_ptr<Error>)> on_error;
	};

	/// run event loop.
	/// return null or NoError on success
	std::shared_ptr<Error> run();

	/// #async
	void quit();

	bool is_runing();

	/// #async
	void add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> done);

	/// #async
	void remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> done);

	bool has_flow(int fd);

private:
	void do_add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> done);

	void do_remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> done);

	void do_calls();

	std::shared_ptr<Action> flow_action(int fd);

	std::mutex _mtx;
	std::vector<std::function<void()>> _calls;

	std::atomic_bool _run;
	std::atomic_int _epoll = -1;

	std::shared_mutex _mtx_listeners;
	std::map<int, std::shared_ptr<Action>> _listeners;
};

}
#endif
