"""JWT wire-contract test (stdlib only): pins the exact token format that
src/util/JwtUtil.cc implements, so any reimplementation stays compatible.
- HS256, base64url without padding, claims {sub, role, exp}
- tampered signature rejected, expired token rejected, wrong shape rejected
"""
import base64, hashlib, hmac, json, time

def b64e(b: bytes) -> str:
    return base64.urlsafe_b64encode(b).rstrip(b"=").decode()

def issue(uid, role, secret, ttl=43200):
    h = b64e(b'{"alg":"HS256","typ":"JWT"}')
    p = b64e(json.dumps({"sub": uid, "role": role, "exp": int(time.time()) + ttl}).encode())
    s = b64e(hmac.new(secret.encode(), f"{h}.{p}".encode(), hashlib.sha256).digest())
    return f"{h}.{p}.{s}"

def verify(tok, secret):
    parts = tok.split(".")
    assert len(parts) == 3, "shape"
    h, p, s = parts
    want = hmac.new(secret.encode(), f"{h}.{p}".encode(), hashlib.sha256).digest()
    got = base64.urlsafe_b64decode(s + "=" * (-len(s) % 4))
    assert hmac.compare_digest(want, got), "signature"
    claims = json.loads(base64.urlsafe_b64decode(p + "=" * (-len(p) % 4)))
    assert claims["sub"] > 0 and claims["role"], "claims"
    assert claims["exp"] >= int(time.time()), "expiry"
    return claims

SEC = "test-secret-16plus-chars"
t = issue(7, "SELLER", SEC)
assert verify(t, SEC)["role"] == "SELLER"
for bad in [t[:-2] + "xx", t + ".extra", "not-a-token", issue(7, "SELLER", SEC, ttl=-5)]:
    try:
        verify(bad, SEC)
        raise SystemExit(f"NOT REJECTED: {bad[:30]}")
    except (AssertionError, Exception):
        pass
try:
    verify(t, "wrong-secret-16plus-chars")
    raise SystemExit("wrong secret accepted")
except AssertionError:
    pass
print("jwt-contract-ok")
