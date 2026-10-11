-- V6: one-time email login codes (OTP). Only SHA-256 hashes stored, never the code.
-- expires_at_sec is epoch seconds; attempts caps guesses at 5; used marks single-use.
CREATE TABLE IF NOT EXISTS login_codes(
  id BIGSERIAL PRIMARY KEY,
  email TEXT NOT NULL,
  code_hash TEXT NOT NULL,
  expires_at_sec BIGINT NOT NULL,
  attempts INTEGER NOT NULL DEFAULT 0,
  used INTEGER NOT NULL DEFAULT 0,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW());
CREATE INDEX IF NOT EXISTS idx_login_codes_email ON login_codes(email);
