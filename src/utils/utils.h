#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#include <string>
#include <locale>
#include <sstream>
#include <mutex>

#if defined(_WIN32)
/// Not defined on Win32.
#ifndef EAI_OVERFLOW
#define EAI_OVERFLOW ERROR_INSUFFICIENT_BUFFER
#endif
#endif

namespace utils {

/**
 * A shareable container that allows only one owner for the data it holds. Used to prevent threads
 * from taking ownership of the same data concurrently.
 * @tparam T The type of the data which is stored in a unique pointer
 */
template<typename T>
class UniqueLocker {
protected:
  std::mutex lock_;
  std::unique_ptr<T> data_;
public:
  /**
   * Default constructor.
   */
  UniqueLocker() {
  }

  /**
   * Constructs the locker with data.
   * @param data The data
   */
  UniqueLocker(std::unique_ptr<T> &&data) : data_(std::move(data)) {
  }

  /**
   * Move constructor.
   */
  UniqueLocker(UniqueLocker<T> &&other) noexcept {
    this->lock_ = std::move(other.lock_);
    this->data_ = std::move(other.data_);
  }

  /**
   * Assigns data to the locker.
   * @param data The data
   */
  UniqueLocker &operator=(std::unique_ptr<T> &&data) {
    this->data_ = std::move(data);
  }

  /**
   * Move assignment.
   */
  UniqueLocker &operator=(UniqueLocker<T> &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    this->lock_ = std::move(other.lock_);
    this->data_ = std::move(other.data_);
    return *this;
  }

  /**
   * Takes ownership of the data. Blocks if the data is out of the locker.
   * Behavior is undefined if called in the thread already owning the data.
   * @return The data
   */
  std::unique_ptr<T> take() {
    this->lock_.lock();
    return std::move(this->data_);
  }

  /**
   * Yields back ownership of the data. The data can be different from the original.
   * Behavior is undefined if called without ownership.
   * @param data The data
   */
  void yield(std::unique_ptr<T> &&data) {
    this->data_ = std::move(data);
    this->lock_.unlock();
  }
};

/**
 * Utility class for operating on wide characters.
 */
class wchar {
public:
#if defined(_WIN32)

  /**
   * Converts a wide string into a normal string.
   * @param input The wide string
   * @return The normal string
   */
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
