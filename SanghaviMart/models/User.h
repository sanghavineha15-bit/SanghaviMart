#pragma once
#include <string>
#include <json/json.h>

namespace sanghavimart::models {

struct User {
    int id{0};
    std::string name;
    std::string email;
    std::string passwordHash;
    std::string role; // "buyer", "seller", "admin"
    std::string phone;
    std::string address;
    std::string createdAt;
    std::string updatedAt;

    Json::Value toJson(bool includePrivate = false) const {
        Json::Value val;
        val["id"] = id;
        val["name"] = name;
        val["email"] = email;
        val["role"] = role;
        val["phone"] = phone;
        val["address"] = address;
        val["created_at"] = createdAt;
        if (includePrivate) {
            val["password_hash"] = passwordHash;
        }
        return val;
    }
};

} // namespace sanghavimart::models
