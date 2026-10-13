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
#ifndef FLOW_SRC_FLOWER_HPP_
#define FLOW_SRC_FLOWER_HPP_
#include "Error.hpp"
#include "Event.hpp"
#include <Com/raii.hpp>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace Flow {

/// The event loop, powered by epoll, edge-triggered
class Flower
: Com::Unique
{
public:
	/// called when an event occurs.
	/// if multiple events arrive at the same time, notifys will be called by order as
	/// they declared.
	struct Action {
		/// error notify
		std::function<void(std::shared_ptr<Error>)> on_error;
		/// if `only_read` is true, peer will never send data any more
		std::function<void(bool only_read)> on_peer_close;
		std::function<void()> on_read;
		std::function<void()> on_write;
	};

	/// run event loop.
	/// return null or NoError on success
	std::shared_ptr<Error> run();

	/// #async
	void quit();

	bool is_runing();

	/// 检查调用线程是否是此 flower 绑定的
	bool is_flow_thread();

	/// #async
	void add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> const &done);

	/// #async
	void remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> const &done);

	bool has_flow(int fd);

	/// if current is not flow thread, same as `post_task`
	void add_task(std::function<void()> const &task);

	void post_task(std::function<void()> const &task);

private:
	void do_add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> const &done);

	void do_remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> const &done);

	void do_post_task(std::function<void()> const &task);

	void do_calls();

	void create_event();

	void destroy_event();

	std::shared_ptr<Action> flow_action(int fd);

	std::mutex _mtx;
	std::vector<std::function<void()>> _calls;
	std::shared_ptr<Event> _event;

	/// which flower is runing on 'current' thred
	static thread_local Flower const *_bound_flower;
	std::atomic_bool _run {false};
	std::atomic_int _epoll = -1;

	std::shared_mutex _mtx_listeners;
	std::map<int, std::shared_ptr<Action>> _listeners;
};

}
#endif
