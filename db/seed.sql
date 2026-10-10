-- SanghaviMart demo seed data (Postgres + SQLite compatible).
-- Creates a demo SELLER if missing, then 4 demo products for that seller.
-- Demo password for seed logins: password123 (Argon2id hash filled at deploy).
INSERT INTO users(name,email,password_hash,role)
SELECT 'Demo Seller','seller@sanghavimart.local','$argon2id$v=19$m=65536,t=2,p=1$REPLACE_ME_AT_DEPLOY$REPLACE_ME_AT_DEPLOY_REPLACE_ME','SELLER'
WHERE NOT EXISTS (SELECT 1 FROM users WHERE email='seller@sanghavimart.local');
INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)
SELECT id,'Steel Water Bottle 1L','Double-wall insulated bottle, 24h cold.',89900,120,'home',''
FROM users WHERE email='seller@sanghavimart.local';
INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)
SELECT id,'Cotton Kurta - Indigo','Handloom cotton kurta, sizes S-XXL.',129900,60,'fashion',''
FROM users WHERE email='seller@sanghavimart.local';
INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)
SELECT id,'USB-C Fast Charger 65W','GaN charger with 2C+1A ports.',249900,200,'electronics',''
FROM users WHERE email='seller@sanghavimart.local';
INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url)
SELECT id,'Filter Coffee Powder 500g','80/20 coffee-chicory blend.',34900,300,'grocery',''
FROM users WHERE email='seller@sanghavimart.local';
