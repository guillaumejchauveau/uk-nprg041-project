#ifndef NET_SOCKETS_H
#define NET_SOCKETS_H

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <poll.h>

#endif

#include "../utils/exception.h"
#include <string>
#include <utility>
#include <memory>

namespace net {
#if defined(_WIN32)
/// Common type for the actual socket.
typedef SOCKET socket_handle_t;
/// Common value for an invalid socket.
const socket_handle_t INVALID_SOCKET_HANDLE = INVALID_SOCKET;
#else
/// Common type for the actual socket.
typedef int socket_handle_t;
/// Common value for an invalid socket.
const socket_handle_t INVALID_SOCKET_HANDLE = -1;
#endif

/**
 * Replacement of struct addrinfo which holds its own copy of the values. This
 * allows the use of ::freeaddrinfo when needed.
 */
struct SocketAddress {
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  socklen_t ai_addrlen;
  sockaddr ai_addr;

  /**
   * Creates an instance with a socket address only.
   * @param addr The socket address
   * @param addrlen The length of the socket address
   */
  SocketAddress(sockaddr *addr, socklen_t addrlen) noexcept : ai_addr(*addr) {
    this->ai_family = addr->sa_family;
    this->ai_socktype = -1;
    this->ai_protocol = -1;
    this->ai_addrlen = addrlen;
  }

  /**
   * Creates an instance from an address info structure. The structure is not
   * freed.
   * @param addrinfo
   */
  SocketAddress(addrinfo *addrinfo) noexcept : ai_addr(*addrinfo->ai_addr) {
    this->ai_family = addrinfo->ai_family;
    this->ai_socktype = addrinfo->ai_socktype;
    this->ai_protocol = addrinfo->ai_protocol;
    this->ai_addrlen = addrinfo->ai_addrlen;
  }

  SocketAddress(const SocketAddress &socket_address) : ai_addr(socket_address.ai_addr) {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
  }

  SocketAddress(SocketAddress &&socket_address) noexcept : ai_addr(socket_address.ai_addr) {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
  }

  SocketAddress &operator=(const SocketAddress &socket_address) {
    if (this == &socket_address) {
      return *this;
    }
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
    this->ai_addr = socket_address.ai_addr;
    return *this;
  }

  SocketAddress &operator=(SocketAddress &&socket_address) noexcept {
    if (this == &socket_address) {
      return *this;
    }
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
    this->ai_addr = socket_address.ai_addr;
    return *this;
  }

  ~SocketAddress() = default;

  /**
   * Retrieves the host name corresponding to the address.
   * @param flags Flags for ::getnameinfo
   * @return The host name
   */
  std::string getHost(int flags = 0) const {
    std::string host;
    host.resize(20);

    int code;
    while ((code = getnameinfo(&this->ai_addr, this->ai_addrlen, &host.front(),
                               static_cast<unsigned int>(host.size()), nullptr, 0,
                               flags))) {
      if (code == EAI_OVERFLOW) {
        host.resize(host.size() * 2);
      } else {
        throw utils::AddressInfoException(code);
      }
    }
    host.resize(host.find(static_cast<char>(0)));
    return host;
  }

  /**
   * Retrieves the service/port corresponding to the address.
   * @param flags Flags for ::getnameinfo
   * @return The service name
   */
  std::string getService(int flags = 0) const {
    std::string service;
    service.resize(20);

    int code;
    while ((code = getnameinfo(&this->ai_addr, this->ai_addrlen, nullptr, 0, &service.front(),
                               static_cast<unsigned int>(service.size()),
                               flags))) {
      if (code == EAI_OVERFLOW) {
        service.resize(service.size() * 2);
      } else {
        throw utils::AddressInfoException(code);
      }
    }
    service.resize(service.find(static_cast<char>(0)));
    return service;
  }

  /**
   * @return A string with the form "host:port"
   */
  operator std::string() const {
    return this->getHost() + ":" + this->getService(NI_NUMERICSERV);
  }
};

/**
 * Wrapper for BSD sockets on Unix and Windows systems.
 */
class Socket {
protected:
  socket_handle_t handle_;
  std::unique_ptr<SocketAddress> address_;
  int last_error_;

