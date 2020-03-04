#ifndef _HTTP_H_
#define _HTTP_H_

#include "../net/tcp.h"

namespace http {
class HTTPServer : public net::TCPServer {
public:
  explicit HTTPServer(std::unique_ptr<net::Socket> &&socket) : TCPServer(std::move(socket)) {
  }

  std::unique_ptr<net::Socket> &&processClient(std::unique_ptr<net::Socket> &&client) override {
    char buf[20];
    long read = 0;
    std::cout << "Receiving" << std::endl;
    while ((read = client->recv(buf, 19)) > 0) {
    }
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

#endif //_HTTP_H_
