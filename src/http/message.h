#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include "uri.h"
#include <any>
#include <map>
#include <string>
#include <vector>

namespace http {
typedef std::vector<std::string> header_t;
struct Message {
public:
  const std::string getProtocolVersion() const;
  const Message withProtocolVersion(const std::string &version) const;
  const std::map<std::string, header_t> &getHeaders() const;
  bool hasHeader(const std::string &name) const;
  const header_t &getHeader(const std::string &name) const;
  const std::string getHeaderLine(const std::string &name) const;
  const Message withHeader(const std::string &name, const std::string &value) const;
  const Message withHeader(const std::string &name, const header_t &value) const;
  const Message withAddedHeader(const std::string &name, const std::string &value) const;
  const Message withAddedHeader(const std::string &name, const header_t &value) const;
  const Message withoutHeader(const std::string &name) const;
  std::ostringstream getBody() const;
  const Message withBody(std::ostringstream &body) const;
};

struct Request : public Message {
public:
  struct Method {
  public:
    enum METHOD {
      HEAD, GET, POST, PUT, PATCH, DELETE, PURGE, OPTIONS, TRACE, CONNECT
    };

    operator const char *() const {
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
      }
    }

  protected:
    METHOD value;
  };

  const std::string getRequestTarget() const;
  const Request withRequestTarget(const std::string &requestTarget) const;
  const Method getMethod() const;
  const Request withMethod(const Method &method) const;
  const Uri getUri() const;
  const Request withUri(const Uri &uri, bool preserveHost = false) const;
};

struct ServerRequest : public Request {
public:
  std::map<std::string, std::any> &getAttributes();
  std::any getAttribute(const std::string &name, std::any default_value = nullptr);
  const ServerRequest withAttribute(const std::string &name, std::any value) const;
  const ServerRequest withoutAttribute(const std::string &name) const;
};

struct Response : public Message {
public:
  struct Status {
  public:
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

    operator const char *() const {
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
      }
    }

  protected:
    STATUS value;
  };
  int getStatusCode() const;
  const Response withStatus(int code, std::string reason_phrase = "") const;
  const std::string getReasonPhrase() const;
};

} // namespace http

#endif //HTTP_MESSAGE_H
