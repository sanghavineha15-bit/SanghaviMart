-- ============================================================================
-- V5: Legacy backfill — bridge legacy schema (sql/schema.sql) to spec-canonical
-- columns WITHOUT breaking legacy controllers (dual-write transition).
--
-- What it does (all idempotent, single transaction via MigrationService):
--  1. Adds spec-canonical columns (price_cents, stock_qty, category, user_id,
--     total_amount_cents, unit_price_cents) where missing.
--  2. Backfills them from legacy columns (price*100, stock_quantity, ...).
--  3. Relaxes legacy NOT NULLs that the new V1 repositories don't write
--     (category_id, price, stock_quantity, order_number, shipping_*,
--     order_items.seller_id/subtotal, cart_items.cart_id, reviews.buyer_id/
--     order_id) so BOTH stacks can write.
--  4. Normalizes users.role and orders.status to UPPERCASE vocabularies and
--     widens legacy CHECKs to accept the canonical values.
--  5. Installs sync triggers (legacy -> canonical direction) so legacy writes
--     remain visible to V1 reads. Canonical -> legacy sync is best-effort for
--     display columns only.
-- PostgreSQL 15+. No SQLite. All statements IF NOT EXISTS / guarded.
-- ============================================================================

-- ---------------------------------------------------------------- users ---
DO $$ BEGIN
    -- Normalize existing lowercase roles to canonical UPPER values first,
    -- so the widened CHECK below never fails on legacy rows.
    UPDATE users SET role = UPPER(role)
    WHERE role IN ('buyer', 'seller', 'admin');
EXCEPTION WHEN OTHERS THEN NULL;
END $$;

DO $$ DECLARE c RECORD; BEGIN
    -- Drop any legacy role CHECK that only allows lowercase, then add the
    -- canonical one. Name-agnostic: drop CHECKs mentioning 'buyer'.
    FOR c IN SELECT conname FROM pg_constraint
             WHERE conrelid = 'users'::regclass AND contype = 'c'
               AND conname <> 'chk_users_role_canonical'
               AND pg_get_constraintdef(oid) ILIKE '%buyer%' LOOP
        EXECUTE format('ALTER TABLE users DROP CONSTRAINT %I', c.conname);
    END LOOP;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint
                   WHERE conrelid = 'users'::regclass AND conname = 'chk_users_role_canonical') THEN
        ALTER TABLE users ADD CONSTRAINT chk_users_role_canonical
            CHECK (role IN ('BUYER', 'SELLER', 'ADMIN'));
    END IF;
END $$;

-- ---------------------------------------------------------------- products -
ALTER TABLE products ADD COLUMN IF NOT EXISTS price_cents BIGINT;
ALTER TABLE products ADD COLUMN IF NOT EXISTS stock_qty INT;
ALTER TABLE products ADD COLUMN IF NOT EXISTS category VARCHAR(100) DEFAULT 'General';

-- Relax legacy NOT NULLs the V1 stack doesn't write (keeps both stacks working).
ALTER TABLE products ALTER COLUMN category_id DROP NOT NULL;
ALTER TABLE products ALTER COLUMN price DROP NOT NULL;
ALTER TABLE products ALTER COLUMN stock_quantity DROP NOT NULL;
ALTER TABLE products ALTER COLUMN image_url DROP NOT NULL;

-- Backfill canonical from legacy (only where canonical is NULL).
UPDATE products SET price_cents = (price * 100)::BIGINT
WHERE price_cents IS NULL AND price IS NOT NULL;
UPDATE products SET stock_qty = stock_quantity
WHERE stock_qty IS NULL AND stock_quantity IS NOT NULL;
-- Category text from categories table where possible (legacy FK may be NULL now).
DO $$ BEGIN
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'categories') THEN
        UPDATE products p SET category = c.name
        FROM categories c
        WHERE p.category IS NULL AND p.category_id = c.id;
    END IF;
END $$;
UPDATE products SET category = 'General' WHERE category IS NULL;
UPDATE products SET price_cents = 0 WHERE price_cents IS NULL;
UPDATE products SET stock_qty = 0 WHERE stock_qty IS NULL;

DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'chk_products_price_cents_nonneg') THEN
        ALTER TABLE products ADD CONSTRAINT chk_products_price_cents_nonneg
            CHECK (price_cents >= 0);
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'chk_products_stock_qty_nonneg') THEN
        ALTER TABLE products ADD CONSTRAINT chk_products_stock_qty_nonneg
            CHECK (stock_qty >= 0);
    END IF;
END $$;
CREATE INDEX IF NOT EXISTS idx_products_seller ON products(seller_id);
CREATE INDEX IF NOT EXISTS idx_products_category_text ON products(category);

