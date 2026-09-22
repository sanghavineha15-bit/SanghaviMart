#include "CartController.h"

namespace sanghavimart::controllers {

void CartController::getCart(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto dbClient = drogon::app().getDbClient();

    // Ensure cart exists
    dbClient->execSqlAsync(
        "INSERT INTO cart (buyer_id) VALUES ($1) ON CONFLICT (buyer_id) DO NOTHING",
        [dbClient, buyerId, callback](const drogon::orm::Result&) {
            dbClient->execSqlAsync(
                "SELECT c.id AS cart_id, ci.id AS item_id, ci.product_id, p.name AS product_name, "
                "p.price, ci.quantity, p.stock_quantity, p.image_url, "
                "(p.price * ci.quantity) AS subtotal "
                "FROM cart c "
                "JOIN cart_items ci ON c.id = ci.cart_id "
                "JOIN products p ON ci.product_id = p.id "
                "WHERE c.buyer_id = $1 AND p.is_active = TRUE "
                "ORDER BY ci.id ASC",
                [callback, buyerId](const drogon::orm::Result& items) {
                    Json::Value res;
                    res["success"] = true;
                    res["buyer_id"] = buyerId;

                    double totalPrice = 0.0;
                    int totalItems = 0;
                    Json::Value itemsArray(Json::arrayValue);

                    for (const auto& row : items) {
                        Json::Value it;
                        it["id"] = row["item_id"].as<int>();
                        it["cart_id"] = row["cart_id"].as<int>();
                        it["product_id"] = row["product_id"].as<int>();
                        it["name"] = row["product_name"].as<std::string>();
                        it["price"] = row["price"].as<double>();
                        it["quantity"] = row["quantity"].as<int>();
                        it["stock_quantity"] = row["stock_quantity"].as<int>();
                        it["image_url"] = row["image_url"].as<std::string>();
                        double subtotal = row["subtotal"].as<double>();
                        it["subtotal"] = subtotal;

                        totalPrice += subtotal;
                        totalItems += it["quantity"].as<int>();
                        itemsArray.append(it);
                    }

                    res["items"] = itemsArray;
                    res["total_price"] = totalPrice;
                    res["total_items"] = totalItems;

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
                buyerId
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

void CartController::addToCart(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Invalid JSON payload.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    int productId = (*json).get("product_id", 0).asInt();
    int quantity = (*json).get("quantity", 1).asInt();

    if (productId <= 0 || quantity <= 0) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Valid product_id and positive quantity are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    // 1. Check product stock
    dbClient->execSqlAsync(
        "SELECT id, name, stock_quantity, price FROM products WHERE id = $1 AND is_active = TRUE",
        [dbClient, buyerId, productId, quantity, callback](const drogon::orm::Result& prodRes) {
            if (prodRes.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Product is unavailable or out of stock.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            int availableStock = prodRes[0]["stock_quantity"].as<int>();
            std::string prodName = prodRes[0]["name"].as<std::string>();

            if (availableStock <= 0) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Sorry, '" + prodName + "' is currently out of stock.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            // 2. Get or create cart ID
            dbClient->execSqlAsync(
                "INSERT INTO cart (buyer_id) VALUES ($1) ON CONFLICT (buyer_id) DO UPDATE SET updated_at = CURRENT_TIMESTAMP RETURNING id",
                [dbClient, productId, quantity, availableStock, prodName, callback](const drogon::orm::Result& cartRes) {
                    int cartId = cartRes[0]["id"].as<int>();

                    // 3. Check existing quantity in cart
                    dbClient->execSqlAsync(
                        "SELECT id, quantity FROM cart_items WHERE cart_id = $1 AND product_id = $2",
                        [dbClient, cartId, productId, quantity, availableStock, prodName, callback](const drogon::orm::Result& itemRes) {
                            int newQuantity = quantity;
                            if (!itemRes.empty()) {
                                newQuantity += itemRes[0]["quantity"].as<int>();
                            }

                            // Enforce strict stock limit
                            if (newQuantity > availableStock) {
                                Json::Value err;
                                err["success"] = false;
                                err["error"] = "Cannot add " + std::to_string(quantity) + " units. Total in cart would exceed available stock (" + std::to_string(availableStock) + ").";
                                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                                resp->setStatusCode(drogon::k400BadRequest);
                                callback(resp);
                                return;
                            }

                            if (itemRes.empty()) {
                                dbClient->execSqlAsync(
                                    "INSERT INTO cart_items (cart_id, product_id, quantity) VALUES ($1, $2, $3)",
                                    [callback, prodName](const drogon::orm::Result&) {
                                        Json::Value res;
                                        res["success"] = true;
                                        res["message"] = "Added '" + prodName + "' to cart.";
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
                                    cartId, productId, newQuantity
                                );
                            } else {
                                dbClient->execSqlAsync(
                                    "UPDATE cart_items SET quantity = $1 WHERE id = $2",
                                    [callback, prodName](const drogon::orm::Result&) {
                                        Json::Value res;
                                        res["success"] = true;
                                        res["message"] = "Updated quantity for '" + prodName + "' in cart.";
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
                                    newQuantity, itemRes[0]["id"].as<int>()
                                );
                            }
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
                        cartId, productId
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
        productId
    );
}

void CartController::updateCartItem(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   int id) {
    int buyerId = req->attributes()->get<int>("user_id");
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

    int quantity = (*json).get("quantity", 1).asInt();
    if (quantity <= 0) {
        removeCartItem(req, std::move(callback), id);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    // Validate ownership and available stock
    dbClient->execSqlAsync(
        "SELECT ci.id, ci.product_id, p.stock_quantity, p.name "
        "FROM cart_items ci "
        "JOIN cart c ON ci.cart_id = c.id "
        "JOIN products p ON ci.product_id = p.id "
        "WHERE ci.id = $1 AND c.buyer_id = $2",
        [dbClient, quantity, id, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Cart item not found or does not belong to you.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            int stock = result[0]["stock_quantity"].as<int>();
            std::string name = result[0]["name"].as<std::string>();

            if (quantity > stock) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Requested quantity (" + std::to_string(quantity) + ") exceeds available stock (" + std::to_string(stock) + ") for '" + name + "'.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            dbClient->execSqlAsync(
                "UPDATE cart_items SET quantity = $1 WHERE id = $2",
                [callback](const drogon::orm::Result&) {
                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "Cart quantity updated successfully.";
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
                quantity, id
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
        id, buyerId
    );
}

void CartController::removeCartItem(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   int id) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "DELETE FROM cart_items WHERE id = $1 AND cart_id IN (SELECT id FROM cart WHERE buyer_id = $2)",
        [callback](const drogon::orm::Result& result) {
            if (result.affectedRows() == 0) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Item not found in your cart.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            Json::Value res;
            res["success"] = true;
            res["message"] = "Product removed from cart.";
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
        id, buyerId
    );
}

void CartController::clearCart(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "DELETE FROM cart_items WHERE cart_id IN (SELECT id FROM cart WHERE buyer_id = $1)",
        [callback](const drogon::orm::Result&) {
            Json::Value res;
            res["success"] = true;
            res["message"] = "Cart cleared.";
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
        buyerId
    );
}

} // namespace sanghavimart::controllers
