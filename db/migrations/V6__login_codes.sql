-- V6: one-time email login codes (OTP), SQLite dialect. Same contract as postgres V6.
CREATE TABLE IF NOT EXISTS login_codes(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  email TEXT NOT NULL,
  code_hash TEXT NOT NULL,
  expires_at_sec BIGINT NOT NULL,
  attempts INTEGER NOT NULL DEFAULT 0,
  used INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);
CREATE INDEX IF NOT EXISTS idx_login_codes_email ON login_codes(email);
