-- Migration: 004_google_auth_schema.sql
-- Description: Google OAuth2 / OIDC integration and bidirectional account linking

-- Allow null password_hash for users who register exclusively via Google
ALTER TABLE users ALTER COLUMN password_hash DROP NOT NULL;

-- Add Google-specific user identifier and provider tracking
ALTER TABLE users ADD COLUMN IF NOT EXISTS google_id VARCHAR(128) UNIQUE;
ALTER TABLE users ADD COLUMN IF NOT EXISTS auth_provider VARCHAR(32) DEFAULT 'local' NOT NULL;
ALTER TABLE users ADD COLUMN IF NOT EXISTS avatar_url TEXT;

-- Sparse index for fast Google ID lookup during Google Sign-In
CREATE INDEX IF NOT EXISTS idx_users_google_id ON users (google_id) WHERE google_id IS NOT NULL;
