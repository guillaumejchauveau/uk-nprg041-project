#include "net/sockets.h"
#include <iostream>

int main() {
  net::SocketInitializer socket_initializer;

  auto sock = net::SocketFactory::boundSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, "8081");
  sock->listen();
  std::cout << "Server started on " << (std::string) sock->getAddress() << std::endl;
  char buf[1000];
  while (auto client = sock->accept()) {
    memset(buf, 0, 1000);
    std::cout << "New connection from " << (std::string) client->getAddress() << std::endl;
    ssize_t bytes_count = client->recv(buf, 999);
    std::cout << bytes_count << " bytes received" << std::endl;
    std::cout << buf << std::endl;
    client->send(buf, static_cast<size_t>(bytes_count));
    delete client;
  }
  delete sock;
}
