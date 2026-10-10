#pragma once
// PasswordUtil — Argon2id hashing via libsodium (crypto_pwhash_str).
// No plaintext storage. No MD5/SHA1. Hashes are never logged.
#include <string>
namespace sanghavi::sanghavimart {
class PasswordUtil {
 public:
  PasswordUtil() = delete;
  /// Returns the encoded Argon2id hash; throws ValidationException on weak pw.
  static std::string Hash(const std::string& password);
  /// Returns true iff password matches the stored encoded hash.
  static bool Verify(const std::string& stored_hash, const std::string& password) noexcept;
};
}  // namespace sanghavi::sanghavimart
