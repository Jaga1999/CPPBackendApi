# ===================================================
# Stage 1: Build Environment
# ===================================================
FROM ubuntu:24.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install C++20 toolchain, CMake, Ninja, Git, OpenSSL, PostgreSQL, and build utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    g++ \
    gcc \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    python3 \
    linux-libc-dev \
    bison \
    flex \
    libssl-dev \
    libpq-dev \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

# Strictly enforce ISO C++20 standard
ENV CC=gcc \
    CXX=g++ \
    CXXFLAGS="-std=c++20"

# Install standalone vcpkg for container builds
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /app

# Copy dependency manifests
COPY vcpkg.json /app/

# Install dependencies via vcpkg in manifest mode
RUN /opt/vcpkg/vcpkg install --triplet x64-linux

# Copy complete project source code (all layers and test suite)
COPY CMakeLists.txt /app/
COPY Domain/ /app/Domain/
COPY Application/ /app/Application/
COPY Infrastructure/ /app/Infrastructure/
COPY Presentation/ /app/Presentation/
COPY Core/ /app/Core/
COPY Tests/ /app/Tests/

# Configure and compile with strictly ISO Modern C++20 and Ninja
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    && cmake --build build --config Release --target Core Tests

# ===================================================
# Stage 2: Minimal Production Runtime
# ===================================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime shared libraries for PostgreSQL, OpenSSL, and healthcheck utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libpqxx-7.8t64 \
    libssl3 \
    postgresql-client \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root system user for production security
RUN useradd -m -u 1001 -s /bin/bash appuser

WORKDIR /app

# Copy vcpkg shared libraries if any were installed dynamically
COPY --from=builder /app/vcpkg_installed/ /app/vcpkg_installed/
ENV LD_LIBRARY_PATH=/app/vcpkg_installed/x64-linux/lib:/usr/local/lib:$LD_LIBRARY_PATH

# Copy compiled binaries from builder stage
COPY --from=builder /app/build/Core/Core /app/crowapi-service
COPY --from=builder /app/build/Tests/Tests /app/crowapi-tests

# Copy entrypoint script and configuration templates
COPY docker/docker-entrypoint.sh /app/docker-entrypoint.sh
COPY .env.example /app/.env.example
COPY .env.test /app/.env.test

# Set ownership and execution permissions
RUN chmod +x /app/crowapi-service /app/crowapi-tests /app/docker-entrypoint.sh && \
    chown -R appuser:appuser /app

# Expose default API server port
EXPOSE 8080

# Configure default environment variables
ENV DB_HOST=postgres \
    DB_PORT=5432 \
    DB_NAME=crowapi_db \
    DB_USER=postgres \
    DB_PASSWORD=postgres \
    PORT=8080

USER appuser

# Launch Crow API service via entrypoint (which verifies all tests pass first)
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["8080", "info"]