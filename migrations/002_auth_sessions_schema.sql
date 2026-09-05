-- Migration: 002_auth_sessions_schema.sql
-- Description: Production-grade users, multi-sessions, token revocation, and security audit logs

-- Ensure UUID extension is available
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ==========================================
-- 1. USERS TABLE
-- ==========================================
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(50) NOT NULL DEFAULT 'user', -- 'user', 'admin'
    is_active BOOLEAN NOT NULL DEFAULT true,
    failed_login_attempts INT NOT NULL DEFAULT 0,
    locked_until TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp()
);

CREATE INDEX IF NOT EXISTS idx_users_email ON users (email);
CREATE INDEX IF NOT EXISTS idx_users_role ON users (role);

-- ==========================================
-- 2. SESSIONS TABLE (Multi-Session Support)
-- ==========================================
CREATE TABLE IF NOT EXISTS sessions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    jti VARCHAR(255) UNIQUE NOT NULL,
    refresh_token_hash VARCHAR(64) NOT NULL, -- SHA-256 hex hash of refresh token
    previous_refresh_token_hash VARCHAR(64), -- SHA-256 hex hash of previously rotated token (for reuse detection)
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    last_seen_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    expires_at TIMESTAMPTZ NOT NULL,
    revoked_at TIMESTAMPTZ,
    revocation_reason VARCHAR(255),
    ip_address VARCHAR(100),
    user_agent TEXT,
    device_name VARCHAR(150),
    client_type VARCHAR(50) DEFAULT 'browser'
);

CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions (user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_refresh_hash ON sessions (refresh_token_hash);
CREATE INDEX IF NOT EXISTS idx_sessions_prev_refresh ON sessions (previous_refresh_token_hash);
CREATE INDEX IF NOT EXISTS idx_sessions_jti ON sessions (jti);
CREATE INDEX IF NOT EXISTS idx_sessions_active ON sessions (user_id, expires_at) WHERE revoked_at IS NULL;
CREATE INDEX IF NOT EXISTS idx_sessions_expires_at ON sessions (expires_at);

-- ==========================================
-- 3. TOKEN REVOCATIONS TABLE (Fast immediate JTI checks)
-- ==========================================
CREATE TABLE IF NOT EXISTS token_revocations (
    jti VARCHAR(255) PRIMARY KEY,
    session_id UUID NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    revoked_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp()
);

CREATE INDEX IF NOT EXISTS idx_token_revocations_expires ON token_revocations (expires_at);

-- ==========================================
-- 4. SECURITY AUDIT LOGS TABLE (Append-Only)
-- ==========================================
CREATE TABLE IF NOT EXISTS audit_logs (
    id BIGSERIAL PRIMARY KEY,
    event_type VARCHAR(100) NOT NULL,
    user_id UUID,
    admin_user_id UUID,
    session_id UUID,
    ip_address VARCHAR(100),
    user_agent TEXT,
    reason TEXT,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp()
);

CREATE INDEX IF NOT EXISTS idx_audit_logs_user_created ON audit_logs (user_id, created_at);
CREATE INDEX IF NOT EXISTS idx_audit_logs_event_created ON audit_logs (event_type, created_at);
