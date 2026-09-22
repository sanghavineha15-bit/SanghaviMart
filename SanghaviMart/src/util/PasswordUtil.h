#pragma once
#include <sodium.h>
#include <string>

namespace sanghavimart::util {

/// Argon2id passwords via libsodium crypto_pwhash. No plaintext storage/logging.
class PasswordUtil {
  public:
    /// @brief Hash password with Argon2id (OPS+MEM interactive defaults).
    static std::string Hash(const std::string& password) {
        if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
        char out[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(out, password.data(), password.size(),
                              crypto_pwhash_OPSLIMIT_INTERACTIVE,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
            throw std::runtime_error("password hashing failed");
        }
        return std::string(out);
    }

    /// @brief Verify Argon2id hash. Returns false (never throws) on mismatch.
    static bool Verify(const std::string& password, const std::string& hash) noexcept {
        try {
            if (sodium_init() < 0) return false;
            return crypto_pwhash_str_verify(hash.c_str(), password.data(),
                                            password.size()) == 0;
        } catch (...) {
            return false;
        }
    }
};

}  // namespace sanghavimart::util
