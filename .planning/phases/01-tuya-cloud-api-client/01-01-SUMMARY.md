# Plan 01-01 Summary: NVS Credentials + SNTP

**Completed:** 2026-04-16
**Phase:** 1 — Tuya Cloud API Client
**Commit:** dc2c207

## What Was Built

### tuya_config.h
Static helper class `TuyaConfig` providing NVS-backed credential management for the Tuya Cloud API. Uses the existing `Settings` class with NVS namespace `"tuya"`.

**Methods:**
- `GetClientId()` — reads `client_id` from NVS
- `GetClientSecret()` — reads `client_secret` from NVS  
- `GetRegion()` — reads `region` from NVS, defaults to `openapi.tuyaus.com`
- `IsConfigured()` — returns `true` only if both client_id and client_secret are non-empty
- `Save(client_id, client_secret, region)` — writes all three credentials to NVS

### tuya_sntp.h
Static helper class `TuyaSntp` for NTP time synchronization. Created because no SNTP initialization was found in the existing codebase.

**Methods:**
- `Sync()` — initializes SNTP with `pool.ntp.org`, blocks up to 10s for completion
- `GetTimestampMs()` — returns 13-digit UTC timestamp in milliseconds (required for Tuya signing)

### CMakeLists.txt
- Added `tuya/tuya_auth.cc` and `tuya/tuya_client.cc` to build sources
- Added `"tuya"` to `INCLUDE_DIRS` for header resolution

## Key Decisions
- NVS namespace is `"tuya"` (≤15 chars as required by ESP-IDF)
- Default region is `openapi.tuyaus.com` (Americas — user can change via `TuyaConfig::Save()`)
- SNTP uses `pool.ntp.org` which is reliable globally

## Self-Check: PASSED
All 10 acceptance criteria verified green.
