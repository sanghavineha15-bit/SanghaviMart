#include "dto/Dto.h"
namespace sanghavi::sanghavimart::dto {
nlohmann::json OkEnvelope(const nlohmann::json& data) {
  return nlohmann::json{{"success", true}, {"data", data}, {"error", nullptr}};
}
nlohmann::json FailEnvelope(const std::string& code, const std::string& message) {
  return nlohmann::json{{"success", false}, {"data", nullptr},
                        {"error", {{"code", code}, {"message", message}}}};
}
}  // namespace sanghavi::sanghavimart::dto