  /**
   * @throw std::runtime_error Thrown if the socket is in an invalid state
   */
  void checkState() const {
    if (this->isInvalid()) {
      throw std::runtime_error("Invalid socket state");
    }
  }

  /**
   * Asserts if the given error equals EWOULDBLOCK, depending on the platform.
   * @param error The error
   * @return The result of the test
   */
  static bool isErrorEWouldBlock(long int error);
  /**
   * Asserts if the given error equals EINPROGRESS, depending on the platform.
   * @param error The error
   * @return The result of the test
   */
  static bool isErrorEInProgress(long int error);
  /**
   *
   */
  void setNonBlocking();

public:
  /**
   * Creates a socket given an address and initializes the socket handler.
   * @param address The address is moved to prevent modifications outside the wrapper
   * @throw std::invalid_argument Thrown if the socket handler could not be created
   */
  Socket(std::unique_ptr<SocketAddress> &&address) : address_(
    std::move(address)) {
    this->handle_ =
      ::socket(this->address_->ai_family, this->address_->ai_socktype,
               this->address_->ai_protocol);
    this->last_error_ = 0;
    if (this->isInvalid()) {
      this->last_error_ = -1;
      throw std::invalid_argument("Invalid address");
    }
  }

  /**
   * Creates a socket with a valid socket handler and an address.
   * @param handle The socket handler to use
   * @param address The address is moved to prevent modifications outside the wrapper
   * @throw std::invalid_argument Thrown if the handler is invalid
   */
  Socket(socket_handle_t handle, std::unique_ptr<SocketAddress> &&address) : address_(
    std::move(address)) {
    this->handle_ = handle;
    this->last_error_ = 0;
    if (this->isInvalid()) {
      this->last_error_ = -1;
      throw std::invalid_argument("Invalid handle");
    }
  }

  /**
   * Prevents the copy of a wrapper instance.
   */
  Socket(const Socket &socket) = delete;

  Socket(Socket &&socket) noexcept : address_(std::move(socket.address_)) {
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
    this->last_error_ = socket.last_error_;
  }

  /**
   * Prevents the copy of a wrapper instance.
   */
  Socket &operator=(const Socket &socket) = delete;

  Socket &operator=(Socket &&socket) noexcept {
    if (this == &socket) {
      return *this;
    }
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
    this->address_ = std::move(socket.address_);
    this->last_error_ = socket.last_error_;
    return *this;
  }

  /**
   * Closes the socket before destructing the wrapper.
   */
  ~Socket() {
    this->close();
  }

  /**
   * @return The address associated with the socket
   */
  const std::unique_ptr<SocketAddress> &getAddress() const {
    return this->address_;
  }

  /**
   * @return The actual handler for the socket
   */
  socket_handle_t getHandle() const {
    return this->handle_;
  }

  /**
   * Tests if the socket is invalid.
   * @return The result of the test
   */
  bool isInvalid() const {
    return this->handle_ == INVALID_SOCKET_HANDLE;
  }

  /**
   * @return The last non-zero socket error as returned by ::getsockopt
   */
  int getLastError() {
    auto error = this->getsockopt<int>(SOL_SOCKET, SO_ERROR);
    if (*error != 0) {
      this->last_error_ = *error;
    }
    return this->last_error_;
  }

  /**
   * Retrieves a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be returned
   * @return The value of the socket option
   * @throw utils::Exception Thrown if the operation failed
   * @see ::getsockopt
   */
  template<typename T>
  std::unique_ptr<T> getsockopt(int level, int option_name) const {
    this->checkState();
    auto option_value = new T;
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    if ((::getsockopt(this->handle_, level, option_name,
                      reinterpret_cast<char *>(option_value),
                      reinterpret_cast<socklen_t *>(&option_len))) != 0) {
      throw utils::Exception::fromLastError();
    }
    return std::unique_ptr<T>(option_value);
  }

  /**
   * Sets a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be set
   * @param option_value A pointer to the value for the requested option
   * @throw utils::Exception Thrown if the operation failed
   * @see ::setsockopt
   */
  template<typename T>
  void setsockopt(int level, int option_name, T *option_value) {
    this->checkState();
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    if ((::setsockopt(this->handle_, level, option_name,
                      reinterpret_cast<char *>(option_value), option_len)) != 0) {
      throw utils::Exception::fromLastError();
    }
  }

