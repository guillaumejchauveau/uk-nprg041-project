#ifndef HTTP_HTTP_H
#define HTTP_HTTP_H

#include "messages.h"
#include "../net/tcp.h"
#include <iostream>

namespace http {
class IRequestHandler {
public:
  virtual const Response handle(const ServerRequest &request) const = 0;
};

class IMiddleware {
public:
  virtual const Response process(const ServerRequest &request,
                                 const IRequestHandler &handler) const = 0;
};

class HTTPServer : public net::TCPServer {
public:
  explicit HTTPServer(std::unique_ptr<net::Socket> &&socket) : TCPServer(std::move(socket)) {
  }

  std::unique_ptr<net::Socket> &&processClient(std::unique_ptr<net::Socket> &&client) override {
    char buf[20];
    long read = 0;
    std::cout << "Receiving" << std::endl;
    while ((read = client->recv(buf, 19)) > 0) {
      buf[read] = 0;
      std::cout << buf;
    }
    std::cout << read << std::endl;
    std::ostringstream res;
    res << "HTTP/1.1 200 OK" << std::endl;
    res << "Content-Length: 5" << std::endl;
    res << std::endl << "Hello";
    auto string = res.str();
    client->send(string.c_str(), string.size());
    std::cout << "Sent" << std::endl;
    return std::move(client);
  }

  static std::unique_ptr<HTTPServer> with(int ai_family, const char *name, const char *service,
                                          bool reuse = false) {
    return std::make_unique<HTTPServer>(
      net::SocketFactory::boundSocket(ai_family, SOCK_STREAM, IPPROTO_TCP, name, service, true,
                                      reuse));
  }
};
} // namespace http

#endif //HTTP_HTTP_H
