#include <iostream>
#include "tcp.h"
#include "sys/epoll.h"

#define TCP_CLIENT_EVENTS (EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT)

namespace net {
TCPServer::TCPServer(std::unique_ptr<Socket> &&socket) : socket_(std::move(socket)) {
  this->listening_ = false;
  this->epollfd_ = ::epoll_create1(0);
  if (this->epollfd_ == -1) {
    throw utils::Exception::fromLastError();
  }
}

TCPServer::TCPServer(TCPServer &&tcp_server) noexcept : socket_(std::move(tcp_server.socket_)) {
  this->listening_ = tcp_server.listening_;
  this->epollfd_ = tcp_server.epollfd_;
  tcp_server.epollfd_ = -1;
}

TCPServer &TCPServer::operator=(TCPServer &&tcp_server) noexcept {
  if (this == &tcp_server) {
    return *this;
  }
  this->listening_ = tcp_server.listening_;
  this->socket_ = std::move(tcp_server.socket_);
  this->epollfd_ = tcp_server.epollfd_;
  tcp_server.epollfd_ = -1;
  return *this;
}

void TCPServer::listen(int max) {
  if (this->listening_) {
    throw utils::MessageException("Server already listening");
  }
  this->listening_ = true;
  this->socket_->listen(max);
  epoll_event connection{};
  connection.events = EPOLLIN;
  connection.data.fd = this->socket_->getHandle();
  if (::epoll_ctl(this->epollfd_, EPOLL_CTL_ADD, connection.data.fd, &connection) != 0) {
    throw utils::Exception::fromLastError();
  }
}

void TCPServer::run() {
  epoll_event event{}, ready[TCPServer::MAX_EVENT];
  int ready_count, event_fd;
  std::unique_ptr<Socket> client;

  while (true) {
    ready_count = ::epoll_wait(this->epollfd_, ready, TCPServer::MAX_EVENT, -1);
    if (ready_count < 0) {
      throw utils::Exception::fromLastError();
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
        if (::epoll_ctl(this->epollfd_, EPOLL_CTL_ADD, client->getHandle(), &event) != 0) {
          throw utils::Exception::fromLastError();
        }
        this->addClient(std::move(client));
      } else { // A connected client changed state.
        client = this->takeClient(event_fd); // Takes ownership of the client.
        if (!client) { // Client has been taken by another thread.
          continue;
        }
        client = this->processClient(std::move(client)); // Processes the client's data.

        if ((ready[i].events & EPOLLRDHUP) == EPOLLRDHUP) { // Client has disconnected.
          if (::epoll_ctl(this->epollfd_, EPOLL_CTL_DEL, event_fd, nullptr) != 0) {
            throw utils::Exception::fromLastError();
          }
          this->removeClient(event_fd);
          client.reset();
          continue;
        }

        this->yieldClient(std::move(client));

        // Re-arms the client after EPOLLONESHOT.
        event.events = TCP_CLIENT_EVENTS;
        event.data.fd = event_fd;
        if (::epoll_ctl(this->epollfd_, EPOLL_CTL_MOD, event_fd, &event) != 0) {
          throw utils::Exception::fromLastError();
        }
      }
    }
  }
}

} // namespace net
