#pragma once
// SanghaviMart exception hierarchy + global error translator.
// Controllers never leak stack traces / internal paths; every error maps to the
// fixed JSON envelope {success:false, data:null, error:{code, message}}.
#include <stdexcept>
#include <string>
#include <drogon/HttpResponse.h>

namespace sanghavi::sanghavimart {

/// Base application error carrying a machine-readable code + HTTP status.
class AppException : public std::runtime_error {
 public:
  AppException(std::string code, std::string message, int http_status)
      : std::runtime_error(message), code_(std::move(code)),
        message_(std::runtime_error::what()), http_status_(http_status) {}
  [[nodiscard]] const std::string& code() const noexcept { return code_; }
  [[nodiscard]] const std::string& message() const noexcept { return message_; }
  [[nodiscard]] int http_status() const noexcept { return http_status_; }
 private:
  std::string code_; std::string message_; int http_status_;
};

class ValidationException : public AppException {
 public:
  explicit ValidationException(const std::string& msg)
      : AppException("VALIDATION_ERROR", msg, 400) {}
};
class UnauthorizedException : public AppException {
 public:
  explicit UnauthorizedException(const std::string& msg = "Authentication required")
      : AppException("UNAUTHORIZED", msg, 401) {}
};
class ForbiddenException : public AppException {
 public:
  explicit ForbiddenException(const std::string& msg = "Forbidden")
      : AppException("FORBIDDEN", msg, 403) {}
};
class NotFoundException : public AppException {
 public:
  explicit NotFoundException(const std::string& msg = "Not found")
      : AppException("NOT_FOUND", msg, 404) {}
};
class ConflictException : public AppException {
 public:
  explicit ConflictException(const std::string& msg)
      : AppException("CONFLICT", msg, 409) {}
};

/// Maps any exception to a safe JSON HttpResponse (no internal detail leaks).
drogon::HttpResponsePtr ToErrorResponse(const std::exception& e);
drogon::HttpResponsePtr ToErrorResponse(const AppException& e);

}  // namespace sanghavi::sanghavimart
