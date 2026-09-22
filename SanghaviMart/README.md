# SanghaviMart - Full-Stack C++20 Drogon E-Commerce Platform

**SanghaviMart** is a production-architected, high-concurrency multi-seller e-commerce web platform developed as an academic capstone project. It utilizes **C++20** and the **Drogon asynchronous non-blocking framework** on the backend, a normalized **PostgreSQL** relational database, and modern responsive **HTML5/CSS3/JavaScript** on the frontend.

---

## 1. Project Overview & Role Architecture

The system implements strict Role-Based Access Control (RBAC) across three key user roles:

1. **Buyer**:
   - Register, login, manage JWT session.
   - Browse catalog with full-text search and category filtering.
   - Real-time stock availability inspection.
   - Shopping cart operations with strict inventory ceiling enforcement.
   - Frictionless multi-step mock checkout (Cart → Shipping Details → Confirmation → Order ID).
   - Order history tracking with live fulfillment status (`pending`, `processing`, `shipped`, `delivered`).
   - Verified-purchase product reviews (1–5 stars + written feedback; unverified buyers are strictly rejected).

2. **Seller**:
   - Register and login as an independent seller.
   - Dedicated Seller Dashboard.
   - Add new products (title, description, price, category, stock quantity, image).
   - Edit pricing and stock inventory.
   - Delete/deactivate listings.
   - Inspect incoming customer orders for their listed items.

3. **Administrator (Pre-seeded)**:
   - Dedicated Admin Dashboard with system analytics.
   - Inspect all platform users (filtered by Buyer / Seller).
   - Oversee catalog-wide inventory and remove inappropriate products.
   - Monitor total platform orders and gross merchandise value (GMV).

---

## 2. Technology Stack

| Layer | Technology |
|---|---|
| **Programming Language** | C++20 (ISO/IEC 14882:2020) |
| **Web Framework** | Drogon Framework (libtrantor non-blocking async I/O) |
| **Database** | PostgreSQL 15+ only (no SQLite paths) |
| **Build System** | CMake 3.25+ |
| **Security & Auth** | JSON Web Tokens (HMAC-SHA256) + Salted SHA-256 / Bcrypt password hashing |
| **Frontend** | Semantic HTML5, Vanilla CSS3 (Custom Design System), JavaScript (ES6+) |
| **AI Assistant** | Google Gemini API (`gemini-3.8-flash`) / Resilient internal e-commerce matcher |
| **Version Control** | Git & GitHub |
| **Target IDE** | Visual Studio Code / CLion / Visual Studio 2022 |

---

## 3. Complete Project Directory Structure

```text
SanghaviMart/
├── CMakeLists.txt                 # CMake C++20 build definition & asset pipeline
├── config.json                    # Drogon server, thread pool, database pool configuration
├── README.md                      # Complete capstone documentation and setup guide
├── main.cpp                       # Server initialization, CORS filters, and event loop
├── sql/
│   └── schema.sql                 # Relational PostgreSQL DDL schema with constraints and seed data
├── controllers/                   # Drogon HttpControllers (REST Endpoints)
│   ├── AuthController.h / .cc     # POST /api/register, /api/login, /api/logout, /api/auth/me
│   ├── ProductController.h / .cc  # GET, POST, PUT, DELETE /api/products, /api/categories
│   ├── CartController.h / .cc     # GET, POST, PUT, DELETE /api/cart
│   ├── OrderController.h / .cc    # POST, GET /api/orders, /api/orders/{id}, status updates
│   ├── ReviewController.h / .cc   # POST /api/reviews, GET /api/products/{id}/reviews
│   ├── AdminController.h / .cc    # GET /api/admin/users, /products, /orders, /stats
│   └── ChatbotController.h / .cc  # POST /api/chat (Shopping guidance assistant)
├── filters/                       # Request middleware & authorization filters
│   ├── AuthFilter.h / .cc         # JWT token verification
│   ├── BuyerFilter                # Enforces buyer role
│   ├── SellerFilter               # Enforces seller role
│   └── AdminFilter                # Enforces admin role
├── models/                        # C++ data transfer structures & JSON serializers
│   ├── User.h
│   ├── Product.h
│   ├── Cart.h
│   ├── Order.h
│   └── Review.h
├── utils/                         # Cryptography, JWT, and database utilities
│   ├── PasswordUtil.h             # Salted SHA-256 password hashing & verification
│   └── JwtUtil.h                  # HMAC-SHA256 JWT encoder, decoder, and claims validator
├── frontend/                      # Web pages served by Drogon document root
│   ├── index.html                 # Marketplace landing page
│   ├── login.html                 # Master portal selection page
│   ├── login-buyer.html           # Dedicated Buyer Portal sign in
│   ├── login-seller.html          # Dedicated Seller Center sign in
│   ├── login-admin.html           # Dedicated Administrator Console sign in
│   ├── register.html              # Multi-role user registration
│   ├── products.html              # Catalog, search, and category filters
│   ├── cart.html                  # Interactive cart with quantity limits
│   ├── checkout.html              # Shipping & mock order placement
│   ├── orders.html                # Buyer order history
│   ├── seller-dashboard.html      # Seller product management & order tracking
│   └── admin-dashboard.html       # Administrative user & inventory moderation
└── static/
    ├── css/style.css              # Responsive styling
    └── js/api.js                  # Frontend REST API client
```

