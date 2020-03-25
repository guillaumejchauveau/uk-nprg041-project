#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "application.h"
#include "messages.h"
#include "../net/tcp.h"
#include <iostream>
#include <list>

using namespace std;

namespace http {
class HTTPServer : public net::TCPServer, public RequestHandler {
protected:
  map<net::socket_handle_t, ServerRequest> pending_requests_;
  application_middleware_t middleware_;

  void resetCurrentMiddleware(ServerRequest &request) const {
    request.setAttribute(CURRENT_MIDDLEWARE_ATTRIBUTE,
                         make_any<application_middleware_t::const_iterator>(
                           this->middleware_.cbegin()));
  }

  unique_ptr<net::Socket> &&sendResponse(unique_ptr<Response> response,
                                         unique_ptr<net::Socket> &&client) const {
    auto content = response->getBody().str();
    response->setHeader("Content-Length", to_string(content.length()));

    auto head = static_cast<string>(response->getProtocolVersion());
    head += " ";
    head += to_string(static_cast<int>(response->getStatus()));
    head += " ";
    head += static_cast<const char *>(response->getStatus());
    head += "\r\n";
    for (const auto &header : response->getHeaders()) {
      head += header.first + ":";
      auto first = true;
      for (const auto &value : header.second) {
        if (!first) {
          head += ',';
        }
        head += value;
        first = false;
      }
      head += "\r\n";
    }
    head += "\r\n";
    client->send(head.c_str(), head.length());
    client->send(content.c_str(), content.length());
    return move(client);
  }

  void addClient(unique_ptr<net::Socket> &&client) override {
    auto id = client->getHandle();
    net::TCPServer::addClient(move(client));
    if (this->pending_requests_.count(id) == 0) {
      this->pending_requests_.emplace(id, ServerRequest());
    }
    auto &request = this->pending_requests_.at(id);
    this->resetCurrentMiddleware(request);
  }

  void removeClient(net::socket_handle_t id) override {
    net::TCPServer::removeClient(id);
    this->pending_requests_.at(id).clear();
  }

  unique_ptr<net::Socket> &&processClient(unique_ptr<net::Socket> &&client) override {
    const auto size = 128;
    char buf[size];
    long read;
    auto &request = this->pending_requests_.at(client->getHandle());
    while ((read = client->recv(buf, size - 1)) > 0) {
      buf[read] = 0;
      request.getBody() << buf;
    }
    auto response = this->handle(request);
    if (response) {
      this->resetCurrentMiddleware(request);
      return this->sendResponse(move(response), move(client));
    }
    return move(client);
  }

public:
  inline static const string CURRENT_MIDDLEWARE_ATTRIBUTE = "_current_middleware";

  explicit HTTPServer(unique_ptr<net::Socket> &&socket) : TCPServer(move(socket)) {
  }

  void addMiddleware(unique_ptr<Middleware> &&middleware) {
    this->middleware_.push_back(move(middleware));
  }

  unique_ptr<Response> handle(ServerRequest &request) const override {
    auto &current_middleware = any_cast<application_middleware_t::const_iterator &>(
      request.getAttribute(CURRENT_MIDDLEWARE_ATTRIBUTE));
    if (current_middleware == this->middleware_.cend()) {
      throw utils::RuntimeException("Middleware stack exhausted");
    }
    auto &middleware = *current_middleware;
    current_middleware++;
    return middleware->process(request, *this);
  }

  static unique_ptr<HTTPServer> with(int ai_family, const char *name, const char *service,
                                     bool reuse = false) {
    return make_unique<HTTPServer>(
      net::SocketFactory::boundSocket(ai_family, SOCK_STREAM, IPPROTO_TCP, name, service, true,
                                      reuse));
  }
};
} // namespace http

#endif //HTTP_SERVER_H
