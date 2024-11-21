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
#include <string>

int tcp_server(int argc, const char *argv[]);
int tcp_client(int argc, const char *argv[]);

int main(int argc, const char *argv[])
{
	if (argc > 1) {
		auto mode = std::string(argv[1]);
		if (mode == "s") {
			return tcp_server(argc - 1, &argv[1]);
		}
		if (mode == "c") {
			return tcp_client(argc - 1, &argv[1]);
		}
	}
	std::cout << "Usage: " << argv[0] << " s|c args..." << std::endl;
	return 1;
}
