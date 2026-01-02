#pragma once

#include <string>
#include <utility>

namespace teslasynth::helpers {
template <typename T, typename E> class Result {
  bool ok_;
  union {
    T value_;
    E error_;
  };

public:
  // --- Constructors ---
  constexpr Result(const T &v) : ok_(true), value_(v) {}
  constexpr Result(T &&v) : ok_(true), value_(std::move(v)) {}

  constexpr Result(const E &e) : ok_(false), error_(e) {}
  constexpr Result(E &&e) : ok_(false), error_(std::move(e)) {}

  // --- Destructor ---
  ~Result() {
    if (ok_)
      value_.~T();
    else
      error_.~E();
  }

  // --- Observers ---
  constexpr bool ok() const { return ok_; }
  constexpr explicit operator bool() const { return ok_; }

  constexpr const T &value() const { return value_; }
  constexpr T &value() { return value_; }

  constexpr const E &error() const { return error_; }
  constexpr E &error() { return error_; }

  // --- Monadic bind ---
  template <typename F> constexpr auto bind(F &&f) const {
    using R = decltype(f(std::declval<const T &>()));
    if (ok_)
      return f(value_);
    return R(error_);
  }

  // --- Map (covariant) ---
  template <typename F> constexpr auto map(F &&f) const {
    using U = decltype(f(std::declval<const T &>()));
    if (ok_)
      return Result<U, E>(f(value_));
    return Result<U, E>(error_);
  }

  // --- MapError (contravariant) ---
  template <typename F> constexpr auto map_error(F &&f) const {
    using E2 = decltype(f(std::declval<const E &>()));
    if (ok_)
      return Result<T, E2>(value_);
    return Result<T, E2>(f(error_));
  }
};

template <typename T> Result<T, std::string> Ok(T &&v) {
  return Result<T, std::string>(std::forward<T>(v));
}

template <typename E> Result<void, E> Err(E &&e) {
  return Result<void, E>(std::forward<E>(e));
}

#define TRY(expr)                                                              \
  ({                                                                           \
    auto _r = (expr);                                                          \
    if (!_r)                                                                   \
      return _r.error();                                                       \
    _r.value();                                                                \
  })

} // namespace teslasynth::helpers
