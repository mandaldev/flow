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
#include <arpa/inet.h>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

int tcp_client(int argc, const char *argv[])
{
	if (argc != 3) {
		std::cerr << "need ip port to connect" << std::endl;
		return 1;
	}
	auto client_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (client_socket == -1) {
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

	if (0 != connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr))) {
		std::cerr << "connect failed: " << std::strerror(errno) << std::endl;
		return 1;
	}
	auto run = true;
	while (run) {
		std::string msg_buf;
		msg_buf.resize(1024);
		std::cout << "client:> ";
		std::cin.getline(msg_buf.data(), (std::streamsize)msg_buf.size());
		std::string msg = msg_buf.data();
		msg += '\n';
		[[maybe_unused]]
		auto n = write(client_socket, msg.data(), msg.size());
		do {
			auto n = read(client_socket, msg_buf.data(), msg_buf.size());
			if (n == 0) {
				std::cout << "server closed\n";
				run = false;
				break;
			}
			if (n < 0) {
				std::cerr << "read failed: " << std::strerror(errno) << std::endl;
				run = false;
				break;
			}
			std::string msg;
			msg.insert(msg.end(), msg_buf.begin(), msg_buf.begin() + n);
			do {
				auto ln = msg.find('\n');
				if (ln == msg.npos) {
					break;
				}
				ln += 1;
				auto l = std::string_view(msg.data(), ln);
				std::cout << "server:> " << l;
				msg.erase(msg.begin(), msg.begin() + (std::ptrdiff_t)ln);
			} while (true);
			break;
		} while (true);
	}
	return 0;
}
