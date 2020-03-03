#ifndef NET_TCP_H
#define NET_TCP_H

#include "sockets.h"
#include "../utils/exception.h"
#include <map>
#include <mutex>
#include <iostream>

namespace net {

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

  void addClient(std::unique_ptr<Socket> &&client) {
    this->clients_lock_.lock();
    auto handle = client->getHandle();
    this->clients_.emplace(handle, std::move(client));
    this->clients_lock_.unlock();
  }

  std::unique_ptr<Socket> takeClient(int fd) {
    this->clients_lock_.lock();
    auto client = this->clients_[fd].take(); // TODO: Check if a thread blocked at this point will crash because of the owning thread calling removeClient().
    this->clients_lock_.unlock();
    return client;
  }

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
    this->clients_.erase(fd);
    this->clients_lock_.unlock();
  }

  virtual std::unique_ptr<Socket> &&processClient(std::unique_ptr<Socket> &&client) {
    char buf[20];
    long read = 0;
    std::cout << "Receiving" << std::endl;
    while ((read = client->recv(buf, 19)) > 0) {
    }
    std::ostringstream res;
    res << "HTTP/1.1 200 OK" << std::endl << std::endl;
    res << "Hello" << std::endl << std::endl;
    auto string = res.str();
    client->send(string.c_str(), string.size());
    std::cout << "Sent" << std::endl;
    return std::move(client);
  };

public:
  explicit TCPServer(std::unique_ptr<Socket> &&socket);

  TCPServer(const TCPServer &tcp_server) = delete;

  TCPServer(TCPServer &&tcp_server) noexcept;

  TCPServer operator=(const TCPServer &tcp_server) = delete;

  TCPServer &operator=(TCPServer &&tcp_server) noexcept;

  void listen(int max = SOMAXCONN);

  void run();

  static std::unique_ptr<TCPServer> with(int ai_family, const char *name, const char *service,
                                         bool reuse = false) {
    return std::make_unique<TCPServer>(
      SocketFactory::boundSocket(ai_family, SOCK_STREAM, IPPROTO_TCP, name, service, true, reuse));
  }
};

} // namespace net

#endif //NET_TCP_H
