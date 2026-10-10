#!/bin/bash
# Scripted smoke test against /api/v1 (run while binary is up on :8080).
set -e
B=http://localhost:8080/api/v1
C=/tmp/sm.cookie
curl -sf $B/health
curl -sf -c $C -X POST $B/auth/register -d '"'"'{"name":"T Buyer","email":"t.buyer@local","password":"password123","role":"BUYER"}'"'"' -H '"'"'Content-Type: application/json'"'"'
curl -sf -b $C $B/products
echo SMOKE-OK
