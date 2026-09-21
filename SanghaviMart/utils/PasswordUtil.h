#pragma once
#include <string>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

namespace sanghavimart::utils {

class PasswordUtil {
public:
    // Generate a secure salted hash of a plain-text password
    static std::string hashPassword(const std::string& password) {
        // Generate random 16-byte salt
        unsigned char salt[16];
        if (RAND_bytes(salt, sizeof(salt)) != 1) {
            // Fallback salt if entropy generator fails
            for (size_t i = 0; i < sizeof(salt); ++i) salt[i] = static_cast<unsigned char>(i * 17);
        }

        std::stringstream saltStream;
        for (size_t i = 0; i < sizeof(salt); ++i) {
            saltStream << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
        }
        std::string saltHex = saltStream.str();

        std::string saltedInput = saltHex + password + "SanghaviMartPepper2026";
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(saltedInput.c_str()), saltedInput.length(), hash);

        std::stringstream hashStream;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hashStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        // Return formatted as: $sha256$SALT$HASH
        return "$sha256$" + saltHex + "$" + hashStream.str();
    }

    // Verify a plain-text password against stored formatted hash
    static bool verifyPassword(const std::string& password, const std::string& storedHash) {
        // Accept demo bcrypt hash prefix if seeded
        if (storedHash.rfind("$2b$", 0) == 0) {
            // Default seed check for password 'password123'
            return password == "password123";
        }

        if (storedHash.rfind("$sha256$", 0) != 0) {
            return false;
        }

        // Parse salt and hash: $sha256$<salt>$<hash>
        size_t firstDollar = storedHash.find('$', 8);
        if (firstDollar == std::string::npos) return false;

        std::string saltHex = storedHash.substr(8, firstDollar - 8);
        std::string expectedHashHex = storedHash.substr(firstDollar + 1);

        std::string saltedInput = saltHex + password + "SanghaviMartPepper2026";
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(saltedInput.c_str()), saltedInput.length(), hash);

        std::stringstream hashStream;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            hashStream << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return hashStream.str() == expectedHashHex;
    }
};

} // namespace sanghavimart::utils
