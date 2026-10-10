INSERT INTO users(name,email,password_hash,role) VALUES
 ('SanghaviMart Admin','admin@sanghavimart.local',
  '$argon2id$v=19$m=65536,t=2,p=1$REPLACE_ME_AT_DEPLOY$REPLACE_ME_AT_DEPLOY_REPLACE_ME','ADMIN')
ON CONFLICT(email) DO NOTHING;

