#ifndef HTTP_MESSAGES_H
#define HTTP_MESSAGES_H

#include "uri.h"
#include "../utils/exception.h"
#include <any>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>


#if defined(_WIN32)
#define DELETE_BAK DELETE
#undef DELETE
#endif

using namespace std;

namespace http {
typedef vector<string> header_value_t;
class Message {
public:
  struct ProtocolVersion {
  protected:
    unsigned major;
    unsigned minor;
  public:
    ProtocolVersion(unsigned major, unsigned minor) : major(major), minor(minor) {
    }

    explicit operator string() const {
      string version = "HTTP/";
      version += to_string(this->major);
      version += '.';
      version += to_string(this->minor);
      return version;
    }

    static ProtocolVersion fromString(const string &str) {
      auto major_begin = str.find('/') + 1;
      auto major_end = str.find('.', major_begin);
      auto major = static_cast<unsigned>(stoul(str.substr(major_begin, major_end - major_begin)));
      auto minor = static_cast<unsigned>(stoul(str.substr(major_end + 1)));
      return ProtocolVersion(major, minor);
    }
  };

  const ProtocolVersion &getProtocolVersion() const {
    return this->protocol_version_;
  }

  void setProtocolVersion(ProtocolVersion version) {
    this->protocol_version_ = version;
  }

  const map<string, header_value_t> &getHeaders() const {
    return this->headers_;
  }

  bool hasHeader(const string &name) const {
    auto l_name = utils::tolower(name);
    return this->headers_.count(l_name) > 0;
  }

  const header_value_t &getHeader(const string &name) const {
    auto l_name = utils::tolower(name);
    return this->headers_.at(l_name);
  }

  string getHeaderLine(const string &name) const {
    auto l_name = utils::tolower(name);
    ostringstream line(l_name);
    line << ':';

    auto first = true;
    for (const auto &value : this->getHeader(l_name)) {
      if (!first) {
        line << ',';
      }
      line << value;
      first = false;
    }
    return line.str();
  }

  void setAddedHeader(const string &name, string &&value) {
    auto l_name = utils::tolower(name);
    this->headers_[l_name].push_back(move(value));
  }

  void setAddedHeader(const string &name, header_value_t &&value) {
    auto l_name = utils::tolower(name);
    auto &stored_value = this->headers_[l_name];
    stored_value.reserve(stored_value.size() + value.size());
    stored_value.insert(stored_value.end(), value.begin(), value.end());
  }

  void setHeader(const string &name, string &&value) {
    auto l_name = utils::tolower(name);
    this->headers_[l_name].clear();
    this->setAddedHeader(l_name, move(value));
  }

  void setHeader(const string &name, header_value_t &&value) {
    auto l_name = utils::tolower(name);
    this->headers_[l_name] = move(value);
  }

  void unsetHeader(const string &name) {
    auto l_name = utils::tolower(name);
    this->headers_.erase(l_name);
  }

  stringstream &getBody() {
    return this->data_;
  }

  void setBody(stringstream &&body) {
    this->data_ = move(body);
  }

  size_t getContentLength() const {
    if (!this->hasHeader("Content-Length") || this->getHeader("Content-Length").empty()) {
      return 0;
    }
    return stoul(this->getHeader("Content-Length")[0]);
  }

  void clear() {
    this->protocol_version_ = {1u, 1u};
    this->headers_.clear();
    stringstream().swap(this->data_);
  }

protected:
  ProtocolVersion protocol_version_;
  map<string, header_value_t> headers_;
  stringstream data_;

  Message() : protocol_version_(1u, 1u) {
    this->data_.exceptions(stringstream::failbit);
  }

  explicit Message(ProtocolVersion protocol_version) : protocol_version_(protocol_version) {
    this->data_.exceptions(stringstream::failbit);
  }
};

class Request : public Message {
public:
  struct Method {
    enum METHOD {
      HEAD, GET, POST, PUT, PATCH, DELETE, PURGE, OPTIONS, TRACE, CONNECT
    };

