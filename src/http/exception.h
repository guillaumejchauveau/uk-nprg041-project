#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include "message.h"
#include <exception>

namespace http {

class HTTPException : public std::exception {
protected:
  const Response::Status &status;
public:
  explicit HTTPException(const Response::Status &status) : status(status) {
  }

  const char *what() const override {
    return status;
  }
};

class NotFoundException : public HTTPException {
public:
  NotFoundException() : HTTPException(Response::Status::STATUS::NOT_FOUND) {
  }

};

} // namespace http

#endif //HTTP_EXCEPTION_H