-- Sync trigger: legacy writes (price/stock_quantity) propagate to canonical.
CREATE OR REPLACE FUNCTION trg_products_legacy_sync() RETURNS trigger AS $$
BEGIN
    IF NEW.price IS NOT NULL AND (OLD.price IS NULL OR NEW.price <> OLD.price) THEN
        NEW.price_cents := (NEW.price * 100)::BIGINT;
    END IF;
    IF NEW.stock_quantity IS NOT NULL AND (OLD.stock_quantity IS NULL OR NEW.stock_quantity <> OLD.stock_quantity) THEN
        NEW.stock_qty := NEW.stock_quantity;
    END IF;
    IF NEW.price_cents IS NULL AND NEW.price IS NOT NULL THEN
        NEW.price_cents := (NEW.price * 100)::BIGINT;
    END IF;
    IF NEW.stock_qty IS NULL AND NEW.stock_quantity IS NOT NULL THEN
        NEW.stock_qty := NEW.stock_quantity;
    END IF;
    -- Canonical -> legacy (display parity for legacy readers).
    IF NEW.price IS NULL AND NEW.price_cents IS NOT NULL THEN
        NEW.price := NEW.price_cents / 100.0;
    END IF;
    IF NEW.stock_quantity IS NULL AND NEW.stock_qty IS NOT NULL THEN
        NEW.stock_quantity := NEW.stock_qty;
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS trg_products_sync ON products;
CREATE TRIGGER trg_products_sync BEFORE INSERT OR UPDATE ON products
    FOR EACH ROW EXECUTE FUNCTION trg_products_legacy_sync();

-- ------------------------------------------------------------------ orders -
ALTER TABLE orders ADD COLUMN IF NOT EXISTS total_amount_cents BIGINT;

-- Relax legacy NOT NULLs the V1 checkout doesn't write.
ALTER TABLE orders ALTER COLUMN order_number DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN shipping_name DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN shipping_phone DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN shipping_address DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN shipping_city DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN shipping_postal_code DROP NOT NULL;
ALTER TABLE orders ALTER COLUMN total_amount DROP NOT NULL;

UPDATE orders SET status = 'CONFIRMED' WHERE UPPER(status) = 'PROCESSING';
UPDATE orders SET status = UPPER(status)
WHERE UPPER(status) IN ('PENDING', 'CONFIRMED', 'SHIPPED', 'DELIVERED', 'CANCELLED');
UPDATE orders SET total_amount_cents = (total_amount * 100)::BIGINT
WHERE total_amount_cents IS NULL AND total_amount IS NOT NULL;
UPDATE orders SET total_amount_cents = 0 WHERE total_amount_cents IS NULL;

-- Auto-generate order_number for V1 rows that omit it (legacy column kept).
CREATE OR REPLACE FUNCTION trg_orders_defaults() RETURNS trigger AS $$
BEGIN
    IF NEW.order_number IS NULL THEN
        NEW.order_number := 'SM-' || to_char(now(), 'YYYY') || '-' || lpad(floor(random() * 9000 + 1000)::text, 4, '0');
    END IF;
    IF NEW.total_amount IS NULL AND NEW.total_amount_cents IS NOT NULL THEN
        NEW.total_amount := NEW.total_amount_cents / 100.0;
    END IF;
    IF NEW.total_amount_cents IS NULL AND NEW.total_amount IS NOT NULL THEN
        NEW.total_amount_cents := (NEW.total_amount * 100)::BIGINT;
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS trg_orders_defaults ON orders;
CREATE TRIGGER trg_orders_defaults BEFORE INSERT ON orders
    FOR EACH ROW EXECUTE FUNCTION trg_orders_defaults();

DO $$ DECLARE c RECORD; BEGIN
    FOR c IN SELECT conname FROM pg_constraint
             WHERE conrelid = 'orders'::regclass AND contype = 'c'
               AND conname <> 'chk_orders_status_canonical'
               AND pg_get_constraintdef(oid) ILIKE '%pending%' LOOP
        EXECUTE format('ALTER TABLE orders DROP CONSTRAINT %I', c.conname);
    END LOOP;
    IF NOT EXISTS (SELECT 1 FROM pg_constraint
                   WHERE conrelid = 'orders'::regclass AND conname = 'chk_orders_status_canonical') THEN
        ALTER TABLE orders ADD CONSTRAINT chk_orders_status_canonical
            CHECK (status IN ('PENDING', 'CONFIRMED', 'SHIPPED', 'DELIVERED', 'CANCELLED'));
    END IF;
END $$;
CREATE INDEX IF NOT EXISTS idx_orders_buyer ON orders(buyer_id);
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);

-- ------------------------------------------------------------- order_items -
ALTER TABLE order_items ADD COLUMN IF NOT EXISTS unit_price_cents BIGINT;
ALTER TABLE order_items ALTER COLUMN seller_id DROP NOT NULL;
ALTER TABLE order_items ALTER COLUMN subtotal DROP NOT NULL;
ALTER TABLE order_items ALTER COLUMN unit_price DROP NOT NULL;

