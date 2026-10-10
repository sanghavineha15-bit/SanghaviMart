-- V4: admin seed row. Password MUST be rotated at deploy (Argon2id of a strong secret).
-- Placeholder hash below is crypto_pwhash_str('"'"'Admin#2026!ChangeMe'"'"'); replace via rotate_admin.py or SQL.
INSERT INTO users(name,email,password_hash,role) VALUES
 ('"'"'SanghaviMart Admin'"'"','"'"'admin@sanghavimart.local'"'"',
  '"'"'$argon2id$v=19$m=65536,t=2,p=1$REPLACE_ME_AT_DEPLOY$REPLACE_ME_AT_DEPLOY_REPLACE_ME'"'"','"'"'ADMIN'"'"')
ON CONFLICT(email) DO NOTHING;
