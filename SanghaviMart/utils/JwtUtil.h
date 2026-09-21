#pragma once
#include <string>
#include <chrono>
#include <json/json.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <drogon/drogon.h>

namespace sanghavimart::utils {

struct JwtPayload {
    int userId{0};
    std::string email;
    std::string name;
    std::string role;
    int64_t exp{0};
    bool valid{false};
};

class JwtUtil {
private:
    static inline const std::string SECRET = "SanghaviMart_Secure_Capstone_Jwt_Secret_Key_2026_C++20";

    // Base64URL encoding
    static std::string base64UrlEncode(const std::string& input) {
        std::string b64 = drogon::utils::base64Encode(reinterpret_cast<const unsigned char*>(input.data()), input.size());
        std::string b64url;
        for (char c : b64) {
            if (c == '+') b64url += '-';
            else if (c == '/') b64url += '_';
            else if (c != '=') b64url += c;
        }
        return b64url;
    }

    // Base64URL decoding
    static std::string base64UrlDecode(const std::string& input) {
        std::string b64 = input;
        for (char& c : b64) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }
        while (b64.size() % 4 != 0) {
            b64 += '=';
        }
        return drogon::utils::base64Decode(b64);
    }

    static std::string hmacSha256(const std::string& key, const std::string& data) {
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen = 0;
        HMAC(EVP_sha256(), key.data(), key.size(),
             reinterpret_cast<const unsigned char*>(data.data()), data.size(),
             result, &resultLen);
        return std::string(reinterpret_cast<char*>(result), resultLen);
    }

public:
    static std::string generateToken(int userId, const std::string& email, const std::string& name, const std::string& role, int expireSeconds = 86400) {
        Json::Value header;
        header["alg"] = "HS256";
        header["typ"] = "JWT";

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        std::string headerStr = Json::writeString(writer, header);

        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        Json::Value payload;
        payload["sub"] = userId;
        payload["email"] = email;
        payload["name"] = name;
        payload["role"] = role;
        payload["iat"] = static_cast<Json::Int64>(now);
        payload["exp"] = static_cast<Json::Int64>(now + expireSeconds);

        std::string payloadStr = Json::writeString(writer, payload);

        std::string encHeader = base64UrlEncode(headerStr);
        std::string encPayload = base64UrlEncode(payloadStr);
        std::string signature = base64UrlEncode(hmacSha256(SECRET, encHeader + "." + encPayload));

        return encHeader + "." + encPayload + "." + signature;
    }

    static JwtPayload verifyToken(const std::string& token) {
        JwtPayload result;
        result.valid = false;

        size_t firstDot = token.find('.');
        size_t secondDot = token.rfind('.');
        if (firstDot == std::string::npos || secondDot == std::string::npos || firstDot == secondDot) {
            return result;
        }

        std::string encHeader = token.substr(0, firstDot);
        std::string encPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
        std::string encSignature = token.substr(secondDot + 1);

        std::string expectedSig = base64UrlEncode(hmacSha256(SECRET, encHeader + "." + encPayload));
        if (expectedSig != encSignature) {
            return result; // Invalid signature
        }

        std::string payloadJsonStr = base64UrlDecode(encPayload);
        Json::CharReaderBuilder readerBuilder;
        Json::Value payloadJson;
        std::string errs;
        std::istringstream s(payloadJsonStr);
        if (!Json::parseFromStream(readerBuilder, s, &payloadJson, &errs)) {
            return result;
        }

        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        int64_t exp = payloadJson["exp"].asInt64();
        if (now > exp) {
            return result; // Expired
        }

        result.userId = payloadJson["sub"].asInt();
        result.email = payloadJson["email"].asString();
        result.name = payloadJson["name"].asString();
        result.role = payloadJson["role"].asString();
        result.exp = exp;
        result.valid = true;
        return result;
    }
};

} // namespace sanghavimart::utils
