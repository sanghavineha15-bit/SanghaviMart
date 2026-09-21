/**
 * @file main.cpp
 * @brief SanghaviMart - C++20 Drogon E-Commerce Platform Server Entry Point
 * @author Neha Sanghavi (Capstone Project)
 */

#include <drogon/drogon.h>
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::cout << "===========================================================" << std::endl;
    std::cout << "       SANGHAVIMART - C++20 DROGON E-COMMERCE SERVER       " << std::endl;
    std::cout << "===========================================================" << std::endl;

    std::string configFile = "config.json";
    if (argc > 1) {
        configFile = argv[1];
    }

    if (!std::filesystem::exists(configFile)) {
        std::cout << "[WARN] Config file '" << configFile << "' not found in current directory." << std::endl;
        std::cout << "[INFO] Loading fallback default configuration on port 8080..." << std::endl;
        drogon::app().addListener("0.0.0.0", 8080);
        drogon::app().setDocumentRoot("./frontend");
    } else {
        std::cout << "[INFO] Loading Drogon configuration from: " << configFile << std::endl;
        drogon::app().loadConfigFile(configFile);
    }

    // CORS preflight and headers handler for modern frontend compatibility
    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr& req,
                                              drogon::AdviceCallback&& callback,
                                              drogon::AdviceChainCallback&& next) {
        if (req->method() == drogon::HttpMethod::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
            resp->addHeader("Access-Control-Max-Age", "86400");
            callback(resp);
            return;
        }
        next();
    });

    // Global response filter to append CORS headers to all responses
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr&,
                                               const drogon::HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });

    // Custom 404 handler returning JSON for /api routes or HTML fallback
    drogon::app().setCustom404Page([](const drogon::HttpRequestPtr& req) {
        if (req->path().rfind("/api", 0) == 0) {
            Json::Value json;
            json["success"] = false;
            json["error"] = "API endpoint not found: " + req->path();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
            resp->setStatusCode(drogon::k404NotFound);
            return resp;
        }
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("<html><body><h1>404 Not Found - SanghaviMart</h1></body></html>");
        return resp;
    }());

    std::cout << "[INFO] SanghaviMart server initialized successfully." << std::endl;
    std::cout << "[INFO] Listening for connections. Press Ctrl+C to terminate." << std::endl;

    // Run the non-blocking event loop
    drogon::app().run();

    return 0;
}
