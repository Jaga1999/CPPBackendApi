-- Migration: 001_init_schema.sql
-- Description: PostgreSQL 18.6 multi-paradigm schema
-- 1. Relational SQL Database (todos)
-- 2. Redis Alternative (cache_store with TTL)
-- 3. Kafka Alternative (message_queue with SKIP LOCKED)
-- 4. MongoDB Alternative (documents with JSONB and GIN indexing)

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ==========================================
-- 1. RELATIONAL SQL DB: todos table
-- ==========================================
CREATE TABLE IF NOT EXISTS todos (
    id BIGSERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    completed BOOLEAN NOT NULL DEFAULT false,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp()
);

CREATE INDEX IF NOT EXISTS idx_todos_completed ON todos (completed);

-- ==========================================
-- 2. REDIS ALTERNATIVE: cache_store table
-- High speed key-value cache with TTL and atomic upsert
-- ==========================================
CREATE TABLE IF NOT EXISTS cache_store (
    cache_key VARCHAR(255) PRIMARY KEY,
    cache_value TEXT NOT NULL,
    ttl_seconds INT NOT NULL DEFAULT 3600,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    expires_at TIMESTAMPTZ NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_cache_expires_at ON cache_store (expires_at);

-- ==========================================
-- 3. KAFKA ALTERNATIVE: message_queue table
-- High-throughput transactional message queue with SKIP LOCKED worker polling
-- ==========================================
CREATE TABLE IF NOT EXISTS message_queue (
    id BIGSERIAL PRIMARY KEY,
    topic VARCHAR(100) NOT NULL,
    payload JSONB NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    retry_count INT NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    processed_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_queue_topic_status_id ON message_queue (topic, status, id);

-- ==========================================
-- 4. MONGODB ALTERNATIVE: documents table
-- Schemaless JSONB document collection with GIN index
-- ==========================================
CREATE TABLE IF NOT EXISTS documents (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    collection_name VARCHAR(100) NOT NULL,
    data JSONB NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT clock_timestamp()
);

CREATE INDEX IF NOT EXISTS idx_documents_collection ON documents (collection_name);
CREATE INDEX IF NOT EXISTS idx_documents_gin ON documents USING gin (data jsonb_path_ops);

-- Pre-seed some initial data
INSERT INTO todos (title, description, completed) VALUES
    ('Explore Crow C++ Framework', 'Investigate Crow capabilities and routing with C++20', true),
    ('Multi-Module Clean Architecture', 'Organize solution into Domain, Application, Infrastructure, Presentation, Core projects', true),
    ('PostgreSQL 18.6 Multi-Paradigm Integration', 'Unify SQL, Redis, Kafka, and MongoDB paradigms in PostgreSQL', false)
ON CONFLICT DO NOTHING;

INSERT INTO cache_store (cache_key, cache_value, ttl_seconds, expires_at) VALUES
    ('system:status', 'online', 86400, clock_timestamp() + interval '1 day'),
    ('app:version', '1.0.0', 86400, clock_timestamp() + interval '1 day')
ON CONFLICT (cache_key) DO NOTHING;
