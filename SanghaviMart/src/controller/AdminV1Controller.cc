#include "AdminV1Controller.h"
#include <cctype>
#include <drogon/drogon.h>
#include "../dto/Dtos.h"
#include "../repository/PostgresCartOrderReview.h"
#include "../repository/PostgresRepositories.h"
#include "../util/JsonUtil.h"
#include "SessionAuth.h"
#include "V1Error.h"

namespace sanghavimart::controller {

void AdminV1Controller::Users(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        RequireSession(req, false, false, true);
        std::string filter = req->getParameter("role");
        for (auto& c : filter) c = (char)toupper(c);
        repository::PostgresUserRepository repos(drogon::app().getDbClient());
        auto v = repos.ListAll(filter);
        Json::Value arr(Json::arrayValue);
        for (auto& u : v) {
            dto::UserResponse ur{u.id, u.name, u.email, u.role};
            arr.append(ur.ToJson());
        }
        Json::Value d;
        d["users"] = arr;
        d["count"] = (int)v.size();
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "AdminV1::Users", e);
    }
}

void AdminV1Controller::Orders(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        RequireSession(req, false, false, true);
        repository::PostgresOrderRepository repos(drogon::app().getDbClient());
        auto v = repos.AllOrders();
        Json::Value arr(Json::arrayValue);
        for (auto& o : v) arr.append(dto::OrderResponse::From(o));
        Json::Value d;
        d["orders"] = arr;
        d["count"] = (int)v.size();
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "AdminV1::Orders", e);
    }
}

void AdminV1Controller::Stats(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        RequireSession(req, false, false, true);
        auto db = drogon::app().getDbClient();
        repository::PostgresUserRepository users(db);
        repository::PostgresOrderRepository orders(db);
        auto all = users.ListAll("");
        int buyers = 0, sellers = 0;
        for (auto& u : all) {
            if (u.role == "BUYER") ++buyers;
            if (u.role == "SELLER") ++sellers;
        }
        int64_t gmv = 0;
        auto allOrders = orders.AllOrders();
        for (auto& o : allOrders) gmv += o.total.MinorUnits();
        Json::Value d;
        d["buyers"] = buyers;
        d["sellers"] = sellers;
        d["orders"] = (int)allOrders.size();
        d["gmv_cents"] = (Json::Int64)gmv;
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "AdminV1::Stats", e);
    }
}

void AdminV1Controller::RemoveProduct(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                      int id) {
    try {
        RequireSession(req, false, false, true);
        if (id <= 0) throw exception::ValidationException("Invalid product id.");
        repository::PostgresProductRepository repos(drogon::app().getDbClient());
        if (!repos.FindById(id)) throw exception::NotFoundException("Product not found.");
        repos.Remove(id);
        Json::Value d;
        d["message"] = "Product removed.";
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "AdminV1::RemoveProduct", e);
    }
}

}  // namespace sanghavimart::controller
