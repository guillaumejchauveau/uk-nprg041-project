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

  static bool isCodeEWouldBlock(long int code);
  static bool isCodeEInProgress(long int code);

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

  /**
   * Move constructor.
   */
  Socket(Socket &&socket) noexcept : address_(std::move(socket.address_)) {
    this->handle_ = socket.handle_;
    socket.handle_ = INVALID_SOCKET_HANDLE;
    this->last_error_ = socket.last_error_;
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

  /**
   * Move assignment.
   */
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
   * @return The address associated with the socket
   */
  const std::unique_ptr<SocketAddress> &getAddress() const {
    return this->address_;
  }

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
      throw utils::Exception::fromLastFailure();
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
      throw utils::Exception::fromLastFailure();
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
      throw utils::Exception::fromLastFailure();
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
      throw utils::Exception::fromLastFailure();
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
      auto error = utils::Exception::getLastFailureCode();
      if (isCodeEInProgress(error)) {
        return;
      }
      throw utils::Exception(error);
    }
  }

  /**
   *
   * @param max
   * @throw utils::Exception
   * @see ::listen
   */
  void listen(int max = SOMAXCONN) {
    this->checkState();
    if ((::listen(this->handle_, max)) != 0) {
      throw utils::Exception::fromLastFailure();
    }
  }

  /**
   *
   * @return
   * @throw utils::Exception
   * @see ::accept
   */
  std::unique_ptr<Socket> accept(bool non_blocking_accepted = false) const;

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @return
   * @throw utils::Exception
   * @see ::recv
   */
  long int recv(char *buf, size_t len, int flags = 0) const {
    this->checkState();
    auto count = ::recv(this->handle_, buf, len, flags);
    if (count < 0) {
      auto error = utils::Exception::getLastFailureCode();
      if (isCodeEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @param address
   * @return
   * @throw utils::Exception
   * @see ::recvfrom
   */
  long int recvfrom(char *buf, size_t len, int flags, SocketAddress &address) const {
    this->checkState();
    auto count = ::recvfrom(this->handle_, buf, len, flags, &address.ai_addr,
                            &address.ai_addrlen);
    if (count < 0) {
      auto error = utils::Exception::getLastFailureCode();
      if (isCodeEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @return
   * @throw utils::Exception
   * @see ::send
   */
  long int send(const char *buf, size_t len, int flags = 0) const {
    this->checkState();
    auto count = ::send(this->handle_, buf, len, flags);
    if (count < 0) {
      auto error = utils::Exception::getLastFailureCode();
      if (isCodeEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   *
   * @param buf
   * @param len
   * @param flags
   * @param address
   * @return
   * @throw utils::Exception
   * @see ::sendto
   */
  long int sendto(const char *buf, size_t len, int flags,
                  const SocketAddress &address) const {
    this->checkState();
    auto count = ::sendto(this->handle_, buf, len, flags, &address.ai_addr,
                          address.ai_addrlen);
    if (count < 0) {
      auto error = utils::Exception::getLastFailureCode();
      if (isCodeEWouldBlock(error)) {
        return -1;
      }
      throw utils::Exception(error);
    }
    return count;
  }

  /**
   *
   * @param how
   * @throw utils::Exception
   * @see ::shutdown
   */
  void shutdown(int how) {
    this->checkState();
    if ((::shutdown(this->handle_, how)) != 0) {
      throw utils::Exception::fromLastFailure();
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
 *
 */
class SocketFactory {
protected:
  /**
   *
   * @param ai_family
   * @param ai_socktype
   * @param ai_protocol
   * @param ai_flags
   * @param name
   * @param service
   * @return
   */
  static addrinfo *getAddrinfo(int ai_family, int ai_socktype, int ai_protocol, int ai_flags,
                               const char *name,
                               const char *service) {
    addrinfo hints{}, *info = nullptr;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = ai_family;
    hints.ai_socktype = ai_socktype;
    hints.ai_protocol = ai_protocol;
    hints.ai_flags = ai_flags;
    int code = getaddrinfo(name, service, &hints, &info) != 0;
    if (code != 0) {
      throw utils::AddressInfoException(code);
    }
    return info;
  }

public:
  /**
   *
   * @param ai_family
   * @param ai_socktype
   * @param ai_protocol
   * @param name
   * @param service
   * @param non_blocking
   * @param reuse
   * @return
   */
  static std::unique_ptr<Socket> boundSocket(int ai_family, int ai_socktype, int ai_protocol,
                                             const char *name, const char *service,
                                             bool non_blocking = false, bool reuse = false) {
    auto info = SocketFactory::getAddrinfo(ai_family, ai_socktype, ai_protocol, AI_PASSIVE, name,
                                           service);

    std::unique_ptr<Socket> sock;
    std::unique_ptr<utils::Exception> error;
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      auto address = std::make_unique<SocketAddress>(addr);
      try {
        sock = std::make_unique<Socket>(std::move(address));
        if (non_blocking) {
          sock->setNonBlocking();
        }
        if (reuse) {
          sock->setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
        }
        sock->bind();
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
   *
   * @param ai_socktype
   * @param ai_protocol
   * @param name
   * @param service
   * @param non_blocking
   * @return
   */
  static std::unique_ptr<Socket> connectedSocket(int ai_socktype, int ai_protocol, const char *name,
                                                 const char *service, bool non_blocking = false) {
    auto info = SocketFactory::getAddrinfo(AF_UNSPEC, ai_socktype, ai_protocol, 0, name, service);

    std::unique_ptr<Socket> sock;
    std::unique_ptr<utils::Exception> error;
    for (auto addr = info; addr != nullptr; addr = addr->ai_next) {
      auto address = std::make_unique<SocketAddress>(addr);
      try {
        sock = std::make_unique<Socket>(std::move(address));
        if (non_blocking) {
          sock->setNonBlocking();
        }
        sock->connect();
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
 *
 */
class SocketInitializer {
protected:
#if defined(_WIN32)
  WSADATA wsadata_{};
#endif
public:
  SocketInitializer();
  ~SocketInitializer();
};
} // namespace net

#endif // NET_SOCKETS_H
