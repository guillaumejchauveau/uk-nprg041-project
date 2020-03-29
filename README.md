# uk-nprg041-project

An HTTP application library in C++.

## Internals
 - src/utils/*: various utilities for error management, string manipulation...
 - src/net/sockets.h: OS sockets API abstraction layer.
 - src/net/tcp.h: Extensible TCP server, currently implemented only for Linux systems.
 - src/http/messages.h: Representation of HTTP requests and responses.
 - src/http/server.h: TCP server overlay for handling HTTP messages.

## Installation
Although it can be compiled on Windows, the library does not contain a Windows implementation for the TCP server.
This project uses CMake and C++17.
```bash
# Initialize the CMake cache in build directory
mkdir build && cmake -S . -B build
# Compile the demo program in build/src/main
cmake --build build --target main
```

## Demo
You can run the demo program with the `build/src/main` executable.
It's source code (`src/main.cpp`) contains three HTTP middleware which constitute
an HTTP application. The first middleware handles errors from the next middleware,
the second logs information about the request and the response and the third
generates a response for the request.
The main function initializes the socket library, creates an HTTP server running 
on the port 8080, configures the application middleware and finally starts the server
in the current thread.

While the demo is running you can access http://localhost:8080 to see the response.