    Method(const METHOD &method) {
      this->value = method;
    }

    static Method fromString(const string &str) {
      if (str == "HEAD") {
        return Method(HEAD);
      }
      if (str == "GET") {
        return Method(GET);
      }
      if (str == "POST") {
        return Method(POST);
      }
      if (str == "PUT") {
        return Method(PUT);
      }
      if (str == "PATCH") {
        return Method(PATCH);
      }
      if (str == "DELETE") {
        return Method(DELETE);
      }
      if (str == "PURGE") {
        return Method(PURGE);
      }
      if (str == "OPTIONS") {
        return Method(OPTIONS);
      }
      if (str == "TRACE") {
        return Method(TRACE);
      }
      if (str == "CONNECT") {
        return Method(CONNECT);
      }
      throw invalid_argument("Invalid input");
    }

    explicit operator const char *() const {
      switch (this->value) {
        case HEAD:
          return "HEAD";
        case GET:
          return "GET";
        case POST:
          return "POST";
        case PUT:
          return "PUT";
        case PATCH:
          return "PATCH";
        case DELETE:
          return "DELETE";
        case PURGE:
          return "PURGE";
        case OPTIONS:
          return "OPTIONS";
        case TRACE:
          return "TRACE";
        case CONNECT:
          return "CONNECT";
        default:
          throw logic_error("Unexpected method value");
      }
    }

  protected:
    METHOD value;
  };

  explicit Request(Method method) : method_(method) {
  }

  Request(Method method, ProtocolVersion protocol_version)
    : Message(protocol_version), method_(method) {
  }

  const Method &getMethod() const {
    return this->method_;
  }

  void setMethod(Method method) {
    this->method_ = method;
  }

  const Uri &getUri() const {
    return this->uri_;
  }

  void setUri(Uri &&uri, bool preserveHost = false) {
    this->uri_ = move(uri);
  }

protected:
  Method method_;
  Uri uri_;
};

class RequestParser {

};

class ServerRequest : public Request {
public:
  enum STATE {
    REQUEST_LINE,
    HEADER_LINE,
    BODY,
    COMPLETE
  };

  ServerRequest() : Request(Request::Method::GET), state_(REQUEST_LINE) {
  }

  explicit ServerRequest(Method method) : Request(method), state_(REQUEST_LINE) {
  }

  ServerRequest(Method method, ProtocolVersion protocol_version) : Request(
    method,
    protocol_version
  ), state_(REQUEST_LINE) {
  }

  STATE getState() const {
    return this->state_;
  }

  map<string, any> &getAttributes() {
    return this->attributes_;
  }

  bool hasAttribute(const string &name) const {
    return this->attributes_.count(name) > 0;
  }

  any &getAttribute(const string &name) {
    return this->attributes_[name];
  }

  void setAttribute(const string &name, any &&value) {
    this->attributes_[name] = move(value);
  }

  void unsetAttribute(const string &name) {
    this->attributes_.erase(name);
  }

