#include "OrderV1Controller.h"
#include <cctype>
#include <drogon/drogon.h>
#include "../dto/Dtos.h"
#include "../repository/PostgresCartOrderReview.h"
#include "../service/Services.h"
#include "../util/JsonUtil.h"
#include "SessionAuth.h"
#include "V1Error.h"

namespace sanghavimart::controller {

static std::shared_ptr<repository::IOrderRepository> Orders() {
    return std::make_shared<repository::PostgresOrderRepository>(
        drogon::app().getDbClient());
}

void OrderV1Controller::Checkout(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, false, false);
        // Mock payment (§19): strategy confirms server-side total inside txn.
        service::MockPaymentStrategy pay;
        (void)pay;
        int order_id;
        try {
            order_id = Orders()->CheckoutTransaction(user.id);
        } catch (const std::runtime_error& e) {
            std::string w = e.what();
            if (w == "cart empty")
                throw exception::ValidationException("Cart is empty.");
            throw exception::ConflictException("Checkout failed: insufficient stock.");
        }
        auto o = Orders()->FindById(order_id);
        if (!o) throw exception::InternalException();
        Json::Value d;
        d["order"] = dto::OrderResponse::From(*o);
        auto r = drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d));
        r->setStatusCode(drogon::k201Created);
        cb(r);
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "OrderV1::Checkout", e);
    }
}

void OrderV1Controller::List(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    try {
        auto user = RequireSession(req, true, true, true);
        std::vector<model::Order> v;
        if (user.role == "BUYER")
            v = Orders()->OrdersForBuyer(user.id);
        else if (user.role == "SELLER")
            v = Orders()->OrdersForSeller(user.id);
        else
            v = Orders()->AllOrders();
        Json::Value arr(Json::arrayValue);
        for (auto& o : v) arr.append(dto::OrderResponse::From(o));
        Json::Value d;
        d["orders"] = arr;
        d["count"] = (int)v.size();
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "OrderV1::List", e);
    }
}

void OrderV1Controller::Get(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                            int id) {
    try {
        auto user = RequireSession(req, true, true, true);
        if (id <= 0) throw exception::ValidationException("Invalid order id.");
        auto o = Orders()->FindById(id);
        if (!o) throw exception::NotFoundException("Order not found.");
        if (user.role == "BUYER" && o->buyer_id != user.id)
            throw exception::ForbiddenException("Cannot view another user's order.");
        if (user.role == "SELLER") {
            bool involved = false;
            for (auto& s : Orders()->OrdersForSeller(user.id))
                if (s.id == id) {
                    involved = true;
                    break;
                }
            if (!involved) throw exception::ForbiddenException("Order has none of your products.");
        }
        Json::Value d;
        d["order"] = dto::OrderResponse::From(*o);
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "OrderV1::Get", e);
    }
}

void OrderV1Controller::SetStatus(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                  int id) {
    try {
        auto user = RequireSession(req, false, true, true);
        if (id <= 0) throw exception::ValidationException("Invalid order id.");
        auto j = req->getJsonObject();
        if (!j) throw exception::ValidationException("Invalid JSON body.");
        std::string to = (*j).get("status", "").asString();
        for (auto& c : to) c = (char)toupper(c);
        auto o = Orders()->FindById(id);
        if (!o) throw exception::NotFoundException("Order not found.");
        if (!service::IsValidTransition(o->status, to))
            throw exception::ValidationException("Illegal status transition from " + o->status +
                                                 " to " + to + ".");
        Orders()->SetStatus(id, to);
        Json::Value d;
        d["order"] = dto::OrderResponse::From(*Orders()->FindById(id));
        cb(drogon::HttpResponse::newHttpJsonResponse(util::JsonUtil::Success(d)));
    } catch (const exception::AppException& e) {
        SendAppError(cb, e);
    } catch (const std::exception& e) {
        SendInternal(cb, "OrderV1::SetStatus", e);
    }
}

}  // namespace sanghavimart::controller
