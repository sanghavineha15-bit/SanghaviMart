#include "OrderController.h"
#include <cctype>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>

namespace sanghavimart::controllers {

static std::string generateOrderNumber() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    static std::mt19937 rng(static_cast<unsigned int>(now));
    std::uniform_int_distribution<int> dist(1000, 9999);
    return "SM-2026-" + std::to_string(dist(rng));
}

void OrderController::createOrder(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Shipping details JSON payload required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string shippingName = (*json).get("shipping_name", "").asString();
    std::string shippingPhone = (*json).get("shipping_phone", "").asString();
    std::string shippingAddress = (*json).get("shipping_address", "").asString();
    std::string shippingCity = (*json).get("shipping_city", "").asString();
    std::string shippingPostalCode = (*json).get("shipping_postal_code", "").asString();
    std::string paymentMethod = (*json).get("payment_method", "cod_mock").asString();

    if (shippingName.empty() || shippingPhone.empty() || shippingAddress.empty() || shippingCity.empty()) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Please complete all shipping address fields.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();

    // 1. Fetch current cart items with latest product prices & stock
    dbClient->execSqlAsync(
        "SELECT ci.id AS item_id, ci.product_id, ci.quantity, p.name, p.price, p.stock_quantity, p.seller_id "
        "FROM cart_items ci "
        "JOIN cart c ON ci.cart_id = c.id "
        "JOIN products p ON ci.product_id = p.id "
        "WHERE c.buyer_id = $1 AND p.is_active = TRUE",
        [dbClient, buyerId, shippingName, shippingPhone, shippingAddress, shippingCity, shippingPostalCode, paymentMethod, callback](const drogon::orm::Result& cartRows) {
            if (cartRows.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Your cart is currently empty. Please add products before checking out.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            // Verify stock for each item
            double totalAmount = 0.0;
            for (const auto& row : cartRows) {
                int requestedQty = row["quantity"].as<int>();
                int availableStock = row["stock_quantity"].as<int>();
                std::string prodName = row["name"].as<std::string>();

                if (requestedQty > availableStock) {
                    Json::Value err;
                    err["success"] = false;
                    err["error"] = "Order failed: '" + prodName + "' only has " + std::to_string(availableStock) + " left in stock (you requested " + std::to_string(requestedQty) + ").";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                    resp->setStatusCode(drogon::k400BadRequest);
                    callback(resp);
                    return;
                }

                double price = row["price"].as<double>();
                totalAmount += (price * requestedQty);
            }

            std::string orderNumber = generateOrderNumber();

            // 2. Insert into orders table
            dbClient->execSqlAsync(
                "INSERT INTO orders (order_number, buyer_id, total_amount, status, shipping_name, shipping_phone, shipping_address, shipping_city, shipping_postal_code, payment_method) "
                "VALUES ($1, $2, $3, 'PENDING', $4, $5, $6, $7, $8, $9) RETURNING id, order_number, total_amount, status, created_at",
                [dbClient, cartRows, buyerId, callback](const drogon::orm::Result& orderRes) {
                    int orderId = orderRes[0]["id"].as<int>();
                    std::string orderNum = orderRes[0]["order_number"].as<std::string>();
                    double finalAmount = orderRes[0]["total_amount"].as<double>();
                    std::string status = orderRes[0]["status"].as<std::string>();
                    std::string createdAt = orderRes[0]["created_at"].as<std::string>();

                    // 3. Insert order_items and decrement product stock
                    for (const auto& row : cartRows) {
                        int productId = row["product_id"].as<int>();
                        int sellerId = row["seller_id"].as<int>();
                        int quantity = row["quantity"].as<int>();
                        double unitPrice = row["price"].as<double>();
                        double subtotal = unitPrice * quantity;

                        // Insert order item
                        dbClient->execSqlAsync(
                            "INSERT INTO order_items (order_id, product_id, seller_id, quantity, unit_price, subtotal) "
                            "VALUES ($1, $2, $3, $4, $5, $6)",
                            [](const drogon::orm::Result&) {},
                            [](const drogon::orm::DrogonDbException&) {},
                            orderId, productId, sellerId, quantity, unitPrice, subtotal
                        );

                        // Deduct product stock
                        dbClient->execSqlAsync(
                            "UPDATE products SET stock_quantity = stock_quantity - $1 WHERE id = $2",
                            [](const drogon::orm::Result&) {},
                            [](const drogon::orm::DrogonDbException&) {},
                            quantity, productId
                        );
                    }

                    // 4. Clear buyer's cart
                    dbClient->execSqlAsync(
                        "DELETE FROM cart_items WHERE cart_id IN (SELECT id FROM cart WHERE buyer_id = $1)",
                        [](const drogon::orm::Result&) {},
                        [](const drogon::orm::DrogonDbException&) {},
                        buyerId
                    );

                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "Congratulations! Your order has been placed successfully on SanghaviMart.";
                    res["order_id"] = orderId;
                    res["order_number"] = orderNum;
                    res["total_amount"] = finalAmount;
                    res["status"] = status;
                    res["created_at"] = createdAt;

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    resp->setStatusCode(drogon::k201Created);
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
                orderNumber, buyerId, totalAmount, shippingName, shippingPhone, shippingAddress, shippingCity, shippingPostalCode, paymentMethod
            );
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
        buyerId
    );
}

void OrderController::getOrders(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int userId = req->attributes()->get<int>("user_id");
    std::string role = req->attributes()->get<std::string>("user_role");
    auto dbClient = drogon::app().getDbClient();

    std::string sql;
    bool needsParam = false;
    if (role == "BUYER") {
        // Buyer views all orders they placed (parameterized)
        sql = "SELECT o.id, o.order_number, o.buyer_id, u.name AS buyer_name, o.total_amount, o.status, "
              "o.shipping_name, o.shipping_city, o.payment_method, o.created_at, "
              "COUNT(oi.id) AS item_count "
              "FROM orders o "
              "JOIN users u ON o.buyer_id = u.id "
              "LEFT JOIN order_items oi ON o.id = oi.order_id "
              "WHERE o.buyer_id = $1 "
              "GROUP BY o.id, u.name ORDER BY o.id DESC";
        needsParam = true;
    } else if (role == "SELLER") {
        // Seller views orders containing products they sold (parameterized)
        sql = "SELECT DISTINCT o.id, o.order_number, o.buyer_id, u.name AS buyer_name, "
              "SUM(oi.subtotal) AS seller_subtotal, o.status, o.shipping_name, o.shipping_city, "
              "o.created_at, COUNT(oi.id) AS item_count "
              "FROM orders o "
              "JOIN users u ON o.buyer_id = u.id "
              "JOIN order_items oi ON o.id = oi.order_id "
              "WHERE oi.seller_id = $1 "
              "GROUP BY o.id, u.name ORDER BY o.id DESC";
        needsParam = true;
    } else {
        // Admin views all orders
        sql = "SELECT o.id, o.order_number, o.buyer_id, u.name AS buyer_name, o.total_amount, o.status, "
              "o.shipping_name, o.shipping_city, o.payment_method, o.created_at, "
              "COUNT(oi.id) AS item_count "
              "FROM orders o "
              "JOIN users u ON o.buyer_id = u.id "
              "LEFT JOIN order_items oi ON o.id = oi.order_id "
              "GROUP BY o.id, u.name ORDER BY o.id DESC";
    }

    auto okCb = [callback, role](const drogon::orm::Result& result) {
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
                if (role == "SELLER") {
                    ord["total_amount"] = row["seller_subtotal"].as<double>();
                } else {
                    ord["total_amount"] = row["total_amount"].as<double>();
                }
                ord["status"] = row["status"].as<std::string>();
                ord["shipping_name"] = row["shipping_name"].as<std::string>();
                ord["shipping_city"] = row["shipping_city"].as<std::string>();
                ord["created_at"] = row["created_at"].as<std::string>();
                ord["item_count"] = row["item_count"].as<int>();
                orders.append(ord);
            }

            res["orders"] = orders;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        };
        auto errCb = [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "getOrders db error: " << e.base().what();
            Json::Value err;
            err["success"] = false;
            err["error"] = "Unable to fetch orders.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        };
        if (needsParam) {
            dbClient->execSqlAsync(sql, okCb, errCb, userId);
        } else {
            dbClient->execSqlAsync(sql, okCb, errCb);
        }
}

void OrderController::getOrderById(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                  int id) {
    int userId = req->attributes()->get<int>("user_id");
    std::string role = req->attributes()->get<std::string>("user_role");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT o.id, o.order_number, o.buyer_id, u.name AS buyer_name, u.email AS buyer_email, "
        "o.total_amount, o.status, o.shipping_name, o.shipping_phone, o.shipping_address, "
        "o.shipping_city, o.shipping_postal_code, o.payment_method, o.created_at "
        "FROM orders o "
        "JOIN users u ON o.buyer_id = u.id "
        "WHERE o.id = $1",
        [dbClient, userId, role, id, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Order not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            int buyerId = result[0]["buyer_id"].as<int>();
            if (role == "BUYER" && buyerId != userId) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Permission denied: You can only view your own orders.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            Json::Value res;
            res["success"] = true;
            Json::Value ord;
            ord["id"] = result[0]["id"].as<int>();
            ord["order_number"] = result[0]["order_number"].as<std::string>();
            ord["buyer_id"] = buyerId;
            ord["buyer_name"] = result[0]["buyer_name"].as<std::string>();
            ord["buyer_email"] = result[0]["buyer_email"].as<std::string>();
            ord["total_amount"] = result[0]["total_amount"].as<double>();
            ord["status"] = result[0]["status"].as<std::string>();
            ord["shipping_name"] = result[0]["shipping_name"].as<std::string>();
            ord["shipping_phone"] = result[0]["shipping_phone"].as<std::string>();
            ord["shipping_address"] = result[0]["shipping_address"].as<std::string>();
            ord["shipping_city"] = result[0]["shipping_city"].as<std::string>();
            ord["shipping_postal_code"] = result[0]["shipping_postal_code"].as<std::string>();
            ord["payment_method"] = result[0]["payment_method"].as<std::string>();
            ord["created_at"] = result[0]["created_at"].as<std::string>();

            // Query items (parameterized; seller scoping via $2, never concatenated)
            const std::string itemSqlAll =
                "SELECT oi.id, oi.product_id, p.name AS product_name, p.image_url, "
                "oi.seller_id, s.name AS seller_name, oi.quantity, oi.unit_price, oi.subtotal "
                "FROM order_items oi "
                "JOIN products p ON oi.product_id = p.id "
                "JOIN users s ON oi.seller_id = s.id "
                "WHERE oi.order_id = $1";
            const std::string itemSqlSeller = itemSqlAll + " AND oi.seller_id = $2";

            auto itemsOk = [callback, res, ord](const drogon::orm::Result& itemRows) mutable {
                    Json::Value items(Json::arrayValue);
                    for (const auto& row : itemRows) {
                        Json::Value it;
                        it["id"] = row["id"].as<int>();
                        it["product_id"] = row["product_id"].as<int>();
                        it["product_name"] = row["product_name"].as<std::string>();
                        it["image_url"] = row["image_url"].as<std::string>();
                        it["seller_id"] = row["seller_id"].as<int>();
                        it["seller_name"] = row["seller_name"].as<std::string>();
                        it["quantity"] = row["quantity"].as<int>();
                        it["unit_price"] = row["unit_price"].as<double>();
                        it["subtotal"] = row["subtotal"].as<double>();
                        items.append(it);
                    }
                    ord["items"] = items;
                    res["order"] = ord;

                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    callback(resp);
                };
                auto itemsErr = [callback](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "getOrderById items db error: " << e.base().what();
                    Json::Value err;
                    err["success"] = false;
                    err["error"] = "Unable to fetch order items.";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                };
                if (role == "SELLER") {
                    dbClient->execSqlAsync(itemSqlSeller, itemsOk, itemsErr, id, userId);
                } else {
                    dbClient->execSqlAsync(itemSqlAll, itemsOk, itemsErr, id);
                }
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "getOrderById db error: " << e.base().what();
            Json::Value err;
            err["success"] = false;
            err["error"] = "Unable to fetch order.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        id
    );
}

void OrderController::updateOrderStatus(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                       int id) {
    std::string role = req->attributes()->get<std::string>("user_role");
    if (role != "SELLER" && role != "ADMIN") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Only sellers and administrators can update order status.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid JSON body.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string newStatus = (*json).get("status", "").asString();
    for (auto& c : newStatus) c = (char)toupper(c);  // accept lowercase clients
    if (newStatus != "PENDING" && newStatus != "CONFIRMED" && newStatus != "SHIPPED" && newStatus != "DELIVERED" && newStatus != "CANCELLED") {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Status must be: PENDING, CONFIRMED, SHIPPED, DELIVERED, or CANCELLED.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "UPDATE orders SET status = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2 RETURNING id, order_number, status",
        [callback, newStatus](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Order not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            Json::Value res;
            res["success"] = true;
            res["message"] = "Order status updated to '" + newStatus + "'.";
            res["order_id"] = result[0]["id"].as<int>();
            res["order_number"] = result[0]["order_number"].as<std::string>();
            res["status"] = result[0]["status"].as<std::string>();

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
        newStatus, id
    );
}

} // namespace sanghavimart::controllers
