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

#include <exception>
#include <string>
#include <utility>
#include <stdexcept>

namespace net {
#if defined(_WIN32)
/// Not defined on Win32.
#define EAI_OVERFLOW ERROR_INSUFFICIENT_BUFFER
/// Common type for the actual socket.
typedef SOCKET socket_handle_t;
/// Common value for an invalid socket.
const socket_handle_t INVALID_SOCKET_HANDLE = INVALID_SOCKET;
typedef long int result_t;
#else
/// Common type for the actual socket.
typedef int socket_handle_t;
/// Common value for an invalid socket.
const socket_handle_t INVALID_SOCKET_HANDLE = -1;
typedef ssize_t result_t;
#endif

/**
 * Base exception type for socket operations.
 */
class SocketException : public std::exception {
protected:
  std::string message;
  result_t result;

public:
  /**
   *
   * @param message
   * @param result The return code of the function that failed
   */
  SocketException(const char *message, result_t result = 0) {
    this->message = message;
    this->result = result;
  }

  /**
   *
   * @param text
   * @param result The return code of the function that failed
   */
  SocketException(const std::string &message, result_t result = 0) {
    this->message = message;
    this->result = result;
  }

  /**
   * Platform-specific function for error handling.
   * @param result The return code of the function that failed
   * @param tag An indicator for the failing function
   * @throw SocketException
   */
  static void throwFromFailure(result_t result, const char *tag);

  /**
   * Creates an instance using printf-style formatting.
   *
   * @tparam Args Parameter pack type for the format arguments
   * @param format The base format string
   * @param result The return code of the function that failed
   * @param args Additional arguments for formatting
   * @see ::printf
   */
  template<typename ... Args>
  explicit SocketException(const std::string &format, result_t result = 0, Args ... args) {
    this->result = result;
    size_t size = static_cast<size_t>(snprintf(nullptr, 0, format.c_str(), args ...) + 1);
    if (size <= 0) {
      throw std::runtime_error("Error during formatting.");
    }
    auto buf = new char[size];
    snprintf(buf, size, format.c_str(), args ...);
    this->message = buf;
  }

  const char *what() const noexcept override {
    return this->message.c_str();
  }

  result_t getResult() {
    return this->result;
  }
};

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

  SocketAddress(sockaddr *addr, socklen_t addrlen) noexcept : ai_addr(*addr) {
    this->ai_family = addr->sa_family;
    this->ai_socktype = 0;
    this->ai_protocol = 0;
    this->ai_addrlen = addrlen;
  }

  SocketAddress(addrinfo *addrinfo) noexcept : ai_addr(*addrinfo->ai_addr) {
    this->ai_family = addrinfo->ai_family;
    this->ai_socktype = addrinfo->ai_socktype;
    this->ai_protocol = addrinfo->ai_protocol;
    this->ai_addrlen = addrinfo->ai_addrlen;
  }

  SocketAddress(SocketAddress &&socket_address) noexcept : ai_addr(socket_address.ai_addr) {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
  }

  SocketAddress &operator=(SocketAddress &&socket_address) noexcept {
    this->ai_family = socket_address.ai_family;
    this->ai_socktype = socket_address.ai_socktype;
    this->ai_protocol = socket_address.ai_protocol;
    this->ai_addrlen = socket_address.ai_addrlen;
    this->ai_addr = socket_address.ai_addr;
    return *this;
  }

  std::string getHost(int flags = 0) const {
    char *buf = nullptr;
    // Cannot use size_t on Linux.
    unsigned int buf_len = 20;
    buf = static_cast<char *>(malloc(buf_len * sizeof(char)));
    if (buf == nullptr) {
      throw std::bad_alloc();
    }

    int result;
    while ((result = getnameinfo(&this->ai_addr, this->ai_addrlen, buf, buf_len, NULL, 0, flags))) {
      if (result == EAI_OVERFLOW) {
        buf_len *= 2;
        buf = static_cast<char *>(realloc(buf, buf_len * sizeof(char)));
        if (buf == nullptr) {
          throw std::bad_alloc();
        }
      } else {
        throw SocketException("Cannot get name info: %s", 0, gai_strerror(result));
      }
    }
    auto host = std::string(buf);
    free(buf);
    return host;
  }

