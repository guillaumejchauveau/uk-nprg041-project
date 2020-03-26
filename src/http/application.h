#ifndef HTTP_APPLICATION_H
#define HTTP_APPLICATION_H

#include "messages.h"
#include <list>

using namespace std;

namespace http {
class RequestHandler {
public:
  virtual ~RequestHandler() = default;
  virtual unique_ptr<Response> handle(ServerRequest &request) const = 0;
};

class Middleware {
public:
  virtual ~Middleware() = default;
  virtual unique_ptr<Response> process(ServerRequest &request,
                                       const RequestHandler &handler) const = 0;
};
typedef list<unique_ptr<Middleware>> application_middleware_t;

namespace middleware {
class ErrorHandler : public Middleware {
public:
  unique_ptr<Response> process(ServerRequest &request,
                               const RequestHandler &handler) const override {
    try {
      return handler.handle(request);
    } catch (exception &e) {
      return make_unique<Response>(Response::Status::INTERNAL_SERVER_ERROR);
    }
  }
};
} // namespace http::middleware
} // namespace http

#endif //HTTP_APPLICATION_H
