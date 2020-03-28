#include "http/exceptions.h"
#include "http/server.h"
#include "http/application.h"
#include "http/messages.h"

using namespace std;

class ErrorHandler : public http::Middleware {
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     const http::RequestHandler &handler) const override {
    try {
      return handler.handle(request);
    } catch (http::HTTPException &e) {
      auto response = make_unique<http::Response>(e.getStatus());
      response->getBody() << e.what();
      return response;
    } catch (...) {
      return make_unique<http::Response>(http::Response::Status::INTERNAL_SERVER_ERROR);
    }
  }
};

class Hello : public http::Middleware {
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     const http::RequestHandler &handler) const override {
    auto response = make_unique<http::Response>();
    response->getBody() << "Hello" << endl;
    return response;
  }
};

int main() {
  net::SocketInitializer socket_initializer;

  auto server = http::HTTPServer::with(AF_INET, nullptr, "8080");
  server->addMiddleware(make_unique<ErrorHandler>());
  server->addMiddleware(make_unique<Hello>());
  server->initialize();
  server->run();
}
