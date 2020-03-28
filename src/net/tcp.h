#ifndef NET_TCP_H
#define NET_TCP_H

#include "sockets.h"
#include "../utils/exception.h"
#include <map>
#include <mutex>

using namespace std;

namespace net {
/**
 * A class responsible of processing a specific client during its lifespan.
 * An instance of this class can be associated with a new client with the
 * connected/shutdown/disconnected events.
 * This default class is intended to be extended. Override TCPServer::makeClientEventsListener() to
 * use a custom listener.
 */
class ClientEventsListener {
public:
  virtual ~ClientEventsListener() = default;

  /**
   * The client has been associated with this instance.
   */
  virtual unique_ptr<Socket> &&connected(unique_ptr<Socket> &&client) {
    return move(client);
  }

  /**
   * The client has sent data.
   */
  virtual unique_ptr<Socket> &&dataAvailable(unique_ptr<Socket> &&client) {
    char buf[128];
    // Empties the TCP buffer.
    while ((client->recv(buf, 128)) > 0) {
    }
    return move(client);
  }

  /**
   * The client won't send anymore data. It might be completely disconnected and socket may be
   * invalid/broken.
   */
  virtual unique_ptr<Socket> &&shutdown(unique_ptr<Socket> &&client) {
    return move(client);
  }
};

/**
 * Abstract TCP server class.
 */
class TCPServer {
protected:
  /**
   * Creates a new client events listener. Used to override the default type.
   */
  virtual unique_ptr<ClientEventsListener> makeClientEventsListener() {
    return make_unique<ClientEventsListener>();
  }

  typedef socket_handle_t client_id_t;
  unique_ptr<Socket> socket_;
  map<client_id_t, utils::UniqueLocker<Socket>> clients_;
  map<client_id_t, unique_ptr<ClientEventsListener>> client_events_listeners_;
  mutex clients_lock_;
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
   * Adds a client to the server's list. Creates an associated client events listener if necessary.
   * @param client The client's socket
   */
  void addClient(unique_ptr<Socket> &&client) {
    auto id = client->getHandle();
    if (this->client_events_listeners_.count(id) == 0) {
      this->client_events_listeners_.emplace(id, this->makeClientEventsListener());
    }
    client = this->client_events_listeners_.at(id)->connected(move(client));
    this->clients_lock_.lock();
    this->clients_[id] = move(client);
    this->clients_lock_.unlock();
  }

  /**
   * Notifies the events listener associated with the client.
   * @param id The ID of the client
   * @param shutdown The client won't send data anymore
   * @return Whether if the client is ready to reprocessed
   */
  bool processClient(client_id_t id, bool shutdown) {
    this->clients_lock_.lock();
    auto client = this->clients_[id].try_take();
    this->clients_lock_.unlock();
    // Client has been taken by another thread.
    if (!client) {
      return false;
    }

    client = this->client_events_listeners_.at(id)->dataAvailable(move(client));
    if (client->isInvalid()) {
      shutdown = true;
    }
    if (shutdown) {
      this->client_events_listeners_.at(id)->shutdown(move(client));
    }

    this->clients_lock_.lock();
    if (shutdown) {
      this->clients_[id].reset();
    } else {
      this->clients_[id].yield(move(client));
    }
    this->clients_lock_.unlock();
    return !shutdown;
  }

public:
  /**
   * Creates a server given a socket.
   * @param socket The socket for the server
   */
  explicit TCPServer(unique_ptr<Socket> &&socket);
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
