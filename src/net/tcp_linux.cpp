#include <iostream>
#include "tcp.h"
#include "sys/epoll.h"

#define TCP_CLIENT_EVENTS (EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT)

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

void TCPServer::run() {
  if (!this->initialized_) {
    throw utils::RuntimeException("Server already listening");
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
      if (event_fd == this->socket_->getHandle()) { // Event is a new connection.
        client = this->socket_->accept(true);
        if (!client) {
          continue;
        }
        event.events = TCP_CLIENT_EVENTS;
        event.data.fd = client->getHandle();
        if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_ADD, client->getHandle(), &event) != 0) {
          throw utils::SystemException::fromLastError();
        }
        this->addClient(move(client));
      } else { // A connected client changed state.
        client = this->takeClient(event_fd); // Takes ownership of the client.
        if (!client) { // Client has been taken by another thread.
          continue;
        }
        client = this->processClient(move(client)); // Processes the client's data.

        if ((ready[i].events & EPOLLRDHUP) == EPOLLRDHUP) { // Client won't send anymore data.
          if (::epoll_ctl(this->epoll_fd_, EPOLL_CTL_DEL, event_fd, nullptr) != 0) {
            throw utils::SystemException::fromLastError();
          }
          this->removeClient(event_fd);
          client.reset();
          continue;
        }

        this->yieldClient(move(client));

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

} // namespace net

#undef TCP_CLIENT_EVENTS
