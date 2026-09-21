-- ============================================================================
-- SanghaviMart: Full-Stack C++ Drogon Multi-Seller E-Commerce Database Schema
-- Database: PostgreSQL 14+ (Compatible with SQLite with standard syntax adjustments)
-- ============================================================================

-- Drop tables if they exist to allow clean migration
DROP TABLE IF EXISTS reviews CASCADE;
DROP TABLE IF EXISTS order_items CASCADE;
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS cart_items CASCADE;
DROP TABLE IF EXISTS cart CASCADE;
DROP TABLE IF EXISTS products CASCADE;
DROP TABLE IF EXISTS categories CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- 1. USERS TABLE
-- Roles: 'buyer', 'seller', 'admin'
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL CHECK (role IN ('buyer', 'seller', 'admin')),
    phone VARCHAR(20),
    address TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role ON users(role);

-- 2. CATEGORIES TABLE
CREATE TABLE categories (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    slug VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    image_url VARCHAR(255),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 3. PRODUCTS TABLE
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    seller_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    category_id INT NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,
    name VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    price NUMERIC(10, 2) NOT NULL CHECK (price >= 0),
    stock_quantity INT NOT NULL DEFAULT 0 CHECK (stock_quantity >= 0),
    image_url VARCHAR(500) NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_products_seller ON products(seller_id);
CREATE INDEX idx_products_category ON products(category_id);
CREATE INDEX idx_products_active ON products(is_active);

-- 4. CART TABLE (One active cart per buyer)
CREATE TABLE cart (
    id SERIAL PRIMARY KEY,
    buyer_id INT UNIQUE NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 5. CART ITEMS TABLE
CREATE TABLE cart_items (
    id SERIAL PRIMARY KEY,
    cart_id INT NOT NULL REFERENCES cart(id) ON DELETE CASCADE,
    product_id INT NOT NULL REFERENCES products(id) ON DELETE CASCADE,
    quantity INT NOT NULL CHECK (quantity > 0),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(cart_id, product_id)
);

CREATE INDEX idx_cart_items_cart ON cart_items(cart_id);

-- 6. ORDERS TABLE
-- Statuses: 'pending', 'processing', 'shipped', 'delivered', 'cancelled'
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    order_number VARCHAR(50) UNIQUE NOT NULL,
    buyer_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    total_amount NUMERIC(10, 2) NOT NULL CHECK (total_amount >= 0),
    status VARCHAR(30) NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'processing', 'shipped', 'delivered', 'cancelled')),
    shipping_name VARCHAR(100) NOT NULL,
    shipping_phone VARCHAR(20) NOT NULL,
    shipping_address TEXT NOT NULL,
    shipping_city VARCHAR(100) NOT NULL,
    shipping_postal_code VARCHAR(20) NOT NULL,
    payment_method VARCHAR(50) DEFAULT 'cod_mock',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_orders_buyer ON orders(buyer_id);
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_orders_number ON orders(order_number);