  /**
   * Sets a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be set
   * @param option_value The value for the requested option
   * @throw utils::Exception Thrown if the operation failed
   * @see ::setsockopt
   */
  template<typename T>
  void setsockopt(int level, int option_name, T option_value) {
    this->checkState();
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    if ((::setsockopt(this->handle_, level, option_name,
                      reinterpret_cast<char *>(&option_value), option_len)) != 0) {
      throw utils::Exception::fromLastError();
    }
  }

  /**
   * Binds the socket handler to the configured address.
   * @throw utils::Exception Thrown if the binding failed
   * @see ::bind
   */
  void bind() {
    this->checkState();
    if ((::bind(this->handle_, &this->address_->ai_addr,
                this->address_->ai_addrlen)) != 0) {
      throw utils::Exception::fromLastError();
    }
  }

  /**
   * Connects the socket handler to the configured address.
   * @throw utils::Exception Thrown if the connection failed
   * @see ::connect
   */
  void connect() {
    this->checkState();
    if ((::connect(this->handle_, &this->address_->ai_addr,
                   this->address_->ai_addrlen)) != 0) {
      auto error = utils::Exception::getLastError();
      if (Socket::isErrorEInProgress(error)) {
        return;
      }
      throw utils::Exception(error);
    }
  }

  /**
   * Listens for incoming connections.
   * @param max Maximum number of pending connection requests
   * @throw utils::Exception Thrown if the operation failed
   * @see ::listen
   */
  void listen(int max = SOMAXCONN) {
    this->checkState();
    if ((::listen(this->handle_, max)) != 0) {
      throw utils::Exception::fromLastError();
    }
  }

  /**
   * Accepts an incoming connection.
   * @param non_blocking_accepted Sets the client's socket asynchronous
   * @return The socket of the connected client
   * @throw utils::Exception Thrown if the operation failed
   * @see ::accept
   */
  std::unique_ptr<Socket> accept(bool non_blocking_accepted = false) const;

