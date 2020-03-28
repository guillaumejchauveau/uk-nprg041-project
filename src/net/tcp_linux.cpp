#include <iostream>
#include "tcp.h"
#include "sys/epoll.h"

/**
 * EPoll events:
 * - EPOLLIN: The client is available for recv.
 * - EPOLLRDHUP: The client shutdown at least its writing half of the connection, no more data will
 *    be sent to the server.
 * - EPOLLONESHOT: The client won't trigger any additional events unless manually re-armed. This
 *    prevents "thundering herd" wake-ups if the server uses multiple threads.
 */
constexpr auto TCP_CLIENT_EVENTS = (EPOLLIN | EPOLLRDHUP | EPOLLONESHOT);

namespace net {
TCPServer::TCPServer(unique_ptr<Socket> &&socket) : socket_(move(socket)) {
  this->initialized_ = false;
  this->epoll_fd_ = ::epoll_create1(0);
  if (this->epoll_fd_ == -1) {
    throw utils::SystemException::fromLastError();
  }
}

TCPServer::TCPServer(TCPServer &&tcp_server) noexcept : socket_(move(tcp_server.socket_)) {
  this->initialized_ = tcp_server.initialized_;
  this->epoll_fd_ = tcp_server.epoll_fd_;
  tcp_server.epoll_fd_ = -1;
}

TCPServer &TCPServer::operator=(TCPServer &&tcp_server) noexcept {
  if (this == &tcp_server) {
    return *this;
  }
  this->initialized_ = tcp_server.initialized_;
  this->socket_ = move(tcp_server.socket_);
  this->epoll_fd_ = tcp_server.epoll_fd_;
  tcp_server.epoll_fd_ = -1;
  return *this;
}

void TCPServer::initialize(int max) {
  if (this->initialized_) {
    throw utils::RuntimeException("Server already initialized");
  }
  this->initialized_ = true;
  this->socket_->listen(max);
  epoll_event connection{};
  connection.events = EPOLLIN;
  connection.data.fd = this->socket_->getHandle();
  if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, connection.data.fd, &connection) != 0) {
    throw utils::SystemException::fromLastError();
  }
}

/**
 * EPoll based, thread-safe TCP server. Waits for events on the server socket and connected clients
 * sockets.
 */
void TCPServer::run() {
  if (!this->initialized_) {
    throw utils::RuntimeException("Server not initialized");
  }
  epoll_event event{}, ready[TCPServer::MAX_EVENT];
  int ready_count, event_fd;
  unique_ptr<Socket> client;

  while (true) {
    ready_count = ::epoll_wait(this->epoll_fd_, ready, TCPServer::MAX_EVENT, -1);
    if (ready_count < 0) {
      throw utils::SystemException::fromLastError();
    }
    for (int i = 0; i < ready_count; i++) {
      event_fd = ready[i].data.fd;
      // Event is a new connection.
      if (event_fd == this->socket_->getHandle()) {
        client = this->socket_->accept(true);
        if (!client) {
          continue;
        }
        event.events = TCP_CLIENT_EVENTS;
        event.data.fd = client->getHandle();
        // Adds the new client to the EPoll interest list.
        if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, client->getHandle(), &event) != 0) {
          throw utils::SystemException::fromLastError();
        }
        this->addClient(move(client));
      } else { // A connected client changed state.
        auto shutdown = false;
        // Client won't send anymore data.
        if ((ready[i].events & EPOLLRDHUP) == EPOLLRDHUP) {
          if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, event_fd, nullptr) != 0) {
            throw utils::SystemException::fromLastError();
          }
          shutdown = true;
        }

        if (this->processClient(event_fd, shutdown)) {
          // Re-arms the client after EPOLLONESHOT.
          event.events = TCP_CLIENT_EVENTS;
          event.data.fd = event_fd;
          if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_MOD, event_fd, &event) != 0) {
            throw utils::SystemException::fromLastError();
          }
        }
      }
    }
  }
}

} // namespace net

#undef TCP_CLIENT_EVENTS
