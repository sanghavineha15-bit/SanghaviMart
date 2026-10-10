#pragma once
// JsonUtil — safe extraction helpers at the HTTP boundary (JsonCpp Json::Value).
#include <cstdint>
#include <string>
#include <json/json.h>
namespace sanghavi::sanghavimart {
class JsonUtil {
 public:
  JsonUtil() = delete;
  static std::string Str(const Json::Value& v, const char* key, bool required = true);
  static std::int64_t Int64(const Json::Value& v, const char* key, bool required = true);
  static int Int(const Json::Value& v, const char* key, bool required = true);
  static Json::Value Ok(const Json::Value& data);
  static Json::Value Fail(const std::string& code, const std::string& message);
};
}  // namespace sanghavi::sanghavimart
