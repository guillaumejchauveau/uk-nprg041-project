#ifndef HTTP_EXCEPTIONS_H
#define HTTP_EXCEPTIONS_H

#include "messages.h"
#include <exception>

using namespace std;

namespace http {

class HTTPException : public exception {
protected:
  const Response::Status &status_;
public:
  explicit HTTPException(const Response::Status &status) : status_(status) {
  }

  [[nodiscard]]
  const char *what() const override {
    return static_cast<const char *>(status_);
  }
};

class NotFoundException : public HTTPException {
public:
  NotFoundException() : HTTPException(Response::Status::NOT_FOUND) {
  }

};

} // namespace http

#endif //HTTP_EXCEPTIONS_H