---

## 4. Database Schema & Entity-Relationship (ER) Design

### Tables & Relationships:
1. `users` (id, name, email, password_hash, role, phone, address, timestamps)
2. `categories` (id, name, slug, description, image_url)
3. `products` (id, seller_id -> users.id, category_id -> categories.id, name, description, price, stock_quantity, image_url, is_active)
4. `cart` (id, buyer_id -> users.id UNIQUE)
5. `cart_items` (id, cart_id -> cart.id, product_id -> products.id, quantity)
6. `orders` (id, order_number, buyer_id -> users.id, total_amount, status, shipping details)
7. `order_items` (id, order_id -> orders.id, product_id -> products.id, seller_id -> users.id, quantity, unit_price, subtotal)
8. `reviews` (id, product_id -> products.id, buyer_id -> users.id, order_id -> orders.id, rating, comment)

### Key Relational Constraints:
- **Cascade Deletions**: Deleting a cart or order cascades to its respective child items.
- **Stock Integrity**: Cart item addition dynamically checks `quantity <= stock_quantity`.
- **Verified Reviews**: A UNIQUE constraint on `(product_id, buyer_id, order_id)` enforces that a user can only review products they actually purchased in an order.

---

## 5. REST API Documentation

| Method | Endpoint | Access | Description |
|---|---|---|---|
| `POST` | `/api/register` | Public | Register new Buyer or Seller with validated payload |
| `POST` | `/api/login` | Public | Authenticate user and receive signed JWT token |
| `POST` | `/api/logout` | Public | Invalidate client session token |
| `GET` | `/api/auth/me` | Logged In | Retrieve current user profile from token |
| `GET` | `/api/products` | Public | List products (with `?category=`, `?q=`, `?seller_id=`) |
| `GET` | `/api/products/{id}` | Public | Detailed product view with reviews & seller data |
| `POST` | `/api/products` | Seller | Create new product listing |
| `PUT` | `/api/products/{id}` | Seller/Admin | Update product details, price, or stock |
| `DELETE` | `/api/products/{id}`| Seller/Admin | Deactivate or delete product |
| `GET` | `/api/cart` | Buyer | Fetch active cart items, subtotal, and total |
| `POST` | `/api/cart` | Buyer | Add item to cart with stock ceiling check |
| `PUT` | `/api/cart/{id}` | Buyer | Update item quantity |
| `DELETE` | `/api/cart/{id}` | Buyer | Remove specific item from cart |
| `POST` | `/api/orders` | Buyer | Place order: validates stock, creates order, decrements inventory |
| `GET` | `/api/orders` | Auth | Buyer views own orders; Seller views incoming item sales |
| `GET` | `/api/orders/{id}` | Auth | Detailed invoice breakdown for specific order |
| `POST` | `/api/reviews` | Buyer | Submit 1–5 star verified review |
| `GET` | `/api/products/{id}/reviews`| Public | Fetch all reviews and average rating |
| `GET` | `/api/admin/users` | Admin | List all registered accounts |
| `GET` | `/api/admin/stats` | Admin | View platform GMV, order count, and user totals |
| `POST` | `/api/chat` | Public | AI Shopping & Navigation Assistant |

---

## 6. How to Build & Run on Windows + VS Code

### Prerequisites:
1. **Visual Studio 2022** (with "Desktop development with C++") or **MSYS2 / MinGW-w64**.
2. **CMake** (v3.16 or higher).
3. **vcpkg** (recommended for C++ package management) or precompiled Drogon binaries.
4. **PostgreSQL 14+** installed and running on port 5432.

### Step 1: Install Dependencies via vcpkg
```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install drogon[ctl,orm] openssl jsoncpp --triplet x64-windows
```

### Step 2: Set Up PostgreSQL Database
Open pgAdmin or PowerShell:
```sql
CREATE DATABASE sanghavimart_db;
```
Run the SQL schema:
```powershell
psql -U postgres -d sanghavimart_db -f sql/schema.sql
```

### Step 3: Configure Database Credentials
Edit `config.json`:
```json
"db_clients": [
  {
    "name": "default",
    "rdbms": "postgresql",
    "host": "127.0.0.1",
    "port": 5432,
    "dbname": "sanghavimart_db",
    "user": "postgres",
    "passwd": "your_password"
  }
]
```

