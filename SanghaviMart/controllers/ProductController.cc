#include "ProductController.h"
#include <sstream>

namespace sanghavimart::controllers {

void ProductController::getAllProducts(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();

    std::string category = req->getParameter("category");
    std::string search = req->getParameter("q");
    std::string sellerParam = req->getParameter("seller_id");

    // SECURITY: all user input is bound as parameters ($1..). LIKE patterns
    // are passed as bound values, never interpolated.
    const std::string base =
        "SELECT p.id, p.seller_id, u.name AS seller_name, p.category_id, c.name AS category_name, "
        "p.name, p.description, p.price, p.stock_quantity, p.image_url, p.is_active, p.created_at, "
        "COALESCE(AVG(r.rating), 0) AS average_rating, COUNT(r.id) AS review_count "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "JOIN categories c ON p.category_id = c.id "
        "LEFT JOIN reviews r ON p.id = r.product_id "
        "WHERE p.is_active = TRUE "
        "AND ($1 = '' OR c.slug = $1 OR c.name ILIKE $4) "
        "AND ($2 = '' OR p.name ILIKE $5 OR p.description ILIKE $5) "
        "AND ($3 = 0 OR p.seller_id = $3) "
        "GROUP BY p.id, u.name, c.name "
        "ORDER BY p.id DESC";

    int sellerId = 0;
    if (!sellerParam.empty()) {
        try {
            sellerId = std::stoi(sellerParam);
        } catch (...) {
            sellerId = 0;
        }
    }
    std::string catLike = category.empty() ? "" : "%" + category + "%";
    std::string searchLike = search.empty() ? "" : "%" + search + "%";

    dbClient->execSqlAsync(
        base,
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
                item["average_rating"] = row["average_rating"].as<double>();
                item["review_count"] = row["review_count"].as<int>();
                item["created_at"] = row["created_at"].as<std::string>();
                products.append(item);
            }
            res["products"] = products;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "getAllProducts db error: " << e.base().what();
            Json::Value err;
            err["success"] = false;
            err["error"] = "Unable to fetch products. Please try again.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        category, search, sellerId, catLike, searchLike
    );
}

void ProductController::getProductById(const drogon::HttpRequestPtr&,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                      int id) {
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT p.id, p.seller_id, u.name AS seller_name, p.category_id, c.name AS category_name, "
        "p.name, p.description, p.price, p.stock_quantity, p.image_url, p.is_active, p.created_at, "
        "COALESCE(AVG(r.rating), 0) AS average_rating, COUNT(r.id) AS review_count "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "JOIN categories c ON p.category_id = c.id "
        "LEFT JOIN reviews r ON p.id = r.product_id "
        "WHERE p.id = $1 "
        "GROUP BY p.id, u.name, c.name",
        [callback, id, dbClient](const drogon::orm::Result& result) {
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
            Json::Value item;
            item["id"] = result[0]["id"].as<int>();
            item["seller_id"] = result[0]["seller_id"].as<int>();
            item["seller_name"] = result[0]["seller_name"].as<std::string>();
            item["category_id"] = result[0]["category_id"].as<int>();
            item["category_name"] = result[0]["category_name"].as<std::string>();
            item["name"] = result[0]["name"].as<std::string>();
            item["description"] = result[0]["description"].as<std::string>();
            item["price"] = result[0]["price"].as<double>();
            item["stock_quantity"] = result[0]["stock_quantity"].as<int>();
            item["image_url"] = result[0]["image_url"].as<std::string>();
            item["is_active"] = result[0]["is_active"].as<bool>();
            item["average_rating"] = result[0]["average_rating"].as<double>();
            item["review_count"] = result[0]["review_count"].as<int>();
            item["created_at"] = result[0]["created_at"].as<std::string>();
            res["product"] = item;

            // Fetch reviews for this product
            dbClient->execSqlAsync(
                "SELECT r.id, r.rating, r.comment, r.created_at, u.name AS buyer_name "
                "FROM reviews r "
                "JOIN users u ON r.buyer_id = u.id "
                "WHERE r.product_id = $1 ORDER BY r.id DESC",
                [callback, res](const drogon::orm::Result& revRes) mutable {
                    Json::Value revArr(Json::arrayValue);
                    for (const auto& row : revRes) {
                        Json::Value r;
                        r["id"] = row["id"].as<int>();
                        r["rating"] = row["rating"].as<int>();
                        r["comment"] = row["comment"].as<std::string>();
                        r["buyer_name"] = row["buyer_name"].as<std::string>();
                        r["created_at"] = row["created_at"].as<std::string>();
                        revArr.append(r);
                    }
                    res["reviews"] = revArr;
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    callback(resp);
                },
                [callback, res](const drogon::orm::DrogonDbException&) mutable {
                    res["reviews"] = Json::Value(Json::arrayValue);
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
                    callback(resp);
                },
                id
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
        id
    );
}

void ProductController::addProduct(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int sellerId = req->attributes()->get<int>("user_id");
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

    std::string name = (*json)["name"].asString();
    std::string description = (*json)["description"].asString();
    double price = (*json).get("price", 0.0).asDouble();
    int categoryId = (*json).get("category_id", 1).asInt();
    int stock = (*json).get("stock_quantity", 0).asInt();
    std::string imageUrl = (*json).get("image_url", "https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80").asString();

    if (name.empty() || description.empty() || price <= 0.0 || stock < 0) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Product name, description, positive price, and valid stock quantity are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "INSERT INTO products (seller_id, category_id, name, description, price, stock_quantity, image_url, is_active) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, TRUE) RETURNING id, name, price, stock_quantity",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            res["message"] = "Product listed successfully on SanghaviMart.";
            res["product"]["id"] = result[0]["id"].as<int>();
            res["product"]["name"] = result[0]["name"].as<std::string>();
            res["product"]["price"] = result[0]["price"].as<double>();
            res["product"]["stock_quantity"] = result[0]["stock_quantity"].as<int>();
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
        sellerId, categoryId, name, description, price, stock, imageUrl
    );
}

