#ifndef HTTP_APPLICATION_H
#define HTTP_APPLICATION_H

#include "messages.h"
#include <list>

using namespace std;

/**
 * The structure of an HTTP application is directly inspired by the PSR recommendations for PHP.
 */
namespace http {
/**
 * An individual component that processes a request and produces a response. It may throw an
 * exception based on HTTPException if request conditions prevent it from producing a response.
 */
class RequestHandler {
public:
  virtual ~RequestHandler() = default;
  /**
   * @param request The request to process
   * @return The response created
   */
  virtual unique_ptr<Response> handle(ServerRequest &request) const = 0;
};

/**
 * An individual component participating, often together with other middleware components, in the
 * processing of an incoming request and the creation of a resulting response. It may create and
 * return a response without delegating to a request handler, if sufficient conditions are met.
 */
class Middleware {
public:
  virtual ~Middleware() = default;
  /**
   * @param request The request to process
   * @param handler The handler in charge of passing the request to the next middleware
   * @return The response created
   */
  virtual unique_ptr<Response> process(ServerRequest &request,
                                       const RequestHandler &handler) const = 0;
};

typedef list<unique_ptr<Middleware>> application_middleware_t;
} // namespace http

#endif //HTTP_APPLICATION_H
