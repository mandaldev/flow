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
#include "Flower.hpp"
#include <Com/log.hpp>
#include <cerrno>
#include <chrono>
#include <thread>

extern "C" {
#include <sys/epoll.h>
#include <unistd.h>
}

using namespace std::chrono_literals;

namespace Flow {

const auto MAX_TIMEOUT = 10ms;

thread_local Flower const *Flower::_bound_flower = nullptr;

std::shared_ptr<Error> Flower::run()
{
	if (_bound_flower) {
		auto ec = std::make_error_code(std::errc::operation_not_supported);
		auto err = std::make_shared<SetupFlowerFailedError>();
		err->code(ec);
		err->message("current thread is used by another flower");
		return err;
	}
	_bound_flower = this;
	_epoll = epoll_create1(EPOLL_CLOEXEC);
	if (_epoll == -1) {
		auto ec = std::make_error_code(std::errc(errno));
		auto err = std::make_shared<SetupFlowerFailedError>();
		err->code(ec);
		err->message("Failed to create epoll");
		_bound_flower = nullptr;
		return err;
	}

	std::shared_ptr<Error> ret;
	auto timeout = 0ms;
	while (_run) {
		do_calls();

		std::vector<epoll_event> events;
		{
			std::shared_lock<std::shared_mutex> locker(_mtx_listeners);
			events.resize(_listeners.size());
		}
		if (events.empty()) {
			// sleep a while
			timeout++;
			timeout = std::min(timeout, MAX_TIMEOUT);
			// TODO what if calls added
			std::this_thread::sleep_for(timeout);
			continue;
		}

		timeout = std::min(timeout, MAX_TIMEOUT);
		auto n = epoll_wait(_epoll, events.data(), events.size(), timeout.count());
		if (n == 0) {
			// timeout
			timeout++;
			continue;
		}

		if (n < 0) {
			// TODO
			auto ec = std::make_error_code(std::errc(errno));
			if (ec.value() != EINTR) {
				auto err = std::make_shared<FlowerPolllEventError>();
				err->code(ec);
				err->message("epoll_wait returns -1");
				ret = err;
				break;
			}
		}

		auto last_has_events = false;
		for (int i = 0; i < n; i++) {
			auto e = events[i];
			auto action = flow_action(e.data.fd);
			if (!action) {
				// TODO
				continue;
			}
			if (e.events & EPOLLERR) {
				// LOG_DEBUG << "epoll event ERR of " << e.data.fd;
				auto action = flow_action(e.data.fd);
				if (action && action->on_error) {
					auto err = std::make_shared<FlowEventError>();
					err->flow(e.data.fd);
					err->message("flow error event occured");
					action->on_error(err);
				}
				last_has_events = true;
			}
			if (e.events & EPOLLRDHUP) {
				// LOG_DEBUG << "epoll event RDHUP of " << e.data.fd;
				auto action = flow_action(e.data.fd);
				if (action && action->on_peer_close) {
					action->on_peer_close(true);
				}
				last_has_events = true;
			}
			if (e.events & EPOLLHUP) {
				// LOG_DEBUG << "epoll event HUP of " << e.data.fd;
				auto action = flow_action(e.data.fd);
				if (action && action->on_peer_close) {
					action->on_peer_close(false);
				}
				last_has_events = true;
			}
			if (e.events & EPOLLIN) {
				// LOG_DEBUG << "epoll event IN of " << e.data.fd;
				auto action = flow_action(e.data.fd);
				if (action && action->on_read) {
					action->on_read();
				}
				last_has_events = true;
			}
			if (e.events & EPOLLOUT) {
				// LOG_DEBUG << "epoll event OUT of " << e.data.fd;
				auto action = flow_action(e.data.fd);
				if (action && action->on_write) {
					action->on_write();
				}
				last_has_events = true;
			}
		}
		if (last_has_events) {
			timeout = 0ms;
		} else {
			timeout++;
		}
	}

	if (-1 == ::close(_epoll)) {
		auto ec = std::make_error_code(std::errc(errno));
		auto err = std::make_shared<ShutdownFlowerFailedError>();
		err->code(ec);
		err->message("Failed to close epoll");
		ret = err;
	}

	_epoll = -1;

	_bound_flower = nullptr;
	return ret;
}

void Flower::quit()
{
	_run = false;
}

bool Flower::is_runing()
{
	return _epoll != -1;;
}

void Flower::add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> done)
{
	if (is_flow_thread()) {
		do_add_flow(fd, action, std::move(done));
		return;
	}
	std::lock_guard<std::mutex> locker(_mtx);
	_calls.push_back([=](){
		do_add_flow(fd, action, std::move(done));
	});
}

