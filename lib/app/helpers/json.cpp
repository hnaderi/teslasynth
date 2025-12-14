#include "json.hpp"
#include "cJSON.h"

namespace teslasynth::app::helpers {

JSONArrayBuilder JSONObjBuilder::add_array(const char *key) const {
  return JSONArrayBuilder(cJSON_AddArrayToObject(value, key));
}
JSONObjBuilder JSONObjBuilder::add_object(const char *key) const {
  return JSONObjBuilder(cJSON_AddObjectToObject(value, key));
}
const JSONObjBuilder &JSONObjBuilder::add(const char *key, double num) const {
  cJSON_AddNumberToObject(value, key, num);
  return *this;
}
const JSONObjBuilder &JSONObjBuilder::add(const char *key, const char *str) const {
  cJSON_AddStringToObject(value, key, str);
  return *this;
}
const JSONObjBuilder &JSONObjBuilder::add_bool(const char *key, bool v) const {
  cJSON_AddBoolToObject(value, key, v);
  return *this;
}
const JSONObjBuilder &JSONObjBuilder::add_null(const char *key) const {
  cJSON_AddNullToObject(value, key);
  return *this;
}

const JSONArrayBuilder &JSONArrayBuilder::add(int num) const {
  cJSON_AddItemToArray(array, cJSON_CreateNumber(num));
  return *this;
}
const JSONArrayBuilder &JSONArrayBuilder::add(const char *str) const {
  cJSON_AddItemToArray(array, cJSON_CreateString(str));
  return *this;
}
const JSONArrayBuilder &JSONArrayBuilder::add(bool v) const {
  cJSON_AddItemToArray(array, cJSON_CreateBool(v));
  return *this;
}
JSONObjBuilder JSONArrayBuilder::add_object() const {
  auto obj = cJSON_CreateObject();
  cJSON_AddItemToArray(array, obj);
  return JSONObjBuilder(obj);
}
JSONArrayBuilder JSONArrayBuilder::add_array() const {
  auto arr = cJSON_CreateArray();
  cJSON_AddItemToArray(array, arr);
  return JSONArrayBuilder(arr);
}

} // namespace teslasynth::app::helpers