  std::string getService(int flags = 0) const {
    char *buf = nullptr;
    // Cannot use size_t on Linux.
    unsigned int buf_len = 20;
    buf = static_cast<char *>(malloc(buf_len * sizeof(char)));
    if (buf == nullptr) {
      throw std::bad_alloc();
    }

    int result;
    while ((result = getnameinfo(&this->ai_addr, this->ai_addrlen, NULL, 0, buf, buf_len, flags))) {
      if (result == EAI_OVERFLOW) {
        buf_len *= 2;
        buf = static_cast<char *>(realloc(buf, buf_len * sizeof(char)));
        if (buf == nullptr) {
          throw std::bad_alloc();
        }
      } else {
        throw SocketException("Cannot get name info: %s", 0, gai_strerror(result));
      }
    }
    auto host = std::string(buf);
    free(buf);
    return host;
  }

  operator std::string() const {
    return this->getHost() + ":" + this->getService(NI_NUMERICSERV);
  }
};

/**
 * Wrapper for BSD sockets on Unix and Windows systems.
 */
class Socket {
  socket_handle_t handle_;
  SocketAddress address_;
  pollfd pollfd_{
    INVALID_SOCKET_HANDLE,
    0,
    0
  };

protected:

public:
  /**
   * Creates a socket given an address and initializes the socket handler.
   * @param address The address is moved to prevent modifications outside the wrapper
   * @throw SocketException Thrown if the socket handler could not be created
   */
  Socket(SocketAddress &&address) : address_(std::move(address)) {
    this->handle_ =
      ::socket(address.ai_family, address.ai_socktype, address.ai_protocol);
    this->pollfd_.fd = this->handle_;
    if (this->isInvalid()) {
      throw SocketException("Invalid address", INVALID_SOCKET_HANDLE);
    }
  }

  /**
   * Creates a socket with a valid socket handler and an address.
   * @param handle The socket handler to use
   * @param address The address is moved to prevent modifications outside the wrapper
   * @throw SocketException Thrown if the handler is invalid
   */
  Socket(socket_handle_t handle, SocketAddress &&address) : address_(std::move(address)) {
    this->handle_ = handle;
    this->pollfd_.fd = this->handle_;
    if (this->isInvalid()) {
      throw SocketException("Invalid handle", INVALID_SOCKET_HANDLE);
    }
  }

  /**
   * Prevents the copy of a wrapper instance.
   */
  Socket(const Socket &socket) = delete;

  Socket(Socket &&socket) noexcept : address_(std::move(socket.address_)) {
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
    this->pollfd_ = socket.pollfd_;
    socket.pollfd_.fd = INVALID_SOCKET_HANDLE;
  }

  /**
   * Closes the socket before destructing the wrapper.
   */
  ~Socket() {
    this->close();
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
    this->pollfd_ = socket.pollfd_;
    socket.pollfd_.fd = INVALID_SOCKET_HANDLE;
    return *this;
  }

  /**
   * @return The address associated with the socket
   */
  const SocketAddress &getAddress() const {
    return this->address_;
  }

  /**
   * Tests if the socket is invalid.
   * @return The result of the test
   */
  bool isInvalid() const {
    return this->handle_ == INVALID_SOCKET_HANDLE;
  }

  /**
   * Retrieves a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be returned
   * @return The value of the socket option
   * @throw SocketException Thrown if the operation failed
   * @see ::getsockopt
   */
  template<typename T>
  const T *getsockopt(int level, int option_name) const {
    auto option_value = new T;
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    result_t result = ::getsockopt(this->handle_, level, option_name,
                                   reinterpret_cast<char *>(option_value),
                                   reinterpret_cast<socklen_t *>(&option_len));
    if (result != 0) {
      SocketException::throwFromFailure(result, "GETSOCKOPT");
    }
    return option_value;
  }