void Flower::remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> done)
{
	if (is_flow_thread()) {
		do_remove_flow(fd, std::move(done));
		return;
	}
	std::lock_guard<std::mutex> locker(_mtx);
	_calls.push_back([=](){
		do_remove_flow(fd, std::move(done));
	});
}

bool Flower::has_flow(int fd)
{
	std::shared_lock<std::shared_mutex> locker(_mtx_listeners);
	return _listeners.count(fd);
}

void Flower::add_task(std::function<void()> task)
{
	if (is_flow_thread()) {
		do_post_task(std::move(task));
		return;
	}

	post_task(std::move(task));
}
void Flower::post_task(std::function<void()> task)
{
	std::lock_guard<std::mutex> locker(_mtx);
	_calls.push_back([=](){
		do_post_task(std::move(task));
	});
}

void Flower::do_add_flow(int fd, Action const &action, std::function<void(std::shared_ptr<Error>)> done)
{
	auto err = std::make_shared<AddFlowFailedError>();
	err->flow(fd);

	if (has_flow(fd)) {
		if (done) {
			err->code(std::make_error_code(std::errc::file_exists));
			err->message("Flow exists, remove it first if you want change the events to listen");
			done(err);
		}
		return;
	}

	epoll_event event{};
	event.events = EPOLLET;
	if (action.on_read) {
		event.events |= EPOLLIN;
	}
	if (action.on_write) {
		event.events |= EPOLLOUT;
	}
	if (action.on_peer_close) {
		event.events |= EPOLLRDHUP;
	}
	event.data.fd = fd;
	if (0 == epoll_ctl(_epoll, EPOLL_CTL_ADD, fd, &event)) {
		std::unique_lock<std::shared_mutex> locker(_mtx_listeners);
		_listeners[fd] = std::make_shared<Action>(action);
		err.reset();
	} else {
		auto ec = std::make_error_code(std::errc(errno));
		err->code(ec);
		err->message("Failed to add flow");
	}

	if (done) {
		done(err);
	}
}

void Flower::do_remove_flow(int fd, std::function<void(std::shared_ptr<Error>)> done)
{
	auto err = std::make_shared<RemoveFlowFailedError>();
	err->flow(fd);

	auto action = flow_action(fd);
	if (!action) {
		if (done) {
			err->code(std::make_error_code(std::errc::invalid_argument));
			err->message("Flow not exists");
			done(err);
		}
		return;
	}
	if (0 == epoll_ctl(_epoll, EPOLL_CTL_DEL, fd, nullptr)) {
		std::unique_lock<std::shared_mutex> locker(_mtx_listeners);
		_listeners.erase(fd);
		err.reset();
	} else {
		auto ec = std::make_error_code(std::errc(errno));
		err->code(ec);
		err->message("Failed to remove flow");
	}
	if (done) {
		done(err);
	}
}

void Flower::do_post_task(std::function<void()> task)
{
	if (task) {
		task();
	}
}

void Flower::do_calls()
{
	std::vector<std::function<void()>> calls;
	{
		std::lock_guard<std::mutex> locker(_mtx);
		calls = std::move(_calls);
	}
	for (auto &call : calls) {
		if (!call) {
			continue;
		}
		call();
	}
}

std::shared_ptr<Flower::Action> Flower::flow_action(int fd)
{
	std::shared_lock<std::shared_mutex> locker(_mtx_listeners);
	auto it = _listeners.find(fd);
	if (it != _listeners.end()) {
		return it->second;
	}
	return {};
}

bool Flower::is_flow_thread()
{
	return this == _bound_flower;
}

}
