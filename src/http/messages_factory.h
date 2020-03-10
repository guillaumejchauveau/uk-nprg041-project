#ifndef HTTP_MESSAGES_FACTORY_H
#define HTTP_MESSAGES_FACTORY_H

#include "messages.h"
#include "../net/sockets.h"
#include <map>
#include <memory>
#include <utility>

namespace http {
class ServerRequestFactory {
protected:
  std::map<int, std::unique_ptr<ServerRequest>> pendingRequests_;
};
} // namespace http

#endif //HTTP_MESSAGES_FACTORY_H