  /**
   * Sets a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be set
   * @param option_value A pointer to the value for the requested option
   * @throw SocketException Thrown if the operation failed
   * @see ::setsockopt
   */
  template<typename T>
  void setsockopt(int level, int option_name, T *option_value) {
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    result_t result = ::setsockopt(this->handle_, level, option_name,
                                   reinterpret_cast<char *>(option_value), option_len);
    if (result != 0) {
      SocketException::throwFromFailure(result, "SETSOCKOPT");
    }
  }

  /**
   * Sets a socket option.
   * @tparam T The type of the option
   * @param level The level at which the option is defined
   * @param option_name The socket option for which the value is to be set
   * @param option_value The value for the requested option
   * @throw SocketException Thrown if the operation failed
   * @see ::setsockopt
   */
  template<typename T>
  void setsockopt(int level, int option_name, T option_value) {
    auto option_len = sizeof(T);
    // Option value reinterpreted for WinSock.
    result_t result = ::setsockopt(this->handle_, level, option_name,
                                   reinterpret_cast<char *>(&option_value), option_len);
    if (result != 0) {
      SocketException::throwFromFailure(result, "SETSOCKOPT");
    }
  }

  /**
   * Binds the socket handler to the configured address.
   * @throw SocketException Thrown if the binding failed
   * @see ::bind
   */
  void bind() {
    result_t result = ::bind(this->handle_, &this->address_.ai_addr,
                             this->address_.ai_addrlen);
    if (result != 0) {
      SocketException::throwFromFailure(result, "BIND");
    }
  }

  /**
   * Connects the socket handler to the configured address.
   * @throw SocketException Thrown if the connection failed
   * @see ::connect
   */
  void connect() {
    result_t result = ::connect(this->handle_, &this->address_.ai_addr,
                                this->address_.ai_addrlen);
    if (result != 0) {
      SocketException::throwFromFailure(result, "CONNECT");
    }
  }

  /**
   *
   * @param max
   * @throw SocketException
   * @see ::listen
   */
  void listen(int max = SOMAXCONN) {
    result_t result = ::listen(this->handle_, max);
    if (result != 0) {
      SocketException::throwFromFailure(result, "LISTEN");
    }
  }

  /**
   *
   * @return
   * @throw SocketExceptions
   * @see ::accept
   */
  Socket *accept() {
    socklen_t client_sock_address_len = sizeof(sockaddr_storage);
    auto client_sock_address =
      reinterpret_cast<sockaddr *>(new sockaddr_storage);
    memset(client_sock_address, 0, static_cast<size_t>(client_sock_address_len));

    socket_handle_t client_socket =
      ::accept(this->handle_, client_sock_address, &client_sock_address_len);
    if (client_socket == INVALID_SOCKET_HANDLE) {
      SocketException::throwFromFailure(static_cast<result_t>(client_socket), "ACCEPT");
    }
    auto socket_address =
      SocketAddress(client_sock_address, client_sock_address_len);
    delete client_sock_address;
    return new Socket(client_socket, std::move(socket_address));
  }

  void setPollFdEvents(short int events = 0) {
    this->pollfd_.events = events;
  }

  pollfd getPollFd(short int events) {
    this->setPollFdEvents(events);
    return this->pollfd_;
  }