  friend RequestParser;

protected:
  STATE state_;
  map<string, any> attributes_;
};

class Response : public Message {
public:
  struct Status {
    enum STATUS {
      // Informational 1xx
      CONTINUE = 100,
      SWITCHING_PROTOCOLS = 101,
      PROCESSING = 102,
      EARLY_HINTS = 103,
      // Successful 2xx
      OK = 200,
      CREATED = 201,
      ACCEPTED = 202,
      NON_AUTHORITATIVE_INFORMATION = 203,
      NO_CONTENT = 204,
      RESET_CONTENT = 205,
      PARTIAL_CONTENT = 206,
      MULTI_STATUS = 207,
      ALREADY_REPORTED = 208,
      IM_USED = 226,
      // Redirection 3xx
      MULTIPLE_CHOICES = 300,
      MOVED_PERMANENTLY = 301,
      FOUND = 302,
      SEE_OTHER = 303,
      NOT_MODIFIED = 304,
      USE_PROXY = 305,
      TEMPORARY_REDIRECT = 307,
      PERMANENT_REDIRECT = 308,
      // Client errors 4xx
      BAD_REQUEST = 400,
      UNAUTHORIZED = 401,
      PAYMENT_REQUIRED = 402,
      FORBIDDEN = 403,
      NOT_FOUND = 404,
      METHOD_NOT_ALLOWED = 405,
      NOT_ACCEPTABLE = 406,
      PROXY_AUTHENTICATION_REQUIRED = 407,
      REQUEST_TIMEOUT = 408,
      CONFLICT = 409,
      GONE = 410,
      LENGTH_REQUIRED = 411,
      PRECONDITION_FAILED = 412,
      PAYLOAD_TOO_LARGE = 413,
      URI_TOO_LONG = 414,
      UNSUPPORTED_MEDIA_TYPE = 415,
      RANGE_NOT_SATISFIABLE = 416,
      EXPECTATION_FAILED = 417,
      I_AM_A_TEAPOT = 418,
      MISDIRECTED_REQUEST = 421,
      UNPROCESSABLE_ENTITY = 422,
      LOCKED = 423,
      FAILED_DEPENDENCY = 424,
      UNORDERED_COLLECTION = 425,
      UPGRADE_REQUIRED = 426,
      PRECONDITION_REQUIRED = 428,
      TOO_MANY_REQUESTS = 429,
      REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
      CONNECTION_CLOSED_WITHOUT_RESPONSE = 444,
      UNAVAILABLE_FOR_LEGAL_REASONS = 451,
      // Server errors 5xx
      CLIENT_CLOSED_REQUEST = 499,
      INTERNAL_SERVER_ERROR = 500,
      NOT_IMPLEMENTED = 501,
      BAD_GATEWAY = 502,
      SERVICE_UNAVAILABLE = 503,
      GATEWAY_TIMEOUT = 504,
      HTTP_VERSION_NOT_SUPPORTED = 505,
      VARIANT_ALSO_NEGOTIATES = 506,
      INSUFFICIENT_STORAGE = 507,
      LOOP_DETECTED = 508,
      NOT_EXTENDED = 510,
      NETWORK_AUTHENTICATION_REQUIRED = 511,
      NETWORK_CONNECT_TIMEOUT_ERROR = 599
    };

    Status(const STATUS &status) {
      this->value = status;
    }

    operator int() const {
      return this->value;
    }

