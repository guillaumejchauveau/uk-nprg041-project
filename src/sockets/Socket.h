#ifndef SOCKET_H
#define SOCKET_H

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#endif

#include <exception>
#include <string>
#include <utility>

namespace sockets {

class SocketException : public std::exception {
protected:
  const char *message;

public:
  explicit SocketException(const char *message) { this->message = message; }
  const char *what() const noexcept override { return this->message; }
};

#if defined(_WIN32)
typedef long int ssize_t;
typedef SOCKET socket_handle_t;
const socket_handle_t INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
typedef int socket_handle_t;
const socket_handle_t INVALID_SOCKET_HANDLE = -1;
#endif

struct SocketAddress {
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  socklen_t ai_addrlen;
  sockaddr ai_addr{};
  SocketAddress(sockaddr *addr, socklen_t addrlen) {
    this->ai_family = addr->sa_family;
    this->ai_socktype = 0;
    this->ai_protocol = 0;
    this->ai_addrlen = addrlen;
    this->ai_addr = *addr;
  }
  SocketAddress(addrinfo *addrinfo) {
    this->ai_family = addrinfo->ai_family;
    this->ai_socktype = addrinfo->ai_socktype;
    this->ai_protocol = addrinfo->ai_protocol;
    this->ai_addrlen = addrinfo->ai_addrlen;
    this->ai_addr = *addrinfo->ai_addr;
  }
  SocketAddress(SocketAddress &&socket_address) noexcept {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
    this->ai_addr = socket_address.ai_addr;
  }
  SocketAddress &operator=(SocketAddress &&socket_address) noexcept {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
    this->ai_addr = socket_address.ai_addr;
    return *this;
  }
};

class Socket {
  socket_handle_t handle_;
  SocketAddress address_;

protected:
  void handleResult(int result, const std::string &tag);

public:
  Socket(SocketAddress &&address) : address_(std::move(address)) {
    this->handle_ =
        ::socket(address.ai_family, address.ai_socktype, address.ai_protocol);
    if (this->isInvalid()) {
      throw SocketException("Invalid address");
    }
  }
  Socket(socket_handle_t handle, SocketAddress &&address)
      : address_(std::move(address)) {
    this->handle_ = handle;
    if (this->isInvalid()) {
      throw SocketException("Invalid handle");
    }
  }
  Socket(const Socket &socket) = delete;
  Socket(Socket &&socket) noexcept : address_(std::move(socket.address_)) {
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
  }
  ~Socket() { this->close(); }

  Socket &operator=(const Socket &socket) = delete;
  Socket &operator=(Socket &&socket) noexcept {
    if (this == &socket) {
      return *this;
    }
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
    this->address_ = std::move(socket.address_);
    return *this;
  }

  bool operator==(Socket &socket) const {
    return this->handle_ == socket.handle_;
  }
  bool operator==(const Socket &socket) const {
    return this->handle_ == socket.handle_;
  }

  bool isInvalid() const { return this->handle_ == INVALID_SOCKET_HANDLE; }

  void bind() {
    this->handleResult(::bind(this->handle_, &this->address_.ai_addr,
                              this->address_.ai_addrlen),
                       "BIND");
  }
  void connect() {
    this->handleResult(::connect(this->handle_, &this->address_.ai_addr,
                                 this->address_.ai_addrlen),
                       "CONNECT");
  }
  void listen(int max = SOMAXCONN) {
    this->handleResult(::listen(this->handle_, max), "LISTEN");
  }
  Socket *accept() {
    socklen_t client_sock_address_len = sizeof(sockaddr_storage);
    auto client_sock_address =
        reinterpret_cast<sockaddr *>(new sockaddr_storage);
    memset(client_sock_address, 0, client_sock_address_len);

    socket_handle_t client_socket =
        ::accept(this->handle_, client_sock_address, &client_sock_address_len);
    if (client_socket == INVALID_SOCKET_HANDLE) {
      throw SocketException("Accept returned an invalid handle");
    }
    auto socket_address =
        SocketAddress(client_sock_address, client_sock_address_len);
    delete client_sock_address;
    return new Socket(client_socket, std::move(socket_address));
  }
  ssize_t recv(void *buf, size_t len, int flags = 0);
  ssize_t recvfrom(void *buf, size_t len, int flags, SocketAddress &address);
  ssize_t send(void *buf, size_t len, int flags = 0);
  ssize_t sendto(void *buf, size_t len, int flags,
                 const SocketAddress &address);
  void shutdown(int how) {
    this->handleResult(::shutdown(this->handle_, how), "SHUTDOWN");
  }
  void close();
};
} // namespace sockets

#endif // SOCKET_H
