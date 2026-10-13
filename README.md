# Flow

Flow is a simple async io framework based on linux epoll.

[Flower](src/Flower.hpp) is the core obejct of Flow framework, it's basically a manager of linux epoll.
Flower accepts raw file descriptor and callbacks, it doesn't care what kind of object the fd really is.

Currently Flow provides object-oriented facilities for some async io:

- Event
- Timer
- Socket
- RPC
