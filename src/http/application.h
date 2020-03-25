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
class ErrorHandler : public http::Middleware {
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

class RequestParser : public http::Middleware, public http::RequestParser {
public:
  int parseData() {
    if (this->state_ > HEADER_LINE) {
      return 1;
    }
    string line;
    int line_status;
    while (true) {
      line.clear();
      line_status = this->extractDataLine(line);
      if (line_status == -1) {
        return -1; // Line not complete.
      }
      if (this->state_ == REQUEST_LINE) {
        if (line_status == 1) { // Line started with CRLF unexpectedly.
          continue; // Skip CRLF and retry.
        }
        this->parseRequestLine(line);
        this->state_ = HEADER_LINE;
      } else { // HEADER_LINE
        if (line_status == 1) { // Line started with CRLF.
          if (this->getContentLength() == 0) {
            this->state_ = COMPLETE;
          } else {
            this->state_ = BODY;
          }
          return 1; // Body reached.
        }
        this->parseHeaderLine(line);
      }
    }
  }

  int extractDataLine(string &buffer) {
    auto line_started = false;
    auto beginning = this->data_.tellg();
    while (!this->data_.eof()) {
      auto c = static_cast<char>(this->data_.peek());
      if (c == stringstream::traits_type::eof()) {
        break;
      }
      this->data_.seekg(1, stringstream::cur);
      if (c == '\r') {
        if (static_cast<char>(this->data_.peek()) != '\n') {
          break;
        }
        this->data_.seekg(1, stringstream::cur);
        if (line_started) {
          return 0;
        }
        return 1;
      }
      line_started = true;
      buffer.push_back(c);
    }
    this->data_.seekg(beginning);
    return -1;
  }

  void parseRequestLine(const string &line) {
    auto tokens = utils::split(line, ' ');
    if (tokens.size() != 3) {
      throw utils::RuntimeException("Invalid request line");
    }
    this->setMethod(Method::fromString(tokens[0]));
    this->setUri(Uri::fromString(tokens[1]));
    this->setProtocolVersion(ProtocolVersion::fromString(tokens[2]));
  }

  void parseHeaderLine(string &line) {
    auto colon = line.find(':');
    this->setAddedHeader(utils::trim(line.substr(0, colon)), utils::trim(line.substr(colon + 1)));
  }

  unique_ptr<Response> process(ServerRequest &request,
                               const RequestHandler &handler) const override {
    request;
    return handler.handle(request);
  }
};

class Hello : public http::Middleware {
public:
  unique_ptr<Response> process(ServerRequest &request,
                               const RequestHandler &handler) const override {
    if (request.getState() < ServerRequest::STATE::BODY) {
      return nullptr;
    }
    auto response = make_unique<Response>();
    response->getBody() << "Hello" << endl;
    return response;
  }
};

}// namespace http::middleware
} // namespace http

#endif //HTTP_APPLICATION_H
