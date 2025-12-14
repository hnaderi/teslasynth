#pragma once

#include "cJSON.h"
#include <cassert>
#include <cstdlib>
#include <optional>

namespace teslasynth::app::helpers {
struct JSONParser {
  cJSON *root_;

  struct JSONArrayView;
  struct JSONObjectView {
    cJSON *value;

    JSONObjectView(cJSON *value) : value(value) {}
    JSONObjectView get(const char *key) const {
      return JSONObjectView(cJSON_GetObjectItem(value, key));
    }
    JSONArrayView arr() const {
      if (is_arr())
        return JSONArrayView(value->child);
      return JSONArrayView(nullptr);
    }
    std::optional<bool> boolean() const {
      if (is_bool())
        return cJSON_IsTrue(value);
      return {};
    }
    std::optional<const char *> string() const {
      if (is_string())
        return value->valuestring;
      return {};
    }
    std::optional<int> number() const {
      if (is_number())
        return value->valueint;
      return {};
    }
    std::optional<double> number_d() const {
      if (is_number())
        return value->valuedouble;
      return {};
    }

    bool is_obj() const { return cJSON_IsObject(value); }
    bool is_arr() const { return cJSON_IsArray(value); }
    bool is_string() const { return cJSON_IsString(value); }
    bool is_number() const { return cJSON_IsNumber(value); }
    bool is_bool() const { return cJSON_IsBool(value); }
    bool is_null() const { return cJSON_IsNull(value); }
  };

  struct JSONArrayView {
    cJSON *first;

    JSONArrayView(cJSON *value) : first(value) {}
    struct Iterator {
      cJSON *current;
      Iterator(cJSON *ptr) : current(ptr) {}

      JSONObjectView operator*() const { return JSONObjectView(current); }
      Iterator &operator++() noexcept {
        current = current ? current->next : nullptr;
        return *this;
      }
      bool operator!=(const Iterator &other) const noexcept {
        return current != other.current;
      }
    };

    Iterator begin() const { return Iterator(first); }
    Iterator end() const { return Iterator(nullptr); }
  };

  JSONParser() : root_(nullptr) {}
  explicit JSONParser(const char *data) : root_(cJSON_Parse(data)) {}

  JSONParser(const JSONParser &) = delete;
  JSONParser &operator=(const JSONParser &) = delete;
  JSONParser(JSONParser &&other) noexcept : root_(other.root_) {
    other.root_ = nullptr;
  }
  JSONParser &operator=(JSONParser &&other) noexcept {
    if (this != &other) {
      cJSON_Delete(root_);
      root_ = other.root_;
      other.root_ = nullptr;
    }
    return *this;
  }
  JSONObjectView root() const { return JSONObjectView(root_); }
  bool is_null() const { return root_ == nullptr; }

  ~JSONParser() { cJSON_Delete(root_); }
};

class JSONEncoder;
class JSONArrayBuilder;
class JSONObjBuilder {
  cJSON *value;
  explicit JSONObjBuilder(cJSON *v) : value(v) {}
  friend class JSONEncoder;
  friend class JSONArrayBuilder;

  JSONObjBuilder(const JSONObjBuilder &) = delete;
  JSONObjBuilder &operator=(const JSONObjBuilder &) = delete;
  JSONObjBuilder(JSONObjBuilder &&) = default;
  JSONObjBuilder &operator=(JSONObjBuilder &&) = default;

public:
  const JSONObjBuilder &add(const char *key, double num) const;
  const JSONObjBuilder &add(const char *key, const char *str) const;
  const JSONObjBuilder &add_bool(const char *key, bool v) const;
  const JSONObjBuilder &add_null(const char *key) const;
  JSONObjBuilder add_object(const char *key) const;
  JSONArrayBuilder add_array(const char *key) const;

  template <typename T>
  const JSONObjBuilder &add(const char *key, std::optional<T> opt) const {
    if (opt.has_value())
      return add(key, *opt);
    else {
      add_null(key);
      return *this;
    }
  }
};
class JSONArrayBuilder {
  cJSON *array;
  explicit JSONArrayBuilder(cJSON *v) : array(v) {}
  friend class JSONEncoder;
  friend class JSONObjBuilder;

  JSONArrayBuilder(const JSONArrayBuilder &) = delete;
  JSONArrayBuilder &operator=(const JSONArrayBuilder &) = delete;
  JSONArrayBuilder(JSONArrayBuilder &&) = default;
  JSONArrayBuilder &operator=(JSONArrayBuilder &&) = default;

public:
  const JSONArrayBuilder &add(int num) const;
  const JSONArrayBuilder &add(const char *str) const;
  const JSONArrayBuilder &add(bool v) const;
  JSONObjBuilder add_object() const;
  JSONArrayBuilder add_array() const;
};

struct PrintedJSON {
  const char *value;

  PrintedJSON(const char *str) : value(str) {}

  PrintedJSON(const PrintedJSON &) = delete;
  PrintedJSON &operator=(const PrintedJSON &) = delete;
  PrintedJSON(PrintedJSON &&other) noexcept : value(other.value) {
    other.value = nullptr;
  }
  PrintedJSON &operator=(PrintedJSON &&other) noexcept {
    if (this != &other) {
      free((void *)value);
      value = other.value;
      other.value = nullptr;
    }
    return *this;
  }

  ~PrintedJSON() {
    if (value) {
      free((void *)value);
      value = nullptr;
    }
  }
};

class JSONEncoder {
  cJSON *root;

public:
  JSONObjBuilder object() {
    assert(root == nullptr);
    auto obj = JSONObjBuilder(cJSON_CreateObject());
    root = obj.value;
    return obj;
  }
  JSONArrayBuilder array() {
    assert(root == nullptr);
    auto arr = JSONArrayBuilder(cJSON_CreateArray());
    root = arr.array;
    return arr;
  }
  JSONEncoder() : root(nullptr) {}
  JSONEncoder(const JSONEncoder &) = delete;
  JSONEncoder &operator=(const JSONEncoder &) = delete;
  JSONEncoder(JSONEncoder &&other) noexcept : root(other.root) {
    other.root = nullptr;
  }
  JSONEncoder &operator=(JSONEncoder &&other) noexcept {
    if (this != &other) {
      cJSON_Delete(root);
      root = other.root;
      other.root = nullptr;
    }
    return *this;
  }

  PrintedJSON print() && { return PrintedJSON(cJSON_Print(root)); }
  ~JSONEncoder() { cJSON_Delete(root); }
};
} // namespace teslasynth::app::helpers
