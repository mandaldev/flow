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
#include <iostream>
#include <charconv>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
// #include <Com/string.hpp>
// #include <string>

int tcp_server(int argc, const char *argv[])
{
	if (argc != 3) {
		std::cerr << "need ip port to bind" << std::endl;
		return 1;
	}
	auto server_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (server_socket == -1) {
		std::cerr << "failed to create socket" << std::endl;
		return 1;
	}
	// std::string ip_str = argv[1];
	std::string port_str = argv[2];
	struct sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
		std::cerr << "invalid ip" << std::endl;
		return 1;
	}
	// short server_port = 0;
	if (std::from_chars(port_str.data(), port_str.data() + port_str.size(), server_addr.sin_port, 10).ec != std::errc()) {
		std::cerr << "invalid port" << std::endl;
		return 1;
	}
	server_addr.sin_port = htons(server_addr.sin_port);

	if (0 != bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr))) {
		std::cerr << "bind failed: " << std::strerror(errno) << std::endl;
		return 1;
	}

	if (0 != listen(server_socket, 1)) {
		std::cerr << "listen failed: " << std::strerror(errno) << std::endl;
		return 1;
	}

	struct sockaddr_in client_addr = {};
	socklen_t addrlen = sizeof(client_addr);
	auto client_socket = accept4(server_socket, (sockaddr*)&client_addr, &addrlen, SOCK_CLOEXEC);
	if (client_socket == -1) {
		std::cerr << "accept failed: " << std::strerror(errno) << std::endl;
		return 1;
	}
	char buf[32] = "";
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, 32);
	std::cout << "new client: " << buf << ":" << ntohs(client_addr.sin_port) << std::endl;

	std::string msg_buf, msg;
	msg_buf.resize(1024);
	do {
		auto n = read(client_socket, msg_buf.data(), msg_buf.size());
		if (n == 0) {
			std::cout << "client closed\n";
			break;
		}
		if (n < 0) {
			std::cerr << "read failed: " << std::strerror(errno) << std::endl;
			break;
		}
		msg.insert(msg.end(), msg_buf.begin(), msg_buf.begin() + n);
		do {
			auto ln = msg.find('\n');
			if (ln == msg.npos) {
				break;
			}
			ln += 1;
			auto l = std::string_view(msg.data(), ln);
			std::cout << "client:> " << l;
			n = write(client_socket, l.data(), l.size());

			msg.erase(msg.begin(), msg.begin() + ln);
		} while (true);
	} while (true);

	return 0;
}
