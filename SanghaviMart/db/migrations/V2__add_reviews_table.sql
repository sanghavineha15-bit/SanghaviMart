-- V2: reviews table hardening (comment length + unique per user/product).
ALTER TABLE reviews ADD CONSTRAINT chk_reviews_comment_len
    CHECK (char_length(comment) >= 1 AND char_length(comment) <= 2000);
CREATE INDEX IF NOT EXISTS idx_reviews_product_user ON reviews(product_id, user_id);
