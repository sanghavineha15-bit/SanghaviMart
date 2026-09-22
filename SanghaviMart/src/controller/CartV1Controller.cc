#include "CartV1Controller.h"
#include <drogon/drogon.h>
#include "../repository/PostgresCartOrderReview.h"
#include "../repository/PostgresRepositories.h"
#include "../util/JsonUtil.h"
#include "../util/ValidationUtil.h"
#include "SessionAuth.h"
#include "V1Error.h"

namespace sanghavimart::controller {

static std::shared_ptr<repository::ICartRepository> Carts() {
    return std::make_shared<repository::PostgresCartRepository>(drogon::app().getDbClient());
}
static std::shared_ptr<repository::IProductRepository> Products() {
    return std::make_shared<repository::PostgresProductRepository>(
        drogon::app().getDbClient());
}

// Cart view joins current server-side prices (cents) — frontend totals ignored.
static Json::Value CartView(int user_id) {
    auto items = Carts()->ListForUser(user_id);
    auto prods = Products();
    Json::Value arr(Json::arrayValue);
    int64_t total = 0;
    for (auto& c : items) {
        auto p = prods->FindById(c.product_id);
        if (!p) continue;
        int64_t line = p->price.MinorUnits() * c.quantity;
        total += line;
        Json::Value v;
        v["product_id"] = c.product_id;
        v["quantity"] = c.quantity;
        v["unit_price_cents"] = (Json::Int64)p->price.MinorUnits();
        v["line_total_cents"] = (Json::Int64)line;
        v["name"] = p->name;
        arr.append(v);
    }
    Json::Value d;
    d["items"] = arr;
    d["total_cents"] = (Json::Int64)total;
    return d;
}

void CartV1Controller::Show(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, false, true);
        cb(drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Success(CartView(user.id))));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "CartV1::Show", e);
    }
}

void CartV1Controller::Add(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, false, true);
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        int pid = (*j).get("product_id", 0).asInt();
        int qty = (*j).get("quantity", 1).asInt();
        util::ValidationUtil::ValidateQuantity(qty);
        auto p = Products()->FindById(pid);
        if (!p) throw exception::NotFoundException("Product not found.");
        if (p->stock_qty < qty)
            throw exception::ValidationException("Quantity exceeds available stock.");
        Carts()->Upsert(user.id, pid, qty);
        cb(drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Success(CartView(user.id))));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "CartV1::Add", e);
    }
}

void CartV1Controller::SetQty(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int productId) {
    try {
        auto user = RequireSession(req, true, false, true);
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        int qty = (*j).get("quantity", 0).asInt();
        if (qty < 0) throw exception::ValidationException("Invalid quantity.");
        if (qty == 0) {
            Carts()->Remove(user.id, productId);
        } else {
            util::ValidationUtil::ValidateQuantity(qty);
            auto p = Products()->FindById(productId);
            if (!p) throw exception::NotFoundException("Product not found.");
            if (p->stock_qty < qty)
                throw exception::ValidationException("Quantity exceeds available stock.");
            Carts()->SetQuantity(user.id, productId, qty);
        }
        cb(drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Success(CartView(user.id))));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "CartV1::SetQty", e);
    }
}

void CartV1Controller::Remove(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int productId) {
    try {
        auto user = RequireSession(req, true, false, true);
        Carts()->Remove(user.id, productId);
        cb(drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Success(CartView(user.id))));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "CartV1::Remove", e);
    }
}

void CartV1Controller::Clear(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, false, true);
        Carts()->Clear(user.id);
        cb(drogon::HttpResponse::newHttpJsonResponse(
            util::JsonUtil::Success(CartView(user.id))));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "CartV1::Clear", e);
    }
}

}  // namespace sanghavimart::controller
