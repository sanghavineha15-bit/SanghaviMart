#include "exception/AppException.h"
#include <drogon/HttpResponse.h>
namespace sanghavi::sanghavimart {
drogon::HttpResponsePtr ToErrorResponse(const AppException& e) {
  Json::Value body;
  body["success"] = false; body["data"] = Json::Value::null;
  body["error"]["code"] = e.code(); body["error"]["message"] = e.message();
  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(static_cast<drogon::HttpStatusCode>(e.http_status()));
  return resp;
}
drogon::HttpResponsePtr ToErrorResponse(const std::exception& e) {
  const auto* app = dynamic_cast<const AppException*>(&e);
  if (app != nullptr) return ToErrorResponse(*app);
  Json::Value body;
  body["success"] = false; body["data"] = Json::Value::null;
  body["error"]["code"] = "INTERNAL_ERROR";
  body["error"]["message"] = "An unexpected error occurred";
  auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
  resp->setStatusCode(drogon::k500InternalServerError);
  return resp;
}
}  // namespace sanghavi::sanghavimart
