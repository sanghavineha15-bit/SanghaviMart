#pragma once
#include <json/json.h>
#include <string>

namespace sanghavimart::util {

/// Standard /api/v1 envelope: {success,data,error}.
class JsonUtil {
  public:
    static Json::Value Success(Json::Value data = Json::Value(Json::objectValue)) {
        Json::Value out;
        out["success"] = true;
        out["data"] = std::move(data);
        out["error"] = Json::Value::null;
        return out;
    }
    static Json::Value Error(const std::string& code, const std::string& message) {
        Json::Value out;
        out["success"] = false;
        out["data"] = Json::Value::null;
        out["error"]["code"] = code;
        out["error"]["message"] = message;
        return out;
    }
};

}  // namespace sanghavimart::util
