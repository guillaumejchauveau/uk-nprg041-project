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
 * Utility for unique pointers.
 */
struct LocalFreeHelper {
  void operator()(void *to_free) const {
    ::LocalFree(reinterpret_cast<HLOCAL>(to_free));
  };
};
#else
#endif

/**
 * Exception with a string message.
 */
class RuntimeException : public std::runtime_error {
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
  explicit RuntimeException(const std::string &format, Args ... args) : runtime_error("") {
    auto size = static_cast<size_t>(snprintf(nullptr, 0, format.c_str(), args ...)) + 1;
    if (size <= 0) {
      throw std::runtime_error("Error during formatting.");
    }
    std::string message;
    message.resize(size);
    snprintf(&message.front(), size, format.c_str(), args ...);
    runtime_error::operator=(runtime_error(message));
  }
};

/**
 * Exception class that gets its message from the error code.
 */
class SystemException : public RuntimeException {
protected:
  long int error_;

public:
  /**
   * @param error The return code of the function that failed
   */
  explicit SystemException(long int error) : RuntimeException("") {
    this->error_ = error;
  }

  /**
   * Platform-dependent function that retrieves the last error code.
   * @return The last error
   */
  static long int getLastError() {
#if defined(_WIN32)
    return ::GetLastError();
#else
    return errno;
#endif
  }

  /**
   * Creates a new instance using the last error.
   */
  static SystemException fromLastError() {
    return SystemException(SystemException::getLastError());
  }

  /**
   * @return The error related to the exception
   */
  long int getError() const {
    return this->error_;
  }

  /**
   * Allocates a string for the message corresponding to the error.
   * @return The string container
   */
  virtual std::string getMessage() const {
#if defined(_WIN32)
    std::unique_ptr<wchar_t[], LocalFreeHelper> buff;
    LPWSTR buffPtr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, static_cast<DWORD>(this->getError()),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&buffPtr), 0, nullptr);
    buff.reset(buffPtr);
    return utils::wchar::narrow(buff.get());
#else
    std::string buff;
    buff.resize(64);
    auto result = strerror_r(static_cast<int>(this->getError()), &buff.front(), buff.size());
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
 * Extension of base exception for address info errors handling on linux.
 */
class AddressInfoException : public SystemException {
protected:
  static std::mutex lock_;
public:
  /**
   * @see Exception(int)
   */
  explicit AddressInfoException(int error) : SystemException(error) {
  }

#if !defined(_WIN32) // Linux only

  std::string getMessage() const override {
    // gai_strerror is not thread safe.
    std::lock_guard<std::mutex> g(AddressInfoException::lock_);
    return gai_strerror(static_cast<int>(this->getError()));
  }

#endif
};
} // namespace utils
#endif //UTILS_EXCEPTION_H
