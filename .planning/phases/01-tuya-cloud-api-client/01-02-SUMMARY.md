# Plan 01-02 Summary: HMAC-SHA256 Signing + HTTP Client

**Completed:** 2026-04-16
**Phase:** 1 — Tuya Cloud API Client
**Commit:** c8d2f10

## What Was Built

### tuya_client.h
`TuyaClient` class interface with `Get()` and `Post()` methods plus private signing helpers.

### tuya_client.cc
Complete implementation of the Tuya API HTTP client:

**Signing (BuildHeaders):**
- Generates 13-digit UTC timestamp from `gettimeofday()`
- Computes SHA256 of request body (uses hardcoded empty-body constant for GET)
- Builds `stringToSign = METHOD\nBODY_SHA256\n\nPATH`
- Two signing paths per Tuya spec:
  - Token API (empty access_token): `client_id + t + nonce + stringToSign`
  - Business API (non-empty access_token): `client_id + access_token + t + nonce + stringToSign`
- HmacSha256Upper: mbedTLS HMAC-SHA256, hex-encoded, **uppercased**

**HTTP Execution:**
- `esp_http_client` with `esp_crt_bundle_attach` for HTTPS
- 10s timeout
- Response body captured via `HTTP_EVENT_ON_DATA`
- Logs HTTP status and response size

## Key Decisions
- Empty body SHA256 constant avoids computing hash every GET request
- Uppercase transform on HMAC hex is mandatory (Tuya rejects lowercase)
- `esp_crt_bundle_attach` handles Tuya's public CA cert without embedding custom certs

## Self-Check: PASSED
All 13 acceptance criteria verified green.
