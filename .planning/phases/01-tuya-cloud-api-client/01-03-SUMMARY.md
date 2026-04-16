# Plan 01-03 Summary: OAuth2 Token Fetch + Auto-Refresh

**Completed:** 2026-04-16
**Phase:** 1 — Tuya Cloud API Client
**Commit:** 626a913

## What Was Built

### tuya_auth.h
`TuyaAuth` singleton class using the same `GetInstance()` pattern as `McpServer`.

**Interface:**
- `GetValidToken()` — returns valid access token (fetches/refreshes as needed), empty on failure
- `GetUid()` — returns Tuya user UID (extracted from token response, used in Phase 2)
- `Refresh()` — force-clears cache and fetches a fresh token
- `kRefreshThreshold = 0.8f` — refresh at 80% of TTL (5760s out of 7200s default)

### tuya_auth.cc
**FetchToken():**
- Calls `TuyaClient::Get("/v1.0/token?grant_type=1", "", response)` with no access_token
- Parses JSON: extracts `access_token`, `expire_time`, `uid`
- Sets `token_fetched_at_` = current epoch time for TTL tracking
- Guards: `IsConfigured()` check before any network call; `success` field check with detailed error logging

**GetValidToken():**
- Returns `""` immediately if not configured
- Checks if token is empty OR elapsed time ≥ `expire_time * 0.8`
- Calls `FetchToken()` if refresh needed, returns `""` on failure

## Key Decisions
- Token is in-memory only (not persisted to NVS) — re-fetched on each boot
- UID is stored in `uid_` member for downstream use in Phase 2
- Error logging includes Tuya `code` + `msg` fields for easier debugging

## Self-Check: PASSED
All 13 plan acceptance criteria and all 11 end-to-end phase criteria verified green.
