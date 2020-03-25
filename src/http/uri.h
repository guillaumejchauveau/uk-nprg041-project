#ifndef HTTP_URI_H
#define HTTP_URI_H

#include <string>
#include <vector>

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
  const string &getScheme() const {
    return this->scheme_;
  }

  void setScheme(string &&scheme) {
    this->scheme_ = move(scheme_);
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

  static Uri fromString(const string &str) {
    return Uri();
  }
};
}

#endif //HTTP_URI_H