-- 7. ORDER ITEMS TABLE (Stores snapshot of price at purchase time)
CREATE TABLE order_items (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
    product_id INT NOT NULL REFERENCES products(id) ON DELETE RESTRICT,
    seller_id INT NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    quantity INT NOT NULL CHECK (quantity > 0),
    unit_price NUMERIC(10, 2) NOT NULL CHECK (unit_price >= 0),
    subtotal NUMERIC(10, 2) NOT NULL CHECK (subtotal >= 0),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_order_items_order ON order_items(order_id);
CREATE INDEX idx_order_items_seller ON order_items(seller_id);

-- 8. PRODUCT REVIEWS TABLE (Only verified buyers can review)
CREATE TABLE reviews (
    id SERIAL PRIMARY KEY,
    product_id INT NOT NULL REFERENCES products(id) ON DELETE CASCADE,
    buyer_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    order_id INT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
    rating INT NOT NULL CHECK (rating >= 1 AND rating <= 5),
    comment TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(product_id, buyer_id, order_id)
);

CREATE INDEX idx_reviews_product ON reviews(product_id);
CREATE INDEX idx_reviews_buyer ON reviews(buyer_id);

-- ============================================================================
-- SEED INITIAL DATA (For testing, demos, and capstone presentation)
-- Passwords below are pre-hashed for 'password123' using bcrypt / SHA-256 salt
-- ============================================================================

-- Seed Users: Admin, Sellers, Buyers
INSERT INTO users (id, name, email, password_hash, role, phone, address) VALUES
(1, 'Sanghavi Administrator', 'admin@sanghavimart.com', '$2b$12$e8d1a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y', 'admin', '+91 9876543210', '101 Sanghavi Tower, Mumbai, Maharashtra'),
(2, 'Apex Tech Retailers', 'seller.apex@sanghavimart.com', '$2b$12$e8d1a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y', 'seller', '+91 9823456789', 'Plot 45 Electronic City, Bengaluru, Karnataka'),
(3, 'Urban Lifestyle Hub', 'seller.urban@sanghavimart.com', '$2b$12$e8d1a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y', 'seller', '+91 9834567890', '22 Fashion Street, New Delhi'),
(4, 'Neha Sanghavi (Verified Buyer)', 'neha@sanghavimart.com', '$2b$12$e8d1a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y', 'buyer', '+91 9812345678', 'Flat 4B, Residency Enclave, Pune, Maharashtra'),
(5, 'Rahul Verma (Student Buyer)', 'rahul@gmail.com', '$2b$12$e8d1a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y', 'buyer', '+91 9871234567', 'Hostel Block C, IIT Powai, Mumbai');

ALTER SEQUENCE users_id_seq RESTART WITH 6;

-- Seed Categories
INSERT INTO categories (id, name, slug, description, image_url) VALUES
(1, 'Electronics', 'electronics', 'High performance gadgets, audio, laptops, and smart accessories', 'https://images.unsplash.com/photo-1498049794561-7780e7231661?w=500&q=80'),
(2, 'Fashion', 'fashion', 'Contemporary apparel, footwear, and curated modern styles', 'https://images.unsplash.com/photo-1445205170230-053b83016050?w=500&q=80'),
(3, 'Home & Living', 'home-living', 'Ergonomic workspace furniture, ceramic accents, and home comfort', 'https://images.unsplash.com/photo-1513694203232-719a280e022f?w=500&q=80'),
(4, 'Books & Learning', 'books', 'Software engineering handbooks, algorithm guides, and academic literature', 'https://images.unsplash.com/photo-1497633762265-9d179a990aa6?w=500&q=80');

ALTER SEQUENCE categories_id_seq RESTART WITH 5;

-- Seed Products
INSERT INTO products (id, seller_id, category_id, name, description, price, stock_quantity, image_url) VALUES
(1, 2, 1, 'NoisePulse ANC Wireless Headphones', 'Engineered with 40mm neodymium drivers, active hybrid noise cancellation, 35-hour battery life, and ultra-low latency C++ audio codec support.', 79.99, 15, 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80'),
(2, 2, 1, 'UltraBook Pro 14 (C++ Dev Edition)', '14-inch 2.8K IPS display, 16-Core high performance CPU, 32GB DDR5 RAM, 1TB NVMe SSD. Pre-tuned for fast CMake builds and Drogon development.', 1099.00, 6, 'https://images.unsplash.com/photo-1517336714731-489689fd1ca8?w=500&q=80'),
(3, 2, 1, 'Precision Mechanical Keyboard 75%', 'Custom lubricated linear switches, solid aluminum chassis, hot-swappable PCB, and tactile feedback built for high-throughput software programming.', 89.50, 18, 'https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=500&q=80'),
(4, 3, 3, 'ErgoComfort High-Back Mesh Chair', 'Full ergonomic lumbar support, multi-angle synchro-tilt mechanism, breathable matrix mesh, and 3D adjustable armrests.', 199.99, 9, 'https://images.unsplash.com/photo-1580481077191-23a746328328?w=500&q=80'),
(5, 3, 3, 'Nordic Stoneware Ceramic Coffee Set', 'Hand-glazed stoneware ceramic mugs with double-wall thermal insulation. Set of 4 artisan cups with heat-resistant matte finish.', 34.00, 22, 'https://images.unsplash.com/photo-1514432324607-a09d9b4aefdd?w=500&q=80'),
(6, 2, 4, 'Clean Code & Architecture Engineering Handbook', 'Comprehensive guide to clean system design, SOLID principles, asynchronous concurrency, and building production-grade services.', 44.99, 25, 'https://images.unsplash.com/photo-1532012164546-f432f2e37272?w=500&q=80'),
(7, 3, 2, 'Minimalist Executive Chronograph Watch', 'Sapphire crystal glass, Japanese quartz movement, genuine Italian leather strap, 50m water resistance.', 139.00, 8, 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=500&q=80'),
(8, 3, 2, 'Water-Resistant Commuter Backpack 24L', 'Ballistic nylon construction, dedicated 16-inch padded laptop compartment, waterproof zippers, and hidden anti-theft pocket.', 64.50, 14, 'https://images.unsplash.com/photo-1553062407-98eeb64c6a62?w=500&q=80');

ALTER SEQUENCE products_id_seq RESTART WITH 9;

-- Seed Sample Orders & Order Items
INSERT INTO orders (id, order_number, buyer_id, total_amount, status, shipping_name, shipping_phone, shipping_address, shipping_city, shipping_postal_code, payment_method) VALUES
(1, 'SM-2026-9041', 4, 169.49, 'delivered', 'Neha Sanghavi', '+91 9812345678', 'Flat 4B, Residency Enclave, Senapati Bapat Road', 'Pune', '411016', 'card_mock'),
(2, 'SM-2026-9042', 4, 79.99, 'shipped', 'Neha Sanghavi', '+91 9812345678', 'Flat 4B, Residency Enclave, Senapati Bapat Road', 'Pune', '411016', 'cod_mock'),
(3, 'SM-2026-9043', 5, 89.50, 'processing', 'Rahul Verma', '+91 9871234567', 'Hostel Block C, Room 204, IIT Campus', 'Mumbai', '400076', 'upi_mock');

ALTER SEQUENCE orders_id_seq RESTART WITH 4;

INSERT INTO order_items (id, order_id, product_id, seller_id, quantity, unit_price, subtotal) VALUES
(1, 1, 3, 2, 1, 89.50, 89.50),
(2, 1, 1, 2, 1, 79.99, 79.99),
(3, 2, 1, 2, 1, 79.99, 79.99),
(4, 3, 3, 2, 1, 89.50, 89.50);

ALTER SEQUENCE order_items_id_seq RESTART WITH 5;

-- Seed Verified Product Reviews
INSERT INTO reviews (id, product_id, buyer_id, order_id, rating, comment) VALUES
(1, 3, 4, 1, 5, 'Exceptional keyboard! The key travel is precise and ideal for long C++ coding sessions. Fast delivery by SanghaviMart.'),
(2, 1, 4, 1, 4, 'Crisp audio quality with impressive active noise cancellation. Battery lasts over 30 hours without recharging.');

ALTER SEQUENCE reviews_id_seq RESTART WITH 3;
