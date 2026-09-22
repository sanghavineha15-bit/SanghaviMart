#pragma once
#include <stdexcept>
#include <string>

namespace sanghavimart::exception {

/// Base application exception. Never exposed verbatim to clients.
class AppException : public std::runtime_error {
  public:
    explicit AppException(std::string code, std::string public_message)
        : std::runtime_error(public_message),
          code_(std::move(code)),
          public_message_(std::runtime_error::what()) {}
    const std::string& Code() const noexcept { return code_; }
    const std::string& PublicMessage() const noexcept { return public_message_; }

  private:
    std::string code_;
    std::string public_message_;
};

class ValidationException : public AppException {
  public:
    explicit ValidationException(std::string msg)
        : AppException("VALIDATION_ERROR", std::move(msg)) {}
};

class UnauthorizedException : public AppException {
  public:
    explicit UnauthorizedException(std::string msg = "Authentication required.")
        : AppException("UNAUTHORIZED", std::move(msg)) {}
};

class ForbiddenException : public AppException {
  public:
    explicit ForbiddenException(std::string msg = "Access denied.")
        : AppException("FORBIDDEN", std::move(msg)) {}
};

class NotFoundException : public AppException {
  public:
    explicit NotFoundException(std::string msg = "Resource not found.")
        : AppException("NOT_FOUND", std::move(msg)) {}
};

class ConflictException : public AppException {
  public:
    explicit ConflictException(std::string msg) : AppException("CONFLICT", std::move(msg)) {}
};

class InternalException : public AppException {
  public:
    explicit InternalException() : AppException("INTERNAL_ERROR", "Internal server error.") {}
};

}  // namespace sanghavimart::exception
