#!/bin/bash
set -e

echo "================================================================="
echo "  CrowApi Container Initialization & Pre-Flight Verification     "
echo "================================================================="

DB_HOST="${DB_HOST:-postgres}"
DB_PORT="${DB_PORT:-5432}"
DB_USER="${DB_USER:-postgres}"
DB_NAME="${DB_NAME:-crowapi_db}"

echo "[Entrypoint] Waiting for PostgreSQL database at ${DB_HOST}:${DB_PORT} (${DB_NAME})..."

MAX_RETRIES=30
RETRY_COUNT=0

until pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME"; do
    RETRY_COUNT=$((RETRY_COUNT+1))
    if [ $RETRY_COUNT -ge $MAX_RETRIES ]; then
        echo "[Entrypoint] ERROR: Timed out waiting for PostgreSQL after ${MAX_RETRIES} attempts. Exiting."
        exit 1
    fi
    echo "[Entrypoint] PostgreSQL is unavailable - sleeping 1s (Attempt ${RETRY_COUNT}/${MAX_RETRIES})..."
    sleep 1
done

echo "[Entrypoint] PostgreSQL is ready and accepting connections."

echo ""
echo "================================================================="
echo "  Running Automated Test Suite (Healthy State Verification)      "
echo "================================================================="

if [ -f "/app/crowapi-tests" ]; then
    echo "[Entrypoint] Executing /app/crowapi-tests against ${DB_HOST}:${DB_PORT}..."
    /app/crowapi-tests
    TEST_RESULT=$?
    
    if [ $TEST_RESULT -ne 0 ]; then
        echo ""
        echo "[Entrypoint] CRITICAL FAILURE: Automated test suite failed with exit code ${TEST_RESULT}!"
        echo "[Entrypoint] The CrowApi service will NOT start because healthy state requires 100% test pass."
        exit 1
    fi
    
    echo ""
    echo "[Entrypoint] SUCCESS: 100% of automated tests passed successfully!"
else
    echo "[Entrypoint] WARNING: /app/crowapi-tests not found. Skipping pre-flight tests."
fi

echo "================================================================="
echo "  Starting CrowApi HTTP REST API Service                         "
echo "================================================================="
echo "[Entrypoint] Executing: /app/crowapi-service $@"
exec /app/crowapi-service "$@"