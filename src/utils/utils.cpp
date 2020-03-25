#include "utils.h"
#include <algorithm>

constexpr auto WHITESPACE = " \n\r\t\f\v";

void utils::split(const string &str, char delimiter, vector<string> &out) {
  size_t start;
  size_t end = 0;

  while ((start = str.find_first_not_of(delimiter, end)) != string::npos) {
    end = str.find(delimiter, start);
    out.push_back(str.substr(start, end - start));
  }
}

vector<string> utils::split(const string &str, char delimiter) {
  vector<string> out;
  split(str, delimiter, out);
  return out;
}

void utils::tolower(const string &str, string &out) {
  transform(str.begin(), str.end(), out.begin(), ::tolower);
}

string utils::tolower(const string &str) {
  string out;
  out.resize(str.size());
  tolower(str, out);
  return out;
}

void utils::trim(const string &str, string &out) {
  auto start = str.find_first_not_of(WHITESPACE);
  auto end = str.find_last_not_of(WHITESPACE);
  out = str.substr(start, end - start + 1);
}

string utils::trim(const string &str) {
  string out;
  trim(str, out);
  return out;
}
