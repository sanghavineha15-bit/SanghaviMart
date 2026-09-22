#pragma once
#include <memory>
#include "../exception/AppException.h"
#include "../model/Entities.h"
#include "../repository/Interfaces.h"
#include "../util/PasswordUtil.h"
#include "../util/ValidationUtil.h"

namespace sanghavimart::service {

/// @brief Authentication business logic (registration/login validation).
/// Constructor injection over repository interfaces (DIP).
class AuthService {
  public:
    explicit AuthService(std::shared_ptr<repository::IUserRepository> users)
        : users_(std::move(users)) {}

    model::User Register(const std::string& name, const std::string& email,
                         const std::string& password, const std::string& role) {
        util::ValidationUtil::ValidateName(name);
        util::ValidationUtil::ValidateEmail(email);
        util::ValidationUtil::ValidatePassword(password);
        util::ValidationUtil::ValidateRole(role);
        if (users_->FindByEmail(email))
            throw exception::ConflictException("Email already registered.");
        std::string hash = util::PasswordUtil::Hash(password);
        int id = users_->Create(name, email, hash, role);
        auto u = users_->FindByEmail(email);
        if (!u) throw exception::InternalException();
        u->id = id;
        return *u;
    }

    model::User Authenticate(const std::string& email, const std::string& password) {
        util::ValidationUtil::ValidateEmail(email);
        util::ValidationUtil::RequireNonEmpty(password, "Password");
        auto u = users_->FindByEmail(email);
        auto hash = users_->PasswordHashFor(email);
        if (!u || !hash || !util::PasswordUtil::Verify(password, *hash))
            throw exception::UnauthorizedException("Invalid credentials.");
        return *u;
    }

  private:
    std::shared_ptr<repository::IUserRepository> users_;
};

/// @brief Product + cart + order + review orchestration with authz checks.
class ProductService {
  public:
    explicit ProductService(std::shared_ptr<repository::IProductRepository> products)
        : products_(std::move(products)) {}

    int CreateProduct(int seller_id, const std::string& name, const std::string& desc,
                      int64_t price_cents, int stock, const std::string& category) {
        util::ValidationUtil::ValidateName(name, "Product name");
        util::ValidationUtil::ValidatePriceCents(price_cents);
        if (stock < 0) throw exception::ValidationException("Invalid stock.");
        model::Product p;
        p.seller_id = seller_id;
        p.name = name;
        p.description = desc;
        p.price = model::Money::FromMinor(price_cents);
        p.stock_qty = stock;
        p.category = category.empty() ? "General" : category;
        return products_->Create(p);
    }

    void UpdateOwned(int seller_id, bool is_admin, model::Product p) {
        auto cur = products_->FindById(p.id);
        if (!cur) throw exception::NotFoundException("Product not found.");
        if (!is_admin && cur->seller_id != seller_id)
            throw exception::ForbiddenException("Cannot modify another seller's product.");
        p.seller_id = cur->seller_id;  // ownership immutable
        products_->Update(p);
    }

  private:
    std::shared_ptr<repository::IProductRepository> products_;
};

/// Mock payment Strategy (no real gateway).
struct IPaymentStrategy {
    virtual ~IPaymentStrategy() = default;
    virtual bool Confirm(int64_t amount_cents) = 0;
};
struct MockPaymentStrategy : IPaymentStrategy {
    bool Confirm(int64_t) override { return true; }
};

/// Valid order state machine.
inline bool IsValidTransition(const std::string& from, const std::string& to) {
    if (to == "CANCELLED") return from == "PENDING" || from == "CONFIRMED";
    if (from == "PENDING" && to == "CONFIRMED") return true;
    if (from == "CONFIRMED" && to == "SHIPPED") return true;
    if (from == "SHIPPED" && to == "DELIVERED") return true;
    return false;
}

}  // namespace sanghavimart::service
