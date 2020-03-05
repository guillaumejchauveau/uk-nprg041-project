#include "http/http.h"

int main() {
  net::SocketInitializer socket_initializer;

  auto server = http::HTTPServer::with(AF_INET, nullptr, "8080");
  server->initialize();
  server->run();
}
