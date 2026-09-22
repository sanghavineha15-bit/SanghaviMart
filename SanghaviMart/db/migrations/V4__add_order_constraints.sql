-- V4: order status transition guard + stock non-negative enforcement.
-- Application enforces PENDING->CONFIRMED->SHIPPED->DELIVERED (+CANCELLED).
-- DB keeps CHECK + non-negative stock as last line of defense.
DO $$ BEGIN
  IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname='chk_products_stock_nonneg') THEN
    ALTER TABLE products ADD CONSTRAINT chk_products_stock_nonneg CHECK (stock_qty >= 0);
  END IF;
END $$;
DO $$ BEGIN
  IF NOT EXISTS (SELECT 1 FROM pg_constraint WHERE conname='chk_orders_total_nonneg') THEN
    ALTER TABLE orders ADD CONSTRAINT chk_orders_total_nonneg CHECK (total_amount_cents >= 0);
  END IF;
END $$;
