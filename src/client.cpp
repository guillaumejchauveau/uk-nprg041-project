#include <iostream>
#include "net/sockets.h"

int main() {
  net::SocketInitializer socket_initializer;

  auto sock = net::SocketFactory::connectedSocket(SOCK_STREAM, IPPROTO_TCP, "localhost", "8081");
  sock->send("ping", 4);

  char buf[1000];
  memset(buf, 0, 1000);
  auto bytes_count = sock->recv(buf, 999);
  std::cout << bytes_count << " bytes received" << std::endl;
}
