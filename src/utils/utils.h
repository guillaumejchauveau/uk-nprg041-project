#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#include <string>
#include <locale>
#include <sstream>
#include <mutex>
#include <vector>

#if defined(_WIN32)
/// Not defined on Win32.
#ifndef EAI_OVERFLOW
#define EAI_OVERFLOW ERROR_INSUFFICIENT_BUFFER
#endif
#endif

using namespace std;

namespace utils {

/**
 * Splits a string into multiple sub-strings given a delimiter.
 * @param str The original string
 * @param delimiter The delimiter
 * @param out The output container
 */
void split(const string &str, char delimiter, vector<string> &out);

/**
 * Splits a string into multiple sub-strings given a delimiter.
 * @param str The original string
 * @param delimiter The delimiter
 * @return A vector containing the sub-strings
 */
vector<string> split(const string &str, char delimiter);

/**
 * Lower cases a string.
 * @param str The string to lower-case
 * @param out The output container
 */
void tolower(const string &str, string &out);

/**
 * Lower cases a string.
 * @param str The string to lower-case
 * @return A lower-cased copy
 */
string tolower(const string &str);

/**
 * Removes white-spaces at the beginning and at the end of a string.
 * @param str The string to trim
 * @param out The output container
 */
void trim(const string &str, string &out);

/**
 * Removes white-spaces at the beginning and at the end of a string.
 * @param str The string to trim
 * @return A trimmed copy
 */
string trim(const string &str);

/**
 * A shareable container that allows only one owner for the data it holds. Used to prevent threads
 * from taking ownership of the same data concurrently.
 * @tparam T The type of the data which is stored in a unique pointer
 */
template<typename T>
class UniqueLocker {
protected:
  mutex lock_;
  unique_ptr<T> data_;
public:
  /**
   * Default constructor.
   */
  UniqueLocker() = default;

  /**
   * Constructs the locker with data.
   * @param data The data
   */
  UniqueLocker(unique_ptr<T> &&data) : data_(move(data)) {
  }

  /**
   * Prevents copy.
   */
  UniqueLocker(const UniqueLocker &other) = delete;

  /**
   * Move constructor.
   * TODO: Probably unsafe in certain conditions
   */
  /*UniqueLocker(UniqueLocker<T> &&other) noexcept {
    this->lock_ = move(other.lock_);
    this->data_ = move(other.data_);
  }*/
  UniqueLocker(UniqueLocker<T> &&other) noexcept = delete;

  /**
   * Assigns data to the locker.
   * @param data The data
   */
  UniqueLocker &operator=(unique_ptr<T> &&data) {
    this->lock_.lock();
    this->data_ = move(data);
    this->lock_.unlock();
    return *this;
  }

  /**
   * Prevents copy.
   */
  UniqueLocker &operator=(const UniqueLocker<T> &other) = delete;

  /**
   * Move assignment.
   * TODO: Probably unsafe in certain conditions
   */
  /*UniqueLocker &operator=(UniqueLocker<T> &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    this->lock_ = move(other.lock_);
    this->data_ = move(other.data_);
    return *this;
  }*/
  UniqueLocker &operator=(UniqueLocker<T> &&other) noexcept = delete;

  ~UniqueLocker() = default;

  /**
   * Takes ownership of the data. Blocks if the data is out of the locker.
   * Behavior is undefined if called in the thread already owning the data.
   * @return The data
   */
  unique_ptr<T> take() {
    this->lock_.lock();
    return move(this->data_);
  }

  /**
   * Tries to take ownership of the data. An empty unique pointer is returned if
   * the data is out of the locker.
   * Behavior is undefined if called in the thread already owning the data.
   * @return The data or an empty pointer
   */
  unique_ptr<T> try_take() {
    if (!this->lock_.try_lock()) {
      return nullptr;
    }
    return move(this->data_);
  }

  /**
   * Yields back ownership of the data. The data can be different from the original.
   * Behavior is undefined if called without ownership.
   * @param data The data
   */
  void yield(unique_ptr<T> &&data) {
    this->data_ = move(data);
    this->lock_.unlock();
  }

  /**
   * Resets the data.
   * Behavior is undefined if called without ownership.
   */
  void reset() {
    this->data_.reset();
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
  static string narrow(const wstring &input) {
    ostringstream stm;
    const auto &ctfacet = use_facet<ctype<wchar_t>>(stm.getloc());
    for (const auto c : input) {
      stm << ctfacet.narrow(c);
    }
    return stm.str();
  }

#endif
};
}

#endif //UTILS_UTILS_H
