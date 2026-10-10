-- Seed/demo data. Admin row inserted by migration (no admin signup flow).
-- Demo password for all seed users: password123 (Argon2id hash filled at deploy).
-- Admin: admin@sanghavimart.local / SELLER demo: seller@sanghavimart.local / BUYER: buyer@sanghavimart.local
INSERT INTO products(seller_id,name,description,price_cents,stock_qty,category,image_url) VALUES
 (2,'"'"'Steel Water Bottle 1L'"'"','"'"'Double-wall insulated bottle, 24h cold.'"'"',89900,120,'"'"'home'"'"','"'"''"'"'),
 (2,'"'"'Cotton Kurta - Indigo'"'"','"'"'Handloom cotton kurta, sizes S-XXL.'"'"',129900,60,'"'"'fashion'"'"','"'"''"'"'),
 (2,'"'"'USB-C Fast Charger 65W'"'"','"'"'GaN charger with 2C+1A ports.'"'"',249900,200,'"'"'electronics'"'"','"'"''"'"'),
 (2,'"'"'Filter Coffee Powder 500g'"'"','"'"'80/20 coffee-chicory blend.'"'"',34900,300,'"'"'grocery'"'"','"'"''"'"');
