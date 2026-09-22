#include "ProductV1Controller.h"
#include <drogon/drogon.h>
#include "../dto/Dtos.h"
#include "../repository/PostgresRepositories.h"
#include "../service/Services.h"
#include "../util/JsonUtil.h"
#include "../util/ValidationUtil.h"
#include "SessionAuth.h"
#include "V1Error.h"

namespace sanghavimart::controller {

static std::shared_ptr<repository::IProductRepository> Products() {
    return std::make_shared<repository::PostgresProductRepository>(
        drogon::app().getDbClient());
}

void ProductV1Controller::List(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        std::string q = req->getParameter("q");
        std::string category = req->getParameter("category");
        if (q.size() > 200 || category.size() > 100)
            throw exception::ValidationException("Search input too long.");
        auto items = Products()->Search(q, category);
        Json::Value arr(Json::arrayValue);
        for (auto& p : items) arr.append(dto::ProductResponse::From(p));
        Json::Value d;
        d["products"] = arr;
        d["count"] = (int)items.size();
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ProductV1::List", e);
    }
}

void ProductV1Controller::Get(const drogon::HttpRequestPtr&,
                              std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                              int id) {
    try {
        if (id <= 0) throw exception::ValidationException("Invalid product id.");
        auto p = Products()->FindById(id);
        if (!p) throw exception::NotFoundException("Product not found.");
        Json::Value d;
        d["product"] = dto::ProductResponse::From(*p);
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ProductV1::Get", e);
    }
}

void ProductV1Controller::Create(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, false, true, true);
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        service::ProductService svc(Products());
        int id = svc.CreateProduct(user.id, (*j).get("name", "").asString(),
                                   (*j).get("description", "").asString(),
                                   (*j).get("price_cents", 0).asInt64(),
                                   (*j).get("stock_qty", 0).asInt(),
                                   (*j).get("category", "General").asString());
        auto p = Products()->FindById(id);
        Json::Value d;
        d["product"] = dto::ProductResponse::From(*p);
        auto r = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d));
        r->setStatusCode(drogon::k201Created);
        cb(r);
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ProductV1::Create", e);
    }
}

void ProductV1Controller::Update(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 int id) {
    try {
        auto user = RequireSession(req, false, true, true);
        if (id <= 0) throw exception::ValidationException("Invalid product id.");
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        auto repos = Products();
        auto cur = repos->FindById(id);
        if (!cur) throw exception::NotFoundException("Product not found.");
        model::Product p = *cur;
        if (j->isMember("name")) p.name = (*j)["name"].asString();
        if (j->isMember("description")) p.description = (*j)["description"].asString();
        if (j->isMember("price_cents")) {
            int64_t c = (*j)["price_cents"].asInt64();
            util::ValidationUtil::ValidatePriceCents(c);
            p.price = model::Money::FromMinor(c);
        }
        if (j->isMember("stock_qty")) {
            int s = (*j)["stock_qty"].asInt();
            if (s < 0) throw exception::ValidationException("Invalid stock.");
            p.stock_qty = s;
        }
        if (j->isMember("category")) p.category = (*j)["category"].asString();
        if (j->isMember("image_url")) p.image_url = (*j)["image_url"].asString();
        util::ValidationUtil::ValidateName(p.name, "Product name");
        service::ProductService svc(repos);
        svc.UpdateOwned(user.id, user.role == "ADMIN", p);
        Json::Value d;
        d["product"] = dto::ProductResponse::From(*repos->FindById(id));
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ProductV1::Update", e);
    }
}

void ProductV1Controller::Remove(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                 int id) {
    try {
        auto user = RequireSession(req, false, true, true);
        if (id <= 0) throw exception::ValidationException("Invalid product id.");
        auto repos = Products();
        auto cur = repos->FindById(id);
        if (!cur) throw exception::NotFoundException("Product not found.");
        if (user.role != "ADMIN" && cur->seller_id != user.id)
            throw exception::ForbiddenException("Cannot delete another seller's product.");
        repos->Remove(id);
        Json::Value d;
        d["message"] = "Product removed.";
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "ProductV1::Remove", e);
    }
}

}  // namespace sanghavimart::controller