UPDATE order_items SET unit_price_cents = (unit_price * 100)::BIGINT
WHERE unit_price_cents IS NULL AND unit_price IS NOT NULL;
UPDATE order_items SET unit_price_cents = 0 WHERE unit_price_cents IS NULL;

CREATE OR REPLACE FUNCTION trg_order_items_sync() RETURNS trigger AS $$
BEGIN
    IF NEW.unit_price IS NULL AND NEW.unit_price_cents IS NOT NULL THEN
        NEW.unit_price := NEW.unit_price_cents / 100.0;
    END IF;
    IF NEW.subtotal IS NULL AND NEW.unit_price_cents IS NOT NULL AND NEW.quantity IS NOT NULL THEN
        NEW.subtotal := (NEW.unit_price_cents * NEW.quantity) / 100.0;
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS trg_order_items_sync ON order_items;
CREATE TRIGGER trg_order_items_sync BEFORE INSERT OR UPDATE ON order_items
    FOR EACH ROW EXECUTE FUNCTION trg_order_items_sync();

CREATE INDEX IF NOT EXISTS idx_order_items_order ON order_items(order_id);
CREATE INDEX IF NOT EXISTS idx_order_items_product ON order_items(product_id);
CREATE INDEX IF NOT EXISTS idx_order_items_seller ON order_items(seller_id);

-- ------------------------------------------------------------- cart_items --
ALTER TABLE cart_items ADD COLUMN IF NOT EXISTS user_id INT REFERENCES users(id) ON DELETE CASCADE;
ALTER TABLE cart_items ALTER COLUMN cart_id DROP NOT NULL;

-- Backfill user_id from legacy cart table (cart.buyer_id).
DO $$ BEGIN
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'cart') THEN
        UPDATE cart_items ci SET user_id = c.buyer_id
        FROM cart c WHERE ci.user_id IS NULL AND ci.cart_id = c.id;
    END IF;
END $$;
CREATE INDEX IF NOT EXISTS idx_cart_items_user ON cart_items(user_id);
CREATE INDEX IF NOT EXISTS idx_cart_items_product ON cart_items(product_id);

-- Keep legacy cart_id populated for V1 writes (dual-write both keys).
CREATE OR REPLACE FUNCTION trg_cart_items_user_sync() RETURNS trigger AS $$
DECLARE v_cart INT;
BEGIN
    IF NEW.user_id IS NOT NULL AND NEW.cart_id IS NULL THEN
        SELECT id INTO v_cart FROM cart WHERE buyer_id = NEW.user_id;
        IF NOT FOUND THEN
            INSERT INTO cart (buyer_id) VALUES (NEW.user_id) RETURNING id INTO v_cart;
        END IF;
        NEW.cart_id := v_cart;
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS trg_cart_items_user_sync ON cart_items;
CREATE TRIGGER trg_cart_items_user_sync BEFORE INSERT ON cart_items
    FOR EACH ROW EXECUTE FUNCTION trg_cart_items_user_sync();

-- ----------------------------------------------------------------- reviews -
ALTER TABLE reviews ADD COLUMN IF NOT EXISTS user_id INT REFERENCES users(id) ON DELETE CASCADE;
ALTER TABLE reviews ALTER COLUMN buyer_id DROP NOT NULL;
ALTER TABLE reviews ALTER COLUMN order_id DROP NOT NULL;

UPDATE reviews SET user_id = buyer_id WHERE user_id IS NULL AND buyer_id IS NOT NULL;

CREATE OR REPLACE FUNCTION trg_reviews_sync() RETURNS trigger AS $$
BEGIN
    IF NEW.buyer_id IS NULL AND NEW.user_id IS NOT NULL THEN
        NEW.buyer_id := NEW.user_id;
    END IF;
    IF NEW.user_id IS NULL AND NEW.buyer_id IS NOT NULL THEN
        NEW.user_id := NEW.buyer_id;
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS trg_reviews_sync ON reviews;
CREATE TRIGGER trg_reviews_sync BEFORE INSERT OR UPDATE ON reviews
    FOR EACH ROW EXECUTE FUNCTION trg_reviews_sync();

DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname = 'uq_reviews_product_user') THEN
        ALTER TABLE reviews ADD CONSTRAINT uq_reviews_product_user UNIQUE (product_id, user_id);
    END IF;
EXCEPTION WHEN OTHERS THEN NULL;
END $$;
CREATE INDEX IF NOT EXISTS idx_reviews_product ON reviews(product_id);
CREATE INDEX IF NOT EXISTS idx_reviews_user ON reviews(user_id);
