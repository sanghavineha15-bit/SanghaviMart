#include "util/JwtUtil.h"
#include <sodium.h>
#include <chrono>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "exception/AppException.h"

namespace sanghavi::sanghavimart {
namespace {
std::int64_t NowSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
constexpr char kB64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
int B64Val(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '-') return 62;
  if (c == '_') return 63;
  return -1;
}
}  // namespace

std::string JwtUtil::Base64UrlEncode(const std::string& raw) {
  std::string out;
  out.reserve(((raw.size() + 2) / 3) * 4);
  for (size_t i = 0; i < raw.size(); i += 3) {
    unsigned v = static_cast<unsigned char>(raw[i]) << 16;
    int n = 1;
    if (i + 1 < raw.size()) {
      v |= static_cast<unsigned char>(raw[i + 1]) << 8;
      n = 2;
    }
    if (i + 2 < raw.size()) {
      v |= static_cast<unsigned char>(raw[i + 2]);
      n = 3;
    }
    out += kB64[(v >> 18) & 63];
    out += kB64[(v >> 12) & 63];
    if (n > 1) out += kB64[(v >> 6) & 63];
    if (n > 2) out += kB64[v & 63];
  }
  return out;  // no padding by construction
}

std::string JwtUtil::Base64UrlDecode(const std::string& enc) {
  if (enc.empty() || enc.size() % 4 == 1)
    throw UnauthorizedException("malformed token");
  std::string out;
  out.reserve((enc.size() / 4) * 3);
  for (size_t i = 0; i < enc.size(); i += 4) {
    int a = B64Val(enc[i]);
    int b = (i + 1 < enc.size()) ? B64Val(enc[i + 1]) : -1;
    int c = (i + 2 < enc.size()) ? B64Val(enc[i + 2]) : -1;
    int d = (i + 3 < enc.size()) ? B64Val(enc[i + 3]) : -1;
    if (a < 0 || b < 0) throw UnauthorizedException("malformed token");
    unsigned v = (static_cast<unsigned>(a) << 18) | (static_cast<unsigned>(b) << 12);
    out += static_cast<char>((v >> 16) & 0xFF);
    if (c >= 0) {
      v |= static_cast<unsigned>(c) << 6;
      out += static_cast<char>((v >> 8) & 0xFF);
    }
    if (d >= 0) {
      v |= static_cast<unsigned>(d);
      out += static_cast<char>(v & 0xFF);
    }
  }
  return out;
}

std::string JwtUtil::BearerOf(const std::string& h) {
  constexpr const char* kPre = "Bearer ";
  if (h.rfind(kPre, 0) == 0 && h.size() > 7) return h.substr(7);
  return "";
}

static std::string Hmac(const std::string& key, const std::string& msg) {
  if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
  unsigned char k[crypto_auth_hmacsha256_KEYBYTES] = {0};
  const size_t n = std::min(key.size(), sizeof(k));
  std::copy(key.begin(), key.begin() + static_cast<std::ptrdiff_t>(n),
            reinterpret_cast<char*>(k));
  unsigned char mac[crypto_auth_hmacsha256_BYTES];
  crypto_auth_hmacsha256(mac,
                         reinterpret_cast<const unsigned char*>(msg.data()),
                         msg.size(), k);
  sodium_memzero(k, sizeof(k));
  return std::string(reinterpret_cast<char*>(mac), sizeof(mac));
}

std::string JwtUtil::Issue(std::int64_t user_id, const std::string& role,
                           const std::string& secret,
                           std::int64_t ttl_seconds) {
  if (secret.size() < 16)
    throw std::runtime_error("JWT_SECRET must be >= 16 chars");
  const std::string header = Base64UrlEncode(R"({"alg":"HS256","typ":"JWT"})");
  nlohmann::json pay = {{"sub", user_id},
                        {"role", role},
                        {"exp", NowSec() + ttl_seconds}};
  const std::string payload = Base64UrlEncode(pay.dump());
  const std::string signing = header + "." + payload;
  return signing + "." + Base64UrlEncode(Hmac(secret, signing));
}

JwtClaims JwtUtil::Verify(const std::string& token, const std::string& secret) {
  const auto p1 = token.find('.');
  const auto p2 = (p1 == std::string::npos) ? std::string::npos
                                            : token.find('.', p1 + 1);
  if (p1 == std::string::npos || p2 == std::string::npos ||
      token.find('.', p2 + 1) != std::string::npos)
    throw UnauthorizedException("malformed token");
  const std::string signing = token.substr(0, p2);
  std::string want, got;
  try {
    want = Base64UrlDecode(token.substr(p2 + 1));
  } catch (const AppException&) {
    throw UnauthorizedException("malformed token");
  }
  got = Hmac(secret, signing);
  if (want.size() != got.size() ||
      sodium_memcmp(want.data(), got.data(), got.size()) != 0)
    throw UnauthorizedException("invalid token signature");
  nlohmann::json pay;
  try {
    pay = nlohmann::json::parse(Base64UrlDecode(signing.substr(p1 + 1)));
  } catch (...) {
    throw UnauthorizedException("malformed token");
  }
  JwtClaims c;
  c.user_id = pay.value("sub", 0);
  c.role = pay.value("role", std::string());
  if (c.user_id <= 0 || c.role.empty())
    throw UnauthorizedException("malformed token");
  if (pay.value("exp", std::int64_t{0}) < NowSec())
    throw UnauthorizedException("token expired");
  return c;
}

}  // namespace sanghavi::sanghavimart
