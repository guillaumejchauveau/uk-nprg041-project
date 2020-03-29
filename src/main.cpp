#include "http/exceptions.h"
#include "http/server.h"
#include "http/application.h"
#include "http/messages.h"

using namespace std;

class ErrorHandler : public http::Middleware {
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     http::RequestHandler &handler) override {
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

class Logger : public http::Middleware {
  mutex lock_;
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     http::RequestHandler &handler) override {
    auto response = handler.handle(request);
    this->lock_.lock();
    cout << request.getClientAddress() << " -> ";
    cout << request.getMethod() << " " << string(request.getUri());
    if (request.getState() < http::ServerRequest::STATE::BODY) {
      cout << " [PARTIAL]";
    }
    cout << " -> ";
    if (response) {
      cout << int(response->getStatus()) << " " << response->getReasonPhrase();
    } else {
      cout << "[NO RESPONSE]";
    }
    cout << endl;
    this->lock_.unlock();
    return response;
  }
};

class Hello : public http::Middleware {
public:
  unique_ptr<http::Response> process(http::ServerRequest &request,
                                     http::RequestHandler &handler) override {
    auto response = make_unique<http::Response>();
    response->setHeader("content-type", "text/html");
    response->getBody()
      << "<!DOCTYPE html>"
      << "<html>"
      << "<head>"
      << "<title>Hello</title>"
      << "</head>"
      << "<body>"
      << "<h1>Hello</h1>"
      << "</body>"
      << "</html>";
    return response;
  }
};

int main() {
  net::SocketInitializer socket_initializer;

  auto server = http::HTTPServer::with(AF_INET, nullptr, "8080");
  server->addMiddleware(make_unique<ErrorHandler>());
  server->addMiddleware(make_unique<Logger>());
  server->addMiddleware(make_unique<Hello>());
  server->initialize();
  server->run();
}
