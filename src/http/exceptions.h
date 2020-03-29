#ifndef HTTP_EXCEPTIONS_H
#define HTTP_EXCEPTIONS_H

#include "messages.h"
#include <exception>
#include <utility>

using namespace std;

namespace http {

class HTTPException : public exception {
protected:
  Response::Status status_;
  exception_ptr previous_;
  string reason_;
public:
  explicit HTTPException(Response::Status &&status) : status_(move(status)) {
  }

  HTTPException(Response::Status &&status, exception_ptr &&previous)
    : status_(move(status)), previous_(move(previous)) {
  }

  HTTPException(Response::Status &&status, const string& reason)
    : status_(move(status)), reason_(std::move(reason)) {
  }

  HTTPException(Response::Status &&status, string &&reason)
    : status_(move(status)), reason_(move(reason)) {
  }

  const Response::Status &getStatus() const {
    return this->status_;
  }

  const exception_ptr &getPrevious() const {
    return this->previous_;
  }

  const string &getReason() const {
    return this->reason_;
  }

  const char *what() const noexcept override {
    if (this->previous_) {
      try {
        rethrow_exception(this->getPrevious());
      } catch (exception &e) {
        return e.what();
      } catch (...) {
      }
    }
    if (!this->reason_.empty()) {
      return this->reason_.c_str();
    }
    return static_cast<const char *>(status_);
  }
};

class NotFoundException : public HTTPException {
public:
  NotFoundException() : HTTPException(Response::Status::NOT_FOUND) {
  }

  explicit NotFoundException(exception_ptr &&previous)
    : HTTPException(Response::Status::NOT_FOUND, move(previous)) {
  }

  explicit NotFoundException(const string &reason)
    : HTTPException(Response::Status::NOT_FOUND, reason) {
  }

  explicit NotFoundException(string &&reason)
    : HTTPException(Response::Status::NOT_FOUND, move(reason)) {
  }
};

} // namespace http

#endif //HTTP_EXCEPTIONS_H
