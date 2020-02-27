#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#include <string>
#include <locale>
#include <sstream>

namespace utils {
class wchar {
public:
#if defined(_WIN32)

  static std::string narrow(const std::wstring &input) {
    std::ostringstream stm;
    const auto &ctfacet = std::use_facet<std::ctype<wchar_t>>(stm.getloc());
    for (const auto c : input) {
      stm << ctfacet.narrow(c);
    }
    return stm.str();
  }

#endif
};
}

#endif //UTILS_UTILS_H
