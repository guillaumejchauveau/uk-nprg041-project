#ifndef NET_TCP_H
#define NET_TCP_H

#include "sockets.h"
#include "../utils/exception.h"
#include <map>
#include <mutex>
#include <iostream>

namespace net {
/**
 * Abstract TCP server class. Can be extended to process incoming data from one client.
 * The server keeps a list of the connected clients' sockets. Only one request from a given client
 * can be processed at the same time, even across threads.
 */
class TCPServer {
protected:
  std::unique_ptr<Socket> socket_{};
  std::map<socket_handle_t, utils::UniqueLocker<Socket>> clients_{};
  std::mutex clients_lock_;
  bool initialized_;

#if defined(_WIN32)
#else
  int epoll_fd_;
  /**
   * Maximum number of epoll events that will be processed by a thread at the same time.
   */
  const int MAX_EVENT = 10;
#endif

  /**
   * Adds a client the server's list.
   * @param client The client's socket
   */
  void addClient(std::unique_ptr<Socket> &&client) {
    this->clients_lock_.lock();
    auto id = client->getHandle();
    this->clients_.emplace(id, std::move(client));
    this->clients_lock_.unlock();
  }

  /**
   * Takes ownership of the client.
   * Behavior is undefined if called in the thread already owning the data.
   * @param id The identifier of the client
   * @return The client
   */
  std::unique_ptr<Socket> takeClient(socket_handle_t id) {
    this->clients_lock_.lock();
    auto &locker = this->clients_[id];
    this->clients_lock_.unlock();
    return locker.try_take();
  }

  /**
   * Yields ownership of the client.
   * Behavior is undefined if called without ownership.
   * @param client The client
   */
  void yieldClient(std::unique_ptr<Socket> &&client) {
    this->clients_lock_.lock();
    auto handle = client->getHandle();
    this->clients_[handle].yield(std::move(client));
    this->clients_lock_.unlock();
  }

  /**
   * Removes a client from the server's list.
   * Behavior is undefined if called without ownership.
   * @param id The identifier of the client
   */
  void removeClient(socket_handle_t id) {
    this->clients_lock_.lock();
    this->clients_[id].reset();
    this->clients_.erase(id);
    this->clients_lock_.unlock();
  }

  /**
   * Method called by the server when a client makes a request.
   * @param client The client who made the request
   * @return The client. Must be returned to yield ownership
   */
  virtual std::unique_ptr<Socket> &&processClient(std::unique_ptr<Socket> &&client) = 0;

public:
  /**
   * Creates a server given a socket.
   * @param socket The socket for the server
   */
  explicit TCPServer(std::unique_ptr<Socket> &&socket);
  /**
   * Prevents copy of the server.
   */
  TCPServer(const TCPServer &tcp_server) = delete;
  TCPServer(TCPServer &&tcp_server) noexcept;
  /**
   * Prevents copy of the server.
   */
  TCPServer &operator=(const TCPServer &tcp_server) = delete;
  TCPServer &operator=(TCPServer &&tcp_server) noexcept;
  virtual ~TCPServer() = default;

  /**
   * Initializes the server.
   * @param max Maximum number of pending connection requests to the socket
   */
  void initialize(int max = SOMAXCONN);
  /**
   * Starts processing requests.
   * Can be invoked with a thread.
   */
  void run();
};
} // namespace net

#endif //NET_TCP_H
