#pragma once

#include "cJSON.h"
#include <vector>

namespace teslasynth::app::helpers {
struct JSONParser {
  cJSON *root;

  struct JSONArray;
  struct JSON {
    cJSON *value;

    constexpr JSON(cJSON *value) : value(value) {}
    JSON get(const char *key) const {
      return JSON(cJSON_GetObjectItem(value, key));
    }
    JSONArray arr() const { return JSONArray(value); }
    bool is_obj() const { return cJSON_IsObject(value); }
    bool is_arr() const { return cJSON_IsArray(value); }
    bool is_string() const { return cJSON_IsString(value); }
    bool is_number() const { return cJSON_IsNumber(value); }
    bool is_bool() const { return cJSON_IsBool(value); }
    bool is_null() const { return cJSON_IsNull(value); }
    int number() const { return value->valueint; }
    double number_d() const { return value->valuedouble; }
    bool boolean() const { return cJSON_IsTrue(value) == cJSON_True; }
    const char *string() const { return value->valuestring; }
  };

  struct JSONArray {
    cJSON *first;

    constexpr JSONArray(cJSON *value) : first(value) {}
    struct Iterator {
      cJSON *current;
      constexpr Iterator(cJSON *ptr) : current(ptr) {}

      JSON operator*() const { return JSON(current); }
      Iterator &operator++() {
        current = current ? current->next : nullptr;
        return *this;
      }
      bool operator!=(const Iterator &other) const {
        return current != other.current;
      }
    };

    Iterator begin() const { return Iterator(first); }
    Iterator end() const { return Iterator(nullptr); }
  };

  constexpr JSONParser() : root(nullptr) {}
  constexpr JSONParser(const std::vector<char> &data)
      : root(cJSON_Parse(data.data())) {}
  constexpr JSONParser(const char *data) : root(cJSON_Parse(data)) {}
  constexpr JSON json() const { return JSON(root); }
  constexpr bool is_null() const { return root == nullptr; }

  ~JSONParser() { cJSON_Delete(root); }
};
} // namespace teslasynth::app::helpers