void ProductController::updateProduct(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                     int id) {
    int sellerId = req->attributes()->get<int>("user_id");
    std::string role = req->attributes()->get<std::string>("user_role");
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

    auto dbClient = drogon::app().getDbClient();
    // Verify ownership
    dbClient->execSqlAsync(
        "SELECT seller_id FROM products WHERE id = $1",
        [dbClient, sellerId, role, id, json, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Product not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            int currentSellerId = result[0]["seller_id"].as<int>();
            if (role != "ADMIN" && currentSellerId != sellerId) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Permission denied: You can only edit your own products.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            std::string name = (*json).get("name", "").asString();
            std::string desc = (*json).get("description", "").asString();
            double price = (*json).get("price", 0.0).asDouble();
            int stock = (*json).get("stock_quantity", -1).asInt();
            int categoryId = (*json).get("category_id", 0).asInt();
            std::string imageUrl = (*json).get("image_url", "").asString();

            dbClient->execSqlAsync(
                "UPDATE products SET "
                "name = CASE WHEN $1 != '' THEN $1 ELSE name END, "
                "description = CASE WHEN $2 != '' THEN $2 ELSE description END, "
                "price = CASE WHEN $3 > 0.0 THEN $3 ELSE price END, "
                "stock_quantity = CASE WHEN $4 >= 0 THEN $4 ELSE stock_quantity END, "
                "category_id = CASE WHEN $5 > 0 THEN $5 ELSE category_id END, "
                "image_url = CASE WHEN $6 != '' THEN $6 ELSE image_url END, "
                "updated_at = CURRENT_TIMESTAMP "
                "WHERE id = $7 RETURNING id, name, price, stock_quantity",
                [callback](const drogon::orm::Result& updateRes) {
                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "Product updated successfully.";
                    res["product"]["id"] = updateRes[0]["id"].as<int>();
                    res["product"]["name"] = updateRes[0]["name"].as<std::string>();
                    res["product"]["price"] = updateRes[0]["price"].as<double>();
                    res["product"]["stock_quantity"] = updateRes[0]["stock_quantity"].as<int>();
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
                name, desc, price, stock, categoryId, imageUrl, id
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
        id
    );
}

void ProductController::deleteProduct(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                     int id) {
    int userId = req->attributes()->get<int>("user_id");
    std::string role = req->attributes()->get<std::string>("user_role");
    auto dbClient = drogon::app().getDbClient();

    dbClient->execSqlAsync(
        "SELECT seller_id FROM products WHERE id = $1",
        [dbClient, userId, role, id, callback](const drogon::orm::Result& result) {
            if (result.empty()) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Product not found.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            int sellerId = result[0]["seller_id"].as<int>();
            if (role != "ADMIN" && (role != "SELLER" || sellerId != userId)) {
                Json::Value err;
                err["success"] = false;
                err["error"] = "Permission denied. Only the product seller or an administrator can delete this listing.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            // Soft-delete by marking inactive to preserve order history integrity
            dbClient->execSqlAsync(
                "UPDATE products SET is_active = FALSE, updated_at = CURRENT_TIMESTAMP WHERE id = $1",
                [callback](const drogon::orm::Result&) {
                    Json::Value res;
                    res["success"] = true;
                    res["message"] = "Product has been successfully deleted/deactivated.";
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

void ProductController::getCategories(const drogon::HttpRequestPtr&,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto dbClient = drogon::app().getDbClient();
    dbClient->execSqlAsync(
        "SELECT id, name, slug, description, image_url FROM categories ORDER BY id ASC",
        [callback](const drogon::orm::Result& result) {
            Json::Value res;
            res["success"] = true;
            Json::Value categories(Json::arrayValue);
            for (const auto& row : result) {
                Json::Value cat;
                cat["id"] = row["id"].as<int>();
                cat["name"] = row["name"].as<std::string>();
                cat["slug"] = row["slug"].as<std::string>();
                cat["description"] = row["description"].as<std::string>();
                cat["image_url"] = row["image_url"].as<std::string>();
                categories.append(cat);
            }
            res["categories"] = categories;
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
