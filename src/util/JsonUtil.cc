#include "util/JsonUtil.h"
#include "exception/AppException.h"
namespace sanghavi::sanghavimart {
std::string JsonUtil::Str(const Json::Value& v, const char* key, bool required) {
  if (!v.isMember(key) || v[key].isNull()) {
    if (required) throw ValidationException(std::string("missing field: ") + key);
    return "";
  }
  if (!v[key].isString()) throw ValidationException(std::string("field must be string: ") + key);
  return v[key].asString();
}
std::int64_t JsonUtil::Int64(const Json::Value& v, const char* key, bool required) {
  if (!v.isMember(key) || v[key].isNull()) {
    if (required) throw ValidationException(std::string("missing field: ") + key);
    return 0;
  }
  if (!v[key].isInt64() && !v[key].isInt() && !v[key].isUInt())
    throw ValidationException(std::string("field must be integer: ") + key);
  return v[key].asInt64();
}
int JsonUtil::Int(const Json::Value& v, const char* key, bool required) {
  return static_cast<int>(Int64(v, key, required));
}
Json::Value JsonUtil::Ok(const Json::Value& data) {
  Json::Value b; b["success"] = true; b["data"] = data; b["error"] = Json::Value::null; return b;
}
Json::Value JsonUtil::Fail(const std::string& code, const std::string& message) {
  Json::Value b; b["success"] = false; b["data"] = Json::Value::null;
  b["error"]["code"] = code; b["error"]["message"] = message; return b;
}
}  // namespace sanghavi::sanghavimart
