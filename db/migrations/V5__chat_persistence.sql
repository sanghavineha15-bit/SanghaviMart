-- V5: chat rate-limit + reply cache persistence (SQLite dialect).
-- hit_at_sec is epoch seconds so pruning is portable across backends.
CREATE TABLE IF NOT EXISTS chat_hits(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  session_key TEXT NOT NULL,
  hit_at_sec BIGINT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);
CREATE INDEX IF NOT EXISTS idx_chat_hits_key ON chat_hits(session_key);
CREATE TABLE IF NOT EXISTS chat_cache(
  session_key TEXT NOT NULL,
  question TEXT NOT NULL,
  reply TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY(session_key, question));
