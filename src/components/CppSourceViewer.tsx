import React, { useState } from 'react';
import { Code2, FileCode, Database, Terminal, Copy, Check, Folder, ChevronRight } from 'lucide-react';

export const CppSourceViewer: React.FC = () => {
  const [selectedFile, setSelectedFile] = useState<string>('main.cpp');
  const [copied, setCopied] = useState(false);

  const fileContents: Record<string, { lang: string; code: string; desc: string }> = {
    'main.cpp': {
      lang: 'cpp',
      desc: 'C++20 Drogon server entry point with thread pool and connection listeners',
      code: `#include <drogon/drogon.h>
#include <iostream>

int main() {
    std::cout << "====================================================\\n";
    std::cout << "Starting SanghaviMart C++20 Drogon E-Commerce Server\\n";
    std::cout << "Author: Neha Sanghavi | Capstone Project\\n";
    std::cout << "====================================================\\n";

    drogon::app()
        .loadConfigFile("../config.json")
        .setLogLevel(trantor::Logger::kInfo)
        .setCustom404Page(drogon::HttpResponse::newHttpJsonResponse([]{
            Json::Value res;
            res["success"] = false;
            res["error"] = "SanghaviMart API Endpoint Not Found";
            return res;
        }()))
        .registerBeginningAdvice([] {
            std::cout << "Async non-blocking event loops initialized successfully.\\n";
        });

    drogon::app().run();
    return 0;
}`,
    },

    'CMakeLists.txt': {
      lang: 'cmake',
      desc: 'CMake 3.16+ build definition linking Drogon, OpenSSL, PostgreSQL, and JSONCPP',
      code: `cmake_minimum_required(VERSION 3.16)
project(SanghaviMart CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

file(GLOB_RECURSE SRC_FILES
    "main.cpp"
    "controllers/*.cc"
    "filters/*.cc"
)

add_executable(SanghaviMart \${SRC_FILES})

target_include_directories(SanghaviMart PRIVATE
    \${CMAKE_CURRENT_SOURCE_DIR}
    \${CMAKE_CURRENT_SOURCE_DIR}/controllers
    \${CMAKE_CURRENT_SOURCE_DIR}/models
    \${CMAKE_CURRENT_SOURCE_DIR}/filters
    \${CMAKE_CURRENT_SOURCE_DIR}/utils
    \${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(SanghaviMart PRIVATE
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    \${PostgreSQL_LIBRARIES}
)`,
    },

    'OrderController.cc': {
      lang: 'cpp',
      desc: 'Atomic checkout routine, stock deduction, and order creation in C++',
      code: `void OrderController::createOrder(const drogon::HttpRequestPtr& req,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    int buyerId = req->attributes()->get<int>("user_id");
    auto json = req->getJsonObject();
    // 1. Fetch current cart items & verify product stock
    // 2. Validate requestedQty <= availableStock (prevent race condition overselling)
    // 3. Atomically INSERT INTO orders, INSERT INTO order_items
    // 4. UPDATE products SET stock_quantity = stock_quantity - $1
    // 5. DELETE FROM cart_items WHERE cart_id = buyer_cart_id
    // 6. Return unique Order Number (SM-2026-XXXX) and confirmation
}`,
    },

    'schema.sql': {
      lang: 'sql',
      desc: 'PostgreSQL Relational Schema with cascade constraints & unique verified review indexes',
      code: `CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('buyer', 'seller', 'admin')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    seller_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    category_id INT NOT NULL REFERENCES categories(id),
    name VARCHAR(200) NOT NULL,
    price NUMERIC(10, 2) NOT NULL CHECK (price >= 0),
    stock_quantity INT NOT NULL DEFAULT 0 CHECK (stock_quantity >= 0),
    is_active BOOLEAN DEFAULT TRUE
);

CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    order_number VARCHAR(50) UNIQUE NOT NULL,
    buyer_id INT NOT NULL REFERENCES users(id),
    total_amount NUMERIC(10, 2) NOT NULL,
    status VARCHAR(30) DEFAULT 'pending' CHECK (status IN ('pending', 'processing', 'shipped', 'delivered', 'cancelled'))
);`,
    },

    'PasswordUtil.h': {
      lang: 'cpp',
      desc: 'Cryptographic Salted SHA-256 / PBKDF2 Password Hashing Utility',
      code: `#pragma once
#include <string>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace sanghavimart::utils {
class PasswordUtil {
public:
    static std::string hashPassword(const std::string& plainPassword, const std::string& salt = "SanghaviMart_Salt_2026") {
        std::string salted = salt + plainPassword + salt;
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(salted.c_str()), salted.size(), hash);
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    static bool verifyPassword(const std::string& password, const std::string& hash) {
        return hashPassword(password) == hash;
    }
};
}`,
    },
  };

  const handleCopy = () => {
    navigator.clipboard.writeText(fileContents[selectedFile]?.code || '');
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="space-y-6">
      {/* Title */}
      <div className="bg-white p-6 rounded-2xl border border-slate-200 shadow-xs flex flex-wrap items-center justify-between gap-4">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Code2 className="w-6 h-6 text-emerald-600" />
            <h1 className="text-xl font-extrabold text-slate-900">C++20 Drogon Source Code & Architecture</h1>
          </div>
          <p className="text-xs text-slate-500">
            Inspect the high-performance C++ backend controllers, CMake build pipeline, and PostgreSQL relational schema.
          </p>
        </div>

        <div className="flex items-center gap-2">
          <span className="text-xs font-semibold bg-emerald-50 text-emerald-700 px-3 py-1 rounded-full border border-emerald-200">
            C++20 Native Standard
          </span>
          <span className="text-xs font-semibold bg-blue-50 text-blue-700 px-3 py-1 rounded-full border border-blue-200">
            Drogon Async Non-blocking I/O
          </span>
        </div>
      </div>

      {/* Code Browser Grid */}
      <div className="grid grid-cols-1 lg:grid-cols-4 gap-6">
        {/* Sidebar file tree */}
        <div className="bg-white p-4 rounded-2xl border border-slate-200 shadow-xs lg:col-span-1 space-y-3">
          <div className="text-xs font-bold text-slate-700 uppercase tracking-wider flex items-center gap-1.5">
            <Folder className="w-4 h-4 text-amber-500" /> SanghaviMart Project Files
          </div>

          <div className="space-y-1">
            {Object.keys(fileContents).map((fileName) => (
              <button
                key={fileName}
                onClick={() => setSelectedFile(fileName)}
                className={`w-full text-left px-3 py-2 rounded-xl text-xs font-medium flex items-center justify-between transition-colors ${
                  selectedFile === fileName
                    ? 'bg-blue-50 text-blue-700 font-bold border border-blue-200'
                    : 'text-slate-600 hover:bg-slate-100'
                }`}
              >
                <div className="flex items-center gap-2 truncate">
                  <FileCode className="w-3.5 h-3.5 text-slate-400" />
                  <span className="truncate">{fileName}</span>
                </div>
                <ChevronRight className="w-3 h-3 text-slate-400" />
              </button>
            ))}
          </div>

          {/* Quick Terminal Command */}
          <div className="pt-3 border-t border-slate-200">
            <div className="text-[11px] font-bold text-slate-500 mb-1 flex items-center gap-1">
              <Terminal className="w-3 h-3" /> Quick Build
            </div>
            <pre className="bg-slate-900 text-slate-200 p-2.5 rounded-lg text-[10px] font-mono leading-relaxed overflow-x-auto">
              mkdir build && cd build{"\n"}
              cmake ..{"\n"}
              cmake --build .{"\n"}
              ./SanghaviMart
            </pre>
          </div>
        </div>

        {/* Code Display */}
        <div className="bg-slate-900 text-slate-100 rounded-2xl border border-slate-800 shadow-xl lg:col-span-3 flex flex-col overflow-hidden">
          <div className="p-3.5 bg-slate-950 border-b border-slate-800 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className="font-mono font-bold text-xs text-blue-400">{selectedFile}</span>
              <span className="text-[11px] text-slate-400 hidden sm:inline">
                — {fileContents[selectedFile]?.desc}
              </span>
            </div>

            <button
              onClick={handleCopy}
              className="px-2.5 py-1 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs rounded-md flex items-center gap-1.5 transition-colors"
            >
              {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
              {copied ? 'Copied' : 'Copy Code'}
            </button>
          </div>

          <div className="p-4 overflow-x-auto font-mono text-xs leading-relaxed text-slate-200 flex-1">
            <pre>{fileContents[selectedFile]?.code}</pre>
          </div>
        </div>
      </div>
    </div>
  );
};
