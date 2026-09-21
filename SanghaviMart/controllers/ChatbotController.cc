#include "ChatbotController.h"
#include <algorithm>

namespace sanghavimart::controllers {

void ChatbotController::handleChatMessage(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Message body required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userMessage = (*json).get("message", "").asString();
    if (userMessage.empty()) {
        Json::Value err;
        err["success"] = false;
        err["error"] = "Message cannot be empty.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Convert message to lowercase for matching
    std::string lower = userMessage;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });

    std::string reply;

    if (lower.find("hello") != std::string::npos || lower.find("hi") != std::string::npos || lower.find("hey") != std::string::npos) {
        reply = "Hello! Welcome to **SanghaviMart**. I am your AI Shopping Assistant. How can I help you today? You can ask about products, placing an order, seller onboarding, or order status.";
    } else if (lower.find("order") != std::string::npos || lower.find("track") != std::string::npos || lower.find("status") != std::string::npos) {
        reply = "**Order Status Guide:**\n- Navigate to **Order History** from the top menu.\n- You can view your Order ID, item breakdown, total amount, and real-time status (Pending, Processing, Shipped, or Delivered).";
    } else if (lower.find("cart") != std::string::npos || lower.find("buy") != std::string::npos || lower.find("checkout") != std::string::npos) {
        reply = "**How to Buy on SanghaviMart:**\n1. Browse products and click **Add to Cart**.\n2. Open your Cart to review items and adjust quantities (the system prevents adding more than current stock).\n3. Proceed to **Checkout**, provide shipping details, and confirm.\n4. Your order is placed immediately with automated inventory deduction!";
    } else if (lower.find("seller") != std::string::npos || lower.find("add product") != std::string::npos) {
        reply = "**Seller Features:**\n- Register or login with a **Seller** account.\n- Access your **Seller Dashboard**.\n- List new products with price, category, stock, and photos.\n- View incoming customer orders and update product inventory.";
    } else if (lower.find("headphone") != std::string::npos || lower.find("laptop") != std::string::npos || lower.find("chair") != std::string::npos || lower.find("product") != std::string::npos) {
        reply = "**Popular Products on SanghaviMart:**\n- 🎧 *NoisePulse ANC Headphones* ($79.99)\n- 💻 *UltraBook Pro 14 (C++ Dev Edition)* ($1099.00)\n- ⌨️ *Precision Mechanical Keyboard 75%* ($89.50)\n- 🪑 *ErgoComfort High-Back Mesh Chair* ($199.99)\nUse the search bar and category filters to explore more!";
    } else if (lower.find("review") != std::string::npos || lower.find("rating") != std::string::npos) {
        reply = "**Product Reviews:**\n- Buyers who have purchased a product can leave a 1–5 star rating and comment.\n- SanghaviMart verifies purchase history before accepting reviews to prevent spam.";
    } else if (lower.find("admin") != std::string::npos) {
        reply = "**Admin Controls:**\n- The Admin Dashboard allows administrators to inspect all users (Buyers & Sellers), monitor active products, review transactions, and moderate inappropriate listings.";
    } else {
        reply = "Thank you for contacting SanghaviMart support! I can assist you with product searches, categories, cart management, mock checkout, order tracking, and seller features. What would you like to explore?";
    }

    Json::Value res;
    res["success"] = true;
    res["reply"] = reply;
    res["source"] = "SanghaviMart-Assistant-C++Drogon";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(res);
    callback(resp);
}

} // namespace sanghavimart::controllers