### Step 4: Build using CMake in VS Code
Open the `SanghaviMart` folder in VS Code:
```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Step 5: Run the Server
```powershell
.\Release\SanghaviMart.exe
```
Open your browser at `http://localhost:8080`.

---

## 7. How to Upload to GitHub

```powershell
git init
git add .
git commit -m "Initial commit: SanghaviMart C++ Drogon e-commerce capstone project"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/SanghaviMart.git
git push -u origin main
```

---

## 8. Capstone Presentation & Defense Highlights

- **Why C++20 for E-Commerce?**:
  Traditional e-commerce backends in Python or Node.js encounter memory and I/O bottlenecks under massive concurrent flash-sale traffic. Drogon uses asynchronous non-blocking event loops built upon `epoll`/`kqueue` (via libtrantor), capable of serving over 100,000 requests per second with negligible memory footprint.
- **Race Condition Prevention in Checkout**:
  The checkout routine uses PostgreSQL transactions and conditional atomic updates (`stock_quantity = stock_quantity - requested_quantity WHERE stock_quantity >= requested_quantity`) to eliminate overselling during simultaneous checkouts.
- **Role Isolation**:
  Sellers can only modify their own inventory; admins have supervisory oversight; buyers can only review items they have purchased.

---

## 9. API v1 (spec-canonical, sessions + envelope)

Base: `/api/v1`. Success: `{"success":true,"data":{...},"error":null}`.
Error: `{"success":false,"data":null,"error":{"code":"...","message":"..."}}`
with 200/201/400/401/403/404/409/500. Auth is server-side sessions
(`POST /api/v1/auth/register|login`, `POST /api/v1/auth/logout`,
`GET /api/v1/auth/me`); roles `BUYER|SELLER|ADMIN` (admin seeded, no signup).
Money is integer `price_cents`. Key routes:

| Method | Endpoint | Role |
|---|---|---|
| `GET` | `/api/v1/health` (runs `SELECT 1`) | public |
| `GET` | `/api/v1/products?q=&category=` | public |
| `POST/PUT/DELETE` | `/api/v1/products[/{id}]` | SELLER/ADMIN (own listings) |
| `GET/POST/PUT/DELETE` | `/api/v1/cart[/{productId}]` | BUYER |
| `POST` | `/api/v1/orders/checkout` (mock payment, txn) | BUYER |
| `GET` | `/api/v1/orders[/{id}]` (role-scoped) | auth |
| `PUT` | `/api/v1/orders/{id}/status` (`PENDING→CONFIRMED→SHIPPED→DELIVERED`, `CANCELLED`) | SELLER/ADMIN |
| `POST` | `/api/v1/reviews` (verified purchase, 1–5, one/user/product) | BUYER |
| `GET` | `/api/v1/admin/users\|orders\|stats`, `DELETE /api/v1/admin/products/{id}` | ADMIN (403 otherwise) |
| `POST` | `/api/v1/chat` (10/min/session, cached, Gemini or mock fallback) | public |

Legacy `/api/*` (JWT) is retained for existing vanilla pages. Migrations
`db/migrations/V1–V5` run transactionally via `MigrationService`; V5 bridges
legacy columns to canonical cents/UPPER vocabularies with sync triggers.
Build: `cmake --preset linux-release && cmake --build build/release`
(requires vcpkg manifest + PostgreSQL 15+). Tests: `ctest --test-dir
build/debug` (unit always; PG/concurrency need `POSTGRES_TEST_DB`).

---

## 10. Operations (PostgreSQL-only)

PostgreSQL 15+ is the only supported database — no SQLite paths exist.

```bash
# 1. Database
createdb sanghavimart_db
# Migrations V1-V5 apply automatically at startup via DatabasePlugin,
# then load demo data:
psql -d sanghavimart_db -f SanghaviMart/db/seed.sql

# 2. Environment (never commit secrets)
cp SanghaviMart/.env.example .env   # set POSTGRES_*, GEMINI_API_KEY

# 3. Build & test (Linux + vcpkg)
export VCPKG_ROOT=~/vcpkg
cmake --preset linux-release && cmake --build build/release
POSTGRES_TEST_DB=sanghavimart_test_db ctest --test-dir build/debug

# 4. Run
./build/release/SanghaviMart            # serves ./frontend + /api/*
curl localhost:8080/api/v1/health      # {"status":"UP","db":"UP"}
```

Troubleshooting: health reports `"db":"DOWN"` when PostgreSQL is unreachable;
the server still boots (migrations log and continue). Admin has no signup —
seed `admin@sanghavimart.com` and rotate its hash immediately. Old lowercase
JWTs were invalidated by the V5 UPPER normalization; users re-login.
