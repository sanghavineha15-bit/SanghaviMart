#pragma once
// SanghaviMart DTOs — request/response shapes for /api/v1 JSON endpoints.
// DTOs are separate structs from entities; UserResponseDto never carries password_hash.
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace sanghavi::sanghavimart::dto {

struct RegisterRequest { std::string name; std::string email; std::string password; std::string role; };
struct LoginRequest { std::string email; std::string password; };
struct UserResponseDto { std::int64_t id{0}; std::string name; std::string email; std::string role; };
struct ProductRequestDto {
  std::string name; std::string description; std::int64_t price_cents{0};
  std::int64_t stock_qty{0}; std::string category; std::string image_url;
};
struct ProductResponseDto {
  std::int64_t id{0}; std::int64_t seller_id{0}; std::string name; std::string description;
  std::int64_t price_cents{0}; std::int64_t stock_qty{0}; std::string category;
  std::string image_url; double avg_rating{0.0}; std::int64_t review_count{0};
};
struct CartItemDto { std::int64_t product_id{0}; std::int64_t quantity{0}; };
struct OrderResponseDto {
  std::int64_t id{0}; std::string status; std::int64_t total_amount_cents{0}; std::string created_at;
};
struct ReviewRequestDto { int rating{5}; std::string comment; };
struct ChatRequestDto { std::string message; };
struct ErrorDto { std::string code; std::string message; };

// Fixed response envelope: {success, data, error}
nlohmann::json OkEnvelope(const nlohmann::json& data);
nlohmann::json FailEnvelope(const std::string& code, const std::string& message);

}  // namespace sanghavi::sanghavimart::dto
