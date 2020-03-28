#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "application.h"
#include "messages.h"
#include "../net/tcp.h"
#include <iostream>
#include <list>

using namespace std;

namespace http {
class HTTPServer : public net::TCPServer, public RequestHandler {
protected:
  application_middleware_t middleware_;
  struct MiddlewareStatus {
    application_middleware_t::const_iterator current;
    bool process_interrupted;

    explicit MiddlewareStatus(application_middleware_t::const_iterator &&current) :
      current(move(current)), process_interrupted(false) {
    }
  };
  inline static const string MIDDLEWARE_STATUS_ATTRIBUTE = "_middleware_status";

  void resetRequestMiddlewareStatus(ServerRequest &request) const {
    request.setAttribute(MIDDLEWARE_STATUS_ATTRIBUTE,
                         make_any<MiddlewareStatus>(this->middleware_.cbegin()));
  }

  unique_ptr<net::Socket> &&sendResponse(unique_ptr<Response> response,
                                         unique_ptr<net::Socket> &&client) const {
    auto content = response->getBody().str();
    response->setHeader("Content-Length", to_string(content.length()));

    auto head = static_cast<string>(response->getProtocolVersion());
    head += " ";
    head += to_string(static_cast<int>(response->getStatus()));
    head += " ";
    head += response->getReasonPhrase();
    head += "\r\n";
    for (const auto &header : response->getHeaders()) {
      head += header.first + ":";
      auto first = true;
      for (const auto &value : header.second) {
        if (!first) {
          head += ',';
        }
        head += value;
        first = false;
      }
      head += "\r\n";
    }
    head += "\r\n";
    try {
      client->send(head.c_str(), head.length());
      client->send(content.c_str(), content.length());
    } catch (...) {
      client->close();
    }
    return move(client);
  }

public:
  explicit HTTPServer(unique_ptr<net::Socket> &&socket) : TCPServer(move(socket)) {
  }

  static unique_ptr<HTTPServer> with(int ai_family, const char *name, const char *service,
                                     bool reuse = false) {
    return make_unique<HTTPServer>(
      net::SocketFactory::boundSocket(ai_family, SOCK_STREAM, IPPROTO_TCP, name, service, true,
                                      reuse));
  }

  void addMiddleware(unique_ptr<Middleware> &&middleware) {
    this->middleware_.push_back(move(middleware));
  }

  unique_ptr<Response> handle(ServerRequest &request) const override {
    auto &middleware_status = any_cast<MiddlewareStatus &>(
      request.getAttribute(MIDDLEWARE_STATUS_ATTRIBUTE));
    auto &current_middleware = middleware_status.current;
    if (current_middleware == this->middleware_.cend()) {
      throw utils::RuntimeException("Middleware stack exhausted");
    }
    auto &middleware = *current_middleware;
    ++current_middleware;
    middleware_status.process_interrupted = false;
    auto response = middleware->process(request, *this);
    if (!response && !middleware_status.process_interrupted) {
      middleware_status.process_interrupted = true;
      --current_middleware;
    }
    return response;
  }

protected:
  class HTTPClientEventsListener : public net::ClientEventsListener {
  protected:
    HTTPServer &server_;
    ServerRequest current_request_;
    string line_;
    size_t loaded_body_size_;
    bool response_sent_;

    void resetRequestParsing() {
      this->current_request_.clear();
      this->server_.resetRequestMiddlewareStatus(this->current_request_);
      this->line_.clear();
      this->loaded_body_size_ = 0;
      this->response_sent_ = false;
    }

    int receiveLine(unique_ptr<net::Socket> &client) {
      auto line_started = !this->line_.empty();
      auto expect_lf = this->line_.back() == '\r';
      char c;
      while (client->recv(&c, 1) > 0) {
        if (c == '\r') {
          expect_lf = true;
          continue;
        }
        if (expect_lf) {
          if (c == '\n') {
            if (line_started) {
              return 0; // Reached end of line.
            }
            return 1; // Line started with CRLF.
          }
          throw utils::RuntimeException("Invalid request data");
        }
        line_started = true;
        this->line_.push_back(c);
      }
      return -1; // Line is incomplete.
    }

