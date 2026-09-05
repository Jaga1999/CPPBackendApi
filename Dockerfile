# ===================================================
# Stage 1: Build Environment
# ===================================================
FROM ubuntu:24.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install C++20 toolchain, CMake, Ninja, Git, and build utilities
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

# Copy complete project source code
COPY CMakeLists.txt /app/
COPY Domain/ /app/Domain/
COPY Application/ /app/Application/
COPY Infrastructure/ /app/Infrastructure/
COPY Presentation/ /app/Presentation/
COPY Core/ /app/Core/

# Configure and compile with strictly ISO Modern C++20 and Ninja
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    && cmake --build build --config Release --target Core

# ===================================================
# Stage 2: Minimal Production Runtime
# ===================================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime shared libraries for PostgreSQL and OpenSSL
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libpqxx-6.4 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root system user for security
RUN useradd -m -u 1000 -s /bin/bash appuser

WORKDIR /app

# Copy compiled binary from builder stage
COPY --from=builder /app/build/Core/Core /app/crowapi-service

# Expose default API server port
EXPOSE 8080

# Configure environment variables
ENV DB_HOST=postgres \
    DB_PORT=5432 \
    DB_NAME=crowapi_db \
    DB_USER=postgres \
    DB_PASSWORD=postgres \
    PORT=8080

USER appuser

# Launch the Crow API executable
ENTRYPOINT ["/app/crowapi-service"]
CMD ["8080", "info"]
