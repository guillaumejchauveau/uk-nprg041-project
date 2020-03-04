#ifndef NET_TCP_H
#define NET_TCP_H

#include "sockets.h"
#include "../utils/exception.h"
#include <map>
#include <mutex>
#include <iostream>

namespace net {
/**
 *
 */
class TCPServer {
protected:
  std::unique_ptr<Socket> socket_{};
  std::map<int, utils::UniqueLocker<Socket>> clients_{};
  std::mutex clients_lock_;
  bool listening_;

#if defined(_WIN32)
#else
  int epollfd_;
  const int MAX_EVENT = 10;
#endif

  /**
   *
   * @param client
   */
  void addClient(std::unique_ptr<Socket> &&client) {
    this->clients_lock_.lock();
    auto handle = client->getHandle();
    this->clients_.emplace(handle, std::move(client));
    this->clients_lock_.unlock();
  }

  /**
   *
   * @param fd
   * @return
   */
  std::unique_ptr<Socket> takeClient(int fd) {
    this->clients_lock_.lock();
    auto &locker = this->clients_[fd];
    this->clients_lock_.unlock();
    return locker.try_take();
  }

  /**
   *
   * @param client
   */
  void yieldClient(std::unique_ptr<Socket> &&client) {
    this->clients_lock_.lock();
    auto handle = client->getHandle();
    this->clients_[handle].yield(std::move(client));
    this->clients_lock_.unlock();
  }

  /**
   * Behavior is undefined if called without ownership.
   * @param fd
   */
  void removeClient(int fd) {
    this->clients_lock_.lock();
    this->clients_[fd].reset();
    this->clients_.erase(fd);
    this->clients_lock_.unlock();
  }

  /**
   *
   * @param client
   * @return
   */
  virtual std::unique_ptr<Socket> &&processClient(std::unique_ptr<Socket> &&client) = 0;

public:
  /**
   *
   * @param socket
   */
  explicit TCPServer(std::unique_ptr<Socket> &&socket);
  /**
   *
   * @param tcp_server
   */
  TCPServer(const TCPServer &tcp_server) = delete;
  /**
   *
   * @param tcp_server
   */
  TCPServer(TCPServer &&tcp_server) noexcept;
  /**
   *
   * @param tcp_server
   * @return
   */
  TCPServer &operator=(const TCPServer &tcp_server) = delete;
  /**
   *
   * @param tcp_server
   * @return
   */
  TCPServer &operator=(TCPServer &&tcp_server) noexcept;
  /**
   *
   */
  virtual ~TCPServer() = default;

  /**
   *
   * @param max
   */
  void listen(int max = SOMAXCONN);
  /**
   *
   */
  void run();
};
} // namespace net

#endif //NET_TCP_H
