#ifndef UTILS_EXCEPTION_H
#define UTILS_EXCEPTION_H

#include "utils.h"
#include <exception>
#include <stdexcept>
#include <memory>
#include <mutex>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <netdb.h>

#endif

namespace utils {
#if defined(_WIN32)
/**
 *
 */
struct LocalFreeHelper {
  void operator()(void *toFree) const {
    ::LocalFree(reinterpret_cast<HLOCAL>(toFree));
  };
};
#else
#endif

/**
 *
 */
class Exception : public std::exception {
protected:
  long int code;

public:
  /**
   * @param code The return code of the function that failed
   */
  explicit Exception(long int code) : exception() {
    this->code = code;
  }

  /**
   *
   * @return
   */
  static long int getLastFailureCode() {
#if defined(_WIN32)
    return ::GetLastError();
#else
    return errno;
#endif
  }

  /**
   *
   * @return
   */
  static Exception fromLastFailure() {
    return Exception(Exception::getLastFailureCode());
  }

  /**
   *
   * @return
   */
  long int getResult() const {
    return this->code;
  }

  /**
   *
   * @return
   */
  virtual std::string getMessage() const {
#if defined(_WIN32)
    std::unique_ptr<wchar_t[], LocalFreeHelper> buff;
    LPWSTR buffPtr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, static_cast<DWORD>(this->getResult()),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&buffPtr), 0, nullptr);
    buff.reset(buffPtr);
    return utils::wchar::narrow(buff.get());
#else
    std::string buff;
    buff.resize(64);
    auto result = strerror_r(static_cast<int>(this->getResult()), &buff.front(), buff.size());
    buff.resize(buff.find((char) 0));
    if (buff.empty()) {
      buff = result;
    }
    return buff;
#endif
  }

  /**
   * The message is allocated in a container with static and thread storage duration. This makes the
   * message remain in memory even after the execution of the function. However, calling the same
   * method on another instance in the same thread will overwrite the message. A call in a different
   * thread will not be an issue.
   */
  const char *what() const noexcept override {
    static thread_local std::string message;
    message = this->getMessage();
    return message.c_str();
  }
};

/**
 *
 */
class AddressInfoException : public Exception {
  static std::mutex lock;
public:
  /**
   *
   * @param result
   */
  explicit AddressInfoException(int result) : Exception(result) {
  }

#if !defined(_WIN32) // Linux only

  std::string getMessage() const override {
    // gai_strerror is not thread safe.
    std::lock_guard<std::mutex> g(AddressInfoException::lock);
    return gai_strerror(static_cast<int>(this->getResult()));
  }

#endif
};

/**
 *
 */
class MessageException : public std::exception {
protected:
  std::string message;
public:
  /**
   * Creates an instance using printf-style formatting.
   *
   * @tparam Args Parameter pack type for the format arguments
   * @param format The base format string
   * @param args Additional arguments for formatting
   * @see ::printf
   */
  template<typename ... Args>
  explicit MessageException(const std::string &format, Args ... args) {
    auto size = static_cast<size_t>(snprintf(nullptr, 0, format.c_str(), args ...)) + 1;
    if (size <= 0) {
      throw std::runtime_error("Error during formatting.");
    }
    this->message.resize(size);
    snprintf(&this->message.front(), size, format.c_str(), args ...);
  }

  const char *what() const noexcept override {
    return this->message.c_str();
  }
};
} // namespace utils
#endif //UTILS_EXCEPTION_H
