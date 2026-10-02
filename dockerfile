FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    ca-certificates dumb-init \
  && rm -rf /var/lib/apt/lists/*

COPY . .

# Build
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --config Release

# Runtime stage
FROM debian:bookworm-slim
WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates dumb-init \
  && rm -rf /var/lib/apt/lists/*

# Copy built artifacts (adjust path/binary name if needed)
COPY --from=0 /app/build /app/build

# Non-root user
RUN useradd -m -u 10001 appuser && chown -R appuser:appuser /app
USER appuser

ENTRYPOINT ["dumb-init", "--"]

# TODO: Replace with your actual binary path/name
CMD ["./build/AudioVideoEngine"]