  /**
   * Receives data through the socket.
   * @param buf The buffer to store the data
   * @param len The length of the buffer
   * @param flags Flags for ::recv
   * @return The number of bytes written to the buffer. -1 if receiving would
   *  block on an asynchronous socket
   * @throw utils::Exception Thrown if the operation failed
   * @see ::recv
   */
  long int recv(char *buf, size_t len, int flags = 0) const {
    this->checkState();
    auto count = ::recv(this->handle_, buf, len, flags);
    if (count < 0) {
      auto error = utils::Exception::getLastError();
      if (Socket::isErrorEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   * Sends data through the socket.
   * @param buf The buffer of data to send
   * @param len The length of the buffer
   * @param flags Flags for ::send
   * @return The number of bytes sent. -1 if sending would  block on an
   *  asynchronous socket
   * @throw utils::Exception Thrown if the operation failed
   * @see ::send
   */
  long int send(const char *buf, size_t len, int flags = 0) const {
    this->checkState();
    auto count = ::send(this->handle_, buf, len, flags);
    if (count < 0) {
      auto error = utils::Exception::getLastError();
      if (Socket::isErrorEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   * Shuts down all or part of the connection open on the socket.
   * @param how Determines what to shut down:
   *  SHUT_RD   = No more receptions;
   *  SHUT_WR   = No more transmissions;
   *  SHUT_RDWR = No more receptions or transmissions.
   * @throw utils::Exception Thrown if the operation failed
   * @see ::shutdown
   */
  void shutdown(int how) {
    this->checkState();
    if ((::shutdown(this->handle_, how)) != 0) {
      throw utils::Exception::fromLastError();
    }
  }

  /**
   * Closes the socket handler.
   * @see ::close
   */
  void close();

  friend class SocketFactory;
};

/**
 * Utility class for creating sockets.
 */
class SocketFactory {
protected:
  /**
   * Allocates an addrinfo list with the given hints.
   * @param family_hint Address family hint
   * @param socktype_hint Socket type hint
   * @param protocol_hint Protocol hint
   * @param flags_hint Flags hint
   * @param name Hostname for the addresses
   * @param service Service/port for the addresses
   * @return The linked list of addresses
   * @throw utils::AddressInfoException Thrown if the operation failed
   * @see ::getaddrinfo
   */
  static addrinfo *getaddrinfo(int family_hint, int socktype_hint, int protocol_hint,
                               int flags_hint, const char *name, const char *service) {
    addrinfo hints{}, *info = nullptr;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = family_hint;
    hints.ai_socktype = socktype_hint;
    hints.ai_protocol = protocol_hint;
    hints.ai_flags = flags_hint;
    int error = ::getaddrinfo(name, service, &hints, &info);
    if (error != 0) {
      throw utils::AddressInfoException(error);
    }
    return info;
  }

public:
  /**
   * Creates a bound socket.
   * @param family_hint Address family hint
   * @param socktype_hint Socket type hint
   * @param protocol_hint Protocol hint
   * @param name Hostname for the addresses
   * @param service Service/port for the addresses
   * @param non_blocking Defines if the socket should be asynchronous (false by default)
   * @param reuse Defines if the socket should reuse a previously bound address
   * @return The socket
   */
  static std::unique_ptr<Socket> boundSocket(int family_hint, int socktype_hint, int protocol_hint,
                                             const char *name, const char *service,
                                             bool non_blocking = false, bool reuse = false) {
    auto info = SocketFactory::getaddrinfo(family_hint, socktype_hint, protocol_hint, AI_PASSIVE,
                                           name,
                                           service);

    std::unique_ptr<Socket> sock;
    std::unique_ptr<utils::Exception> error;
    // Tries the addresses until one is bound successfully.
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      auto address = std::make_unique<SocketAddress>(addr);
      try {
        sock = std::make_unique<Socket>(std::move(address));
        if (reuse) {
          sock->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        }
        sock->bind();
        if (non_blocking) {
          sock->setNonBlocking();
        }
      } catch (utils::Exception &e) {
        error = std::make_unique<utils::Exception>(std::move(e));
        sock.reset();
        continue;
      }
      break;
    }
    freeaddrinfo(info);
    if (!sock) {
      std::string message = "Unknown error";
      if (error) {
        message = error->getMessage();
      }
      throw utils::MessageException("Cannot create bound socket: %s", message.c_str());
    }
    return sock;
  }

  /**
   * Creates a connected socket.
   * @param socktype_hint Socket type hint
   * @param protocol_hint Protocol hint
   * @param name Hostname for the addresses
   * @param service Service/port for the addresses
   * @param non_blocking Defines if the socket should be asynchronous (false by default)
   * @return The socket
   */
  static std::unique_ptr<Socket> connectedSocket(int socktype_hint, int protocol_hint,
                                                 const char *name,
                                                 const char *service, bool non_blocking = false) {
    auto info = SocketFactory::getaddrinfo(AF_UNSPEC, socktype_hint, protocol_hint, 0, name,
                                           service);

    std::unique_ptr<Socket> sock;
    std::unique_ptr<utils::Exception> error;
    // Tries the addresses until one is connected successfully.
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      auto address = std::make_unique<SocketAddress>(addr);
      try {
        sock = std::make_unique<Socket>(std::move(address));
        sock->connect();
        if (non_blocking) {
          sock->setNonBlocking();
        }
      } catch (utils::Exception &e) {
        error = std::make_unique<utils::Exception>(std::move(e));
        sock.reset();
        continue;
      }
      break;
    }
    freeaddrinfo(info);
    if (!sock) {
      std::string message = "Unknown error";
      if (error) {
        message = error->getMessage();
      }
      throw utils::MessageException("Cannot create connected socket: %s", message.c_str());
    }
    return sock;
  }
};

/**
 * Utility class that automatically initializes and cleans the OS' socket API.
 */
class SocketInitializer {
protected:
#if defined(_WIN32)
  WSADATA wsadata_{};
#endif
public:
  SocketInitializer();
  SocketInitializer(const SocketInitializer &other) = delete;
  SocketInitializer(SocketInitializer &&other) = delete;
  SocketInitializer &operator=(const SocketInitializer &other) = delete;
  SocketInitializer &operator=(SocketInitializer &&other) = delete;
  ~SocketInitializer();
};
} // namespace net

#endif // NET_SOCKETS_H
