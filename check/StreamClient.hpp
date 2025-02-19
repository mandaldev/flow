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
#ifndef _FLOW_CHECK_STREAMCLIENT_HPP_
#define _FLOW_CHECK_STREAMCLIENT_HPP_
#include "../src/StreamSocket.hpp"

class StreamClient
: public Flow::StreamSocket
{
public:
	StreamClient(Flow::Flower &flower, std::string_view msg);

protected:
	void on_connect(std::error_code ec) override;

	void on_peer_close(bool only_read) override;

	void on_close() override;

	void on_error(std::shared_ptr<Flow::Error> err) override;

	void on_receive_bytes(std::string_view bytes) override;

	void on_flowing(std::shared_ptr<Flow::Error> err) override;

private:
	std::string _msg;
};

#endif
