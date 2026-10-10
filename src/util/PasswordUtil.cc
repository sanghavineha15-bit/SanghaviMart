#include "util/PasswordUtil.h"
#include <sodium.h>
#include <stdexcept>
#include <vector>
namespace sanghavi::sanghavimart {
std::string PasswordUtil::Hash(const std::string& password) {
  if (password.size() < 8) throw std::invalid_argument("password must be >= 8 chars");
  if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
  std::vector<char> out(crypto_pwhash_STRBYTES);
  if (crypto_pwhash_str(out.data(), password.c_str(), password.size(),
                        crypto_pwhash_OPSLIMIT_INTERACTIVE,
                        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
    throw std::runtime_error("password hashing failed (out of memory?)");
  }
  return std::string(out.data());
}
bool PasswordUtil::Verify(const std::string& stored_hash, const std::string& password) noexcept {
  if (stored_hash.empty() || password.empty()) return false;
  if (sodium_init() < 0) return false;
  return crypto_pwhash_str_verify(stored_hash.c_str(), password.c_str(), password.size()) == 0;
}
}  // namespace sanghavi::sanghavimart
