#include "http/server.h"
#include "http/application.h"
#include "http/messages.h"

using namespace std;

class Hello : public http::Middleware {
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     const http::RequestHandler &handler) const override {
    // Do not answer if the request has not been entirely received.
    if (request.getState() < http::ServerRequest::STATE::BODY) {
      return nullptr;
    }
    auto response = make_unique<http::Response>();
    response->getBody() << "Hello" << endl;
    return response;
  }
};

int main() {
  net::SocketInitializer socket_initializer;

  auto server = http::HTTPServer::with(AF_INET, nullptr, "8080");
  server->addMiddleware(make_unique<http::middleware::ErrorHandler>());
  server->addMiddleware(make_unique<Hello>());
  server->initialize();
  server->run();
}