    explicit operator const char *() const {
      switch (this->value) {
        case CONTINUE:
          return "Continue";
        case SWITCHING_PROTOCOLS:
          return "Switching Protocols";
        case PROCESSING:
          return "Processing";
        case EARLY_HINTS:
          return "Early Hints";
        case OK:
          return "OK";
        case CREATED:
          return "Created";
        case ACCEPTED:
          return "Accepted";
        case NON_AUTHORITATIVE_INFORMATION:
          return "Non-Authoritative Information";
        case NO_CONTENT:
          return "No Content";
        case RESET_CONTENT:
          return "Reset Content";
        case PARTIAL_CONTENT:
          return "Partial Content";
        case MULTI_STATUS:
          return "Multi-Status";
        case ALREADY_REPORTED:
          return "Already Reported";
        case IM_USED:
          return "IM Used";
        case MULTIPLE_CHOICES:
          return "Multiple Choices";
        case MOVED_PERMANENTLY:
          return "Moved Permanently";
        case FOUND:
          return "Found";
        case SEE_OTHER:
          return "See Other";
        case NOT_MODIFIED:
          return "Not Modified";
        case USE_PROXY:
          return "Use Proxy";
        case TEMPORARY_REDIRECT:
          return "Temporary Redirect";
        case PERMANENT_REDIRECT:
          return "Permanent Redirect";
        case BAD_REQUEST:
          return "Bad Request";
        case UNAUTHORIZED:
          return "Unauthorized";
        case PAYMENT_REQUIRED:
          return "Payment Required";
        case FORBIDDEN:
          return "Forbidden";
        case NOT_FOUND:
          return "Not Found";
        case METHOD_NOT_ALLOWED:
          return "Method Not Allowed";
        case NOT_ACCEPTABLE:
          return "Not Acceptable";
        case PROXY_AUTHENTICATION_REQUIRED:
          return "Proxy Authentication Required";
        case REQUEST_TIMEOUT:
          return "Request Timeout";
        case CONFLICT:
          return "Conflict";
        case GONE:
          return "Gone";
        case LENGTH_REQUIRED:
          return "Length Required";
        case PRECONDITION_FAILED:
          return "Precondition Failed";
        case PAYLOAD_TOO_LARGE:
          return "Payload Too Large";
        case URI_TOO_LONG:
          return "URI Too Long";
        case UNSUPPORTED_MEDIA_TYPE:
          return "Unsupported Media Type";
        case RANGE_NOT_SATISFIABLE:
          return "Range Not Satisfiable";
        case EXPECTATION_FAILED:
          return "Expectation Failed";
        case I_AM_A_TEAPOT:
          return "I\'m a teapot";
        case MISDIRECTED_REQUEST:
          return "Misdirected Request";
        case UNPROCESSABLE_ENTITY:
          return "Unprocessable Entity";
        case LOCKED:
          return "Locked";
        case FAILED_DEPENDENCY:
          return "Failed Dependency";
        case UNORDERED_COLLECTION:
          return "Unordered Collection";
        case UPGRADE_REQUIRED:
          return "Upgrade Required";
        case PRECONDITION_REQUIRED:
          return "Precondition Required";
        case TOO_MANY_REQUESTS:
          return "Too Many Requests";
        case REQUEST_HEADER_FIELDS_TOO_LARGE:
          return "Request Header Fields Too Large";
        case CONNECTION_CLOSED_WITHOUT_RESPONSE:
          return "Connection Closed Without Response";
        case UNAVAILABLE_FOR_LEGAL_REASONS:
          return "Unavailable For Legal Reasons";
        case CLIENT_CLOSED_REQUEST:
          return "Client Closed Request";
        case INTERNAL_SERVER_ERROR:
          return "Internal Server Error";
        case NOT_IMPLEMENTED:
          return "Not Implemented";
        case BAD_GATEWAY:
          return "Bad Gateway";
        case SERVICE_UNAVAILABLE:
          return "Service Unavailable";
        case GATEWAY_TIMEOUT:
          return "Gateway Timeout";
        case HTTP_VERSION_NOT_SUPPORTED:
          return "HTTP Version Not Supported";
        case VARIANT_ALSO_NEGOTIATES:
          return "Variant Also Negotiates";
        case INSUFFICIENT_STORAGE:
          return "Insufficient Storage";
        case LOOP_DETECTED:
          return "Loop Detected";
        case NOT_EXTENDED:
          return "Not Extended";
        case NETWORK_AUTHENTICATION_REQUIRED:
          return "Network Authentication Required";
        case NETWORK_CONNECT_TIMEOUT_ERROR:
          return "Network Connect Timeout Error";
        default:
          throw logic_error("Unexpected status value");
      }
    }

  protected:
    STATUS value;
  };

  Response() : status_(Status::OK) {
    this->setStatus(this->status_);
  }

  explicit Response(Status status) : status_(status) {
    this->setStatus(this->status_);
  }

  Response(Status status, ProtocolVersion protocol_version) : Message(protocol_version),
                                                              status_(status) {
    this->setStatus(this->status_);
  }

  const Status &getStatus() const {
    return this->status_;
  }

  void setStatus(Status status) {
    this->status_ = status;
    this->reason_phrase_ = static_cast<const char *>(status);
  }

  void setStatus(Status status, string &&reason_phrase) {
    this->status_ = status;
    this->reason_phrase_ = move(reason_phrase);
  }

  const string &getReasonPhrase() const {
    return this->reason_phrase_;
  }

protected:
  Status status_;
  string reason_phrase_;
};

} // namespace http


#if defined(_WIN32)
#define DELETE DELETE_BAK
#undef DELETE_BAK
#endif

#endif //HTTP_MESSAGES_H
