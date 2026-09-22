-- V3: additional search/browse indexes (PostgreSQL).
CREATE INDEX IF NOT EXISTS idx_products_name_lower ON products(lower(name));
CREATE INDEX IF NOT EXISTS idx_products_category_lower ON products(lower(category));
CREATE INDEX IF NOT EXISTS idx_orders_buyer_status ON orders(buyer_id, status);
CREATE INDEX IF NOT EXISTS idx_order_items_order_product ON order_items(order_id, product_id);