    void parseRequestLine() {
      auto tokens = utils::split(this->line_, ' ');
      if (tokens.size() != 3) {
        throw utils::RuntimeException("Invalid request line");
      }
      this->current_request_.setMethod(ServerRequest::Method::fromString(tokens[0]));
      this->current_request_.setUri(Uri::fromString(tokens[1]));
      this->current_request_
          .setProtocolVersion(ServerRequest::ProtocolVersion::fromString(tokens[2]));
    }

    void parseHeaderLine() {
      auto colon = this->line_.find(':');
      this->current_request_
          .setAddedHeader(utils::trim(this->line_.substr(0, colon)),
                          utils::trim(this->line_.substr(colon + 1)));
    }

  public:
    explicit HTTPClientEventsListener(HTTPServer &server) : server_(server),
                                                            loaded_body_size_(0),
                                                            response_sent_(false) {
    }

    unique_ptr<net::Socket> &&connected(unique_ptr<net::Socket> &&client) override {
      this->resetRequestParsing();
      return move(client);
    }

    unique_ptr<net::Socket> &&dataAvailable(unique_ptr<net::Socket> &&client) override {
      try {
        // Data is a line of the request's head.
        while (this->current_request_.getState() < ServerRequest::STATE::HEADERS) {
          auto line_status = this->receiveLine(client);
          // Line is not complete, nothing has changed since last middleware execution.
          if (line_status == -1) {
            return move(client);
          }
          // Line is the request line.
          if (this->current_request_.getState() == ServerRequest::STATE::INVALID) {
            // Line started with CRLF unexpectedly.
            if (line_status == 1) {
              continue; // Skip CRLF and retry.
            }
            this->parseRequestLine();
            this->current_request_.state_ = ServerRequest::STATE::REQUEST_LINE;
          } else { // Line is a header line or end of the head.
            // Line started with CRLF, end of the head.
            if (line_status == 1) {
              // Body is empty, the request is complete.
              auto content_length = this->current_request_.getContentLength();
              if (content_length == 0) {
                this->current_request_.state_ = ServerRequest::STATE::BODY;
              } else { // Body needs to be loaded.
                this->current_request_.state_ = ServerRequest::STATE::HEADERS;
              }
              break;
            }
            // No need to parse if the response is already sent.
            if (!this->response_sent_) {
              this->parseHeaderLine();
            }
          }
          this->line_.clear();
        }

        // Body needs to be loaded.
        if (this->current_request_.getState() == ServerRequest::STATE::HEADERS) {
          auto remaining_body_size =
            this->current_request_.getContentLength() - this->loaded_body_size_;
          auto buf = new char[remaining_body_size + 1];
          auto received = client->recv(buf, remaining_body_size);
          if (received < 0) {
            received = 0;
          }
          buf[received] = 0;
          this->loaded_body_size_ += received;
          remaining_body_size -= received;
          this->current_request_.getBody() << buf;
          delete[] buf;
          // Body is complete.
          if (remaining_body_size == 0) {
            this->current_request_.state_ = ServerRequest::STATE::BODY;
          }
        }
      } catch (...) {
        client = this->server_
                     .sendResponse(make_unique<Response>(Response::Status::BAD_REQUEST),
                                   move(client));
        // As parsing the request failed, the next data received from the client will be in an
        // uncertain state. It is safer to close the connection and let the client start over.
        // The event listener's state will be reset with the "connected" event.
        client->close();
        return move(client);
      }

      if (!this->response_sent_) {
        unique_ptr<Response> response;
        try {
          response = this->server_.handle(this->current_request_);
        } catch (...) {
          response = make_unique<Response>(Response::Status::INTERNAL_SERVER_ERROR);
        }
        // Unable to provide a response to the request.
        if (!response && this->current_request_.state_ == ServerRequest::STATE::BODY) {
          response = make_unique<Response>(Response::Status::INTERNAL_SERVER_ERROR);
        }
        if (response) {
          this->server_.resetRequestMiddlewareStatus(this->current_request_);
          this->response_sent_ = true;
          client = this->server_.sendResponse(move(response), move(client));
        }
      }

      // Request is complete and must have been processed.
      if (this->current_request_.getState() == ServerRequest::STATE::BODY) {
        this->resetRequestParsing();
      }
      return move(client);
    }
  };

  unique_ptr<net::ClientEventsListener> makeClientEventsListener() override {
    return make_unique<HTTPClientEventsListener>(*this);
  }
};
} // namespace http

#endif //HTTP_SERVER_H
