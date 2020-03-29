#ifndef HTTP_URI_H
#define HTTP_URI_H

#include <string>
#include <vector>
#include "../utils/exception.h"

using namespace std;

namespace http {
class Uri {
protected:
  string scheme_;
  string user_info_;
  string host_;
  unsigned port_;
  vector<string> path_;
  string query_;
  string fragment_;
public:
  Uri() : port_(0) {
  }

  static Uri fromString(const string &str) {
    Uri uri;
    size_t previous = 0;
    // Scheme.
    auto end = str.find("://");
    if (end != string::npos) {
      uri.setScheme(Uri::decode(str.substr(0, end - previous)));
      previous = end + 3;
    }

    // User info.
    end = str.find('@', previous);
    if (end != string::npos) {
      uri.setUserInfo(Uri::decode(str.substr(previous, end - previous)));
      previous = end + 1;
    }

    auto host_start = previous;

    previous = str.size() - 1;
    // Fragment.
    auto start = str.rfind('#');
    if (start != string::npos) {
      uri.setFragment(Uri::decode(str.substr(start + 1, previous - start)));
      previous = start - 1;
    }

    // Query.
    start = str.rfind('?', previous);
    if (start != string::npos) {
      uri.setQuery(Uri::decode(str.substr(start + 1, previous - start)));
      previous = start - 1;
    }

    // Path.
    start = str.find('/', host_start);
    if (start != string::npos) {
      uri.setPath(utils::split(Uri::decode(str.substr(start + 1, previous - start)), '/'));
      previous = start - 1;
    }

    // Port.
    start = str.rfind(':', previous);
    if (start != string::npos) {
      uri.setPort(static_cast<unsigned>(::stoul(str.substr(start + 1, previous - start))));
      previous = start - 1;
    }

    // Host.
    uri.setHost(Uri::decode(str.substr(host_start, previous - host_start + 1)));
    return uri;
  }

  const string &getScheme() const {
    return this->scheme_;
  }

  void setScheme(string &&scheme) {
    this->scheme_ = move(scheme);
  }

  const string &getUserInfo() const {
    return this->user_info_;
  }

  void setUserInfo(string &&user_info) {
    this->user_info_ = move(user_info);
  }

  const string &getHost() const {
    return this->host_;
  }

  void setHost(string &&host) {
    this->host_ = move(host);
  }

  unsigned getPort() const {
    return this->port_;
  }

  void setPort(unsigned port) {
    this->port_ = port;
  }

  const vector<string> &getPath() const {
    return this->path_;
  }

  void setPath(vector<string> &&path) {
    this->path_ = move(path);
  }

  const string &getQuery() const {
    return this->query_;
  }

  void setQuery(string &&query) {
    this->query_ = query;
  }

  const string &getFragment() const {
    return this->fragment_;
  }

  void setFragment(string &&fragment) {
    this->fragment_ = move(fragment);
  }

  bool isValid() const {
    if (!this->user_info_.empty() && this->host_.empty()) {
      return false;
    }
    if (this->port_ != 0 && this->host_.empty()) {
      return false;
    }
    return true;
  }

  operator string() const {
    if (!this->isValid()) {
      throw utils::RuntimeException("Uri is invalid");
    }

    string render;
    if (!this->scheme_.empty()) {
      render += Uri::encode(this->scheme_) + "://";
    }
    if (!this->user_info_.empty()) {
      render += Uri::encode(this->user_info_) + '@';
    }
    if (!this->host_.empty()) {
      render += Uri::encode(this->host_);
    }
    if (this->port_ != 0) {
      render += ':';
      render += to_string(this->port_);
    }
    for (const auto &segment : this->path_) {
      render += '/';
      render += Uri::encode(segment);
    }
    if (this->path_.empty()) {
      render += '/';
    }
    if (!this->query_.empty()) {
      render += '?';
      render += Uri::encode(this->query_);
    }
    if (!this->fragment_.empty()) {
      render += '#';
      render += Uri::encode(this->fragment_);
    }
    return render;
  }

  void clear() {
    this->scheme_.clear();
    this->user_info_.clear();
    this->host_.clear();
    this->port_ = 0;
    this->path_.clear();
    this->query_.clear();
    this->fragment_.clear();
  }

  static string encode(const string &str) {
    return str;
  }

  static string decode(const string &str) {
    return str;
  }
};
}

#endif //HTTP_URI_H
