#include "AdminController.h"
#include <cctype>

namespace sanghavimart::controllers {

void AdminController::getAllUsers(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();
    std::string roleFilter = req->getParameter("role");
    for (auto& c : roleFilter) c = (char)toupper(c);  // accept ?role=buyer

    auto okCb = [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            res["count"] = static_cast<int>(result.size());
            Json::Value users(Json::arrayValue);

            for (const auto& row : result) {
                Json::Value u;
                u["id"] = row["id"].as<int>();
                u["name"] = row["name"].as<std::string>();
                u["email"] = row["email"].as<std::string>();
                u["role"] = row["role"].as<std::string>();
                u["phone"] = row["phone"].isNull() ? "" : row["phone"].as<std::string>();
                u["address"] = row["address"].isNull() ? "" : row["address"].as<std::string>();
                u["created_at"] = row["created_at"].as<std::string>();
                users.append(u);
            }
            res["users"] = users;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        };
    auto errCb = [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "getAllUsers db error: " << e.base().what();
            Json::Value err;
            err["success"] = false;
            err["error"] = "Unable to fetch users.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        };
    if (roleFilter == "BUYER" || roleFilter == "SELLER" || roleFilter == "ADMIN") {
        dbClient->execSqlAsync(
            "SELECT id, name, email, role, phone, address, created_at FROM users "
            "WHERE role = $1 ORDER BY id ASC",
            okCb, errCb, roleFilter);
    } else {
        dbClient->execSqlAsync(
            "SELECT id, name, email, role, phone, address, created_at FROM users "
            "ORDER BY id ASC",
            okCb, errCb);
    }
}

void AdminController::getAllProducts(const drogon::HttpRequestPtr&,
                                    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT p.id, p.seller_id, u.name AS seller_name, p.category_id, c.name AS category_name, "
        "p.name, p.description, p.price, p.stock_quantity, p.image_url, p.is_active, p.created_at "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "JOIN categories c ON p.category_id = c.id "
        "ORDER BY p.id DESC",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            res["count"] = static_cast<int>(result.size());
            Json::Value products(Json::arrayValue);

            for (const auto& row : result) {
                Json::Value item;
                item["id"] = row["id"].as<int>();
                item["seller_id"] = row["seller_id"].as<int>();
                item["seller_name"] = row["seller_name"].as<std::string>();
                item["category_id"] = row["category_id"].as<int>();
                item["category_name"] = row["category_name"].as<std::string>();
                item["name"] = row["name"].as<std::string>();
                item["description"] = row["description"].as<std::string>();
                item["price"] = row["price"].as<double>();
                item["stock_quantity"] = row["stock_quantity"].as<int>();
                item["image_url"] = row["image_url"].as<std::string>();
                item["is_active"] = row["is_active"].as<bool>();
                item["created_at"] = row["created_at"].as<std::string>();
                products.append(item);
            }
            res["products"] = products;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    );
}

void AdminController::getAllOrders(const drogon::HttpRequestPtr&,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT o.id, o.order_number, o.buyer_id, u.name AS buyer_name, o.total_amount, o.status, "
        "o.shipping_name, o.shipping_city, o.payment_method, o.created_at, "
        "COUNT(oi.id) AS item_count "
        "FROM orders o "
        "JOIN users u ON o.buyer_id = u.id "
        "LEFT JOIN order_items oi ON o.id = oi.order_id "
        "GROUP BY o.id, u.name ORDER BY o.id DESC",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            res["count"] = static_cast<int>(result.size());
            Json::Value orders(Json::arrayValue);

            for (const auto& row : result) {
                Json::Value ord;
                ord["id"] = row["id"].as<int>();
                ord["order_number"] = row["order_number"].as<std::string>();
                ord["buyer_id"] = row["buyer_id"].as<int>();
                ord["buyer_name"] = row["buyer_name"].as<std::string>();
                ord["total_amount"] = row["total_amount"].as<double>();
                ord["status"] = row["status"].as<std::string>();
                ord["shipping_name"] = row["shipping_name"].as<std::string>();
                ord["shipping_city"] = row["shipping_city"].as<std::string>();
                ord["payment_method"] = row["payment_method"].as<std::string>();
                ord["created_at"] = row["created_at"].as<std::string>();
                ord["item_count"] = row["item_count"].as<int>();
                orders.append(ord);
            }
            res["orders"] = orders;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    );
}

void AdminController::deleteProduct(const drogon::HttpRequestPtr&,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   int id) {
    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "UPDATE products SET is_active = FALSE, updated_at = CURRENT_TIMESTAMP WHERE id = $1 RETURNING id, name",
        [callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Product not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            Json::Value res;
            res["success"] = true;
            res["message"] = "Product '" + result[0]["name"].as<std::string>() + "' removed by Administrator.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        id
    );
}

void AdminController::getPlatformStats(const drogon::HttpRequestPtr&,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT "
        "(SELECT COUNT(*) FROM users WHERE role = 'BUYER') AS total_buyers, "
        "(SELECT COUNT(*) FROM users WHERE role = 'SELLER') AS total_sellers, "
        "(SELECT COUNT(*) FROM products WHERE is_active = TRUE) AS active_products, "
        "(SELECT COUNT(*) FROM orders) AS total_orders, "
        "(SELECT COALESCE(SUM(total_amount), 0) FROM orders) AS total_revenue",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            if (!result.empty()) {
                res["stats"]["total_buyers"] = result[0]["total_buyers"].as<int>();
                res["stats"]["total_sellers"] = result[0]["total_sellers"].as<int>();
                res["stats"]["active_products"] = result[0]["active_products"].as<int>();
                res["stats"]["total_orders"] = result[0]["total_orders"].as<int>();
                res["stats"]["total_revenue"] = result[0]["total_revenue"].as<double>();
            }
            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            Json::Value err;
            err["success"] = false;
            LOG_ERROR << "db error: " << e.base().what();
            err["error"] = "Internal server error. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    );
}

} // namespace sanghavimart::controllers