  pollfd getPollFd() const {
    return this->pollfd_;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @return
   * @throw SocketException
   * @see ::recv
   */
  result_t recv(char *buf, size_t len, int flags = 0) {
    result_t result = ::recv(this->handle_, buf, len, flags);
    if (result < 0) {
      SocketException::throwFromFailure(result, "RECVFROM");
    }
    return result;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @param address
   * @return
   * @throw SocketException
   * @see ::recvfrom
   */
  result_t recvfrom(char *buf, size_t len, int flags, SocketAddress &address) {
    result_t result = ::recvfrom(this->handle_, buf, len, flags, &address.ai_addr,
                                 &address.ai_addrlen);
    if (result < 0) {
      SocketException::throwFromFailure(result, "RECVFROM");
    }
    return result;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @return
   * @throw SocketException
   * @see ::send
   */
  result_t send(const char *buf, size_t len, int flags = 0) {
    result_t result = ::send(this->handle_, buf, len, flags);
    if (result < 0) {
      SocketException::throwFromFailure(result, "SEND");
    }
    return result;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @param address
   * @return
   * @throw SocketException
   * @see ::sendto
   */
  result_t sendto(const char *buf, size_t len, int flags,
                  const SocketAddress &address) {
    result_t result = ::sendto(this->handle_, buf, len, flags, &address.ai_addr,
                               address.ai_addrlen);
    if (result < 0) {
      SocketException::throwFromFailure(result, "SENDTO");
    }
    return result;
  }

  /**
   *
   * @param how
   * @throw SocketException
   * @see ::shutdown
   */
  void shutdown(int how) {
    result_t result = ::shutdown(this->handle_, how);
    if (result != 0) {
      SocketException::throwFromFailure(result, "SHUTDOWN");
    }
  }

  /**
   * Closes the socket handler.
   * @see ::close
   */
  void close();
};

class SocketFactory {
protected:
  static addrinfo *getAddrinfo(int ai_family, int ai_socktype, int ai_protocol, int ai_flags,
                               const char *name,
                               const char *service) {
    addrinfo hints{}, *info = nullptr;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = ai_family;
    hints.ai_socktype = ai_socktype;
    hints.ai_protocol = ai_protocol;
    hints.ai_flags = ai_flags;
    result_t result = getaddrinfo(name, service, &hints, &info) != 0;
    if (result != 0) {
      throw SocketException("Cannot get address info: %s", result,
                            gai_strerror(static_cast<int>(result)));
    }
    return info;
  }

public:
  static Socket *boundSocket(int ai_family, int ai_socktype, int ai_protocol, const char *name,
                             const char *service) {
    auto info = SocketFactory::getAddrinfo(ai_family, ai_socktype, ai_protocol, AI_PASSIVE, name,
                                           service);

    Socket *sock = nullptr;
    SocketException *error = nullptr;
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      try {
        sock = new Socket(addr);
        sock->bind();
      } catch (SocketException &e) {
        error = &e;
        delete sock;
        sock = nullptr;
        continue;
      }
      break;
    }
    freeaddrinfo(info);
    if (sock == nullptr) {
      std::string message;
      result_t result = 0;
      if (error != nullptr) {
        message += ": ";
        message += error->what();
        result = error->getResult();
      }
      throw SocketException("Cannot create bound socket%s", result, message.c_str());
    }
    return sock;
  }

  static Socket *connectedSocket(int ai_socktype, int ai_protocol, const char *name,
                                 const char *service) {
    auto info = SocketFactory::getAddrinfo(AF_UNSPEC, ai_socktype, ai_protocol, AI_PASSIVE, name,
                                           service);

    Socket *sock = nullptr;
    SocketException *error = nullptr;
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      try {
        sock = new Socket(addr);
        sock->connect();
      } catch (SocketException &e) {
        error = &e;
        delete sock;
        sock = nullptr;
        continue;
      }
      break;
    }
    freeaddrinfo(info);
    if (sock == nullptr) {
      std::string message;
      result_t result = 0;
      if (error != nullptr) {
        message += ": ";
        message += error->what();
        result = error->getResult();
      }
      throw SocketException("Cannot create connected socket%s", result, message.c_str());
    }
    return sock;
  }
};

class SocketInitializer {
public:
  SocketInitializer();
  ~SocketInitializer();
};
} // namespace net

#endif // NET_SOCKETS_H
