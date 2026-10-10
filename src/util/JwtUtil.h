#pragma once
// JwtUtil — stateless HS256 JWT (header.payload.signature, base64url).
// HMAC-SHA256 via libsodium (crypto_auth_hmacsha256). No new dependencies:
// base64url is hand-rolled, JSON via nlohmann::json.
// Why: Vercel container instances are stateless and scale to zero, so
// Drogon server-side sessions cannot be trusted across instances. A signed
// JWT in `Authorization: Bearer <t>` carries {user_id, role} verifiably.
// Drogon sessions are still set as a same-instance fast path (local dev).
#include <cstdint>
#include <string>

namespace sanghavi::sanghavimart {

struct JwtClaims {
  std::int64_t user_id{0};
  std::string role;
};

class JwtUtil {
 public:
  JwtUtil() = delete;
  /// Issues a token valid for ttl_seconds (default 12h). Secret from JWT_SECRET env.
  static std::string Issue(std::int64_t user_id, const std::string& role,
                           const std::string& secret, std::int64_t ttl_seconds = 43200);
  /// Verifies signature + expiry. Throws UnauthorizedException on any failure.
  static JwtClaims Verify(const std::string& token, const std::string& secret);
  /// Extracts the Bearer token from an Authorization header value ("Bearer <t>").
  /// Returns "" when absent/malformed (caller falls back to Drogon session).
  static std::string BearerOf(const std::string& auth_header);
  static std::string Base64UrlEncode(const std::string& raw);
  static std::string Base64UrlDecode(const std::string& enc);
};

}  // namespace sanghavi::sanghavimart
