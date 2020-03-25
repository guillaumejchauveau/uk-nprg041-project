#include "http/server.h"
#include "http/application.h"

using namespace std;

int main() {
  net::SocketInitializer socket_initializer;

  auto server = http::HTTPServer::with(AF_INET, nullptr, "8080");
  server->addMiddleware(make_unique<http::middleware::ErrorHandler>());
  server->addMiddleware(make_unique<http::middleware::RequestParser>());
  server->addMiddleware(make_unique<http::middleware::Hello>());
  server->initialize();
  server->run();
}
