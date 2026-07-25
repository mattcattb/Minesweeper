FROM alpine:3.21 AS builder

RUN apk add --no-cache g++ make nlohmann-json

WORKDIR /workspace
COPY makefile ./makefile
COPY src ./src
RUN make server

FROM alpine:3.21

ARG VERSION=0.2.0

LABEL org.opencontainers.image.title="Minesweeper"
LABEL org.opencontainers.image.description="Headless Minesweeper TCP runtime"
LABEL org.opencontainers.image.source="https://github.com/mattcattb/Minesweeper"
LABEL org.opencontainers.image.version="${VERSION}"

RUN apk add --no-cache libstdc++ \
    && addgroup -S -g 10001 minesweeper \
    && adduser -S -D -H -u 10001 -G minesweeper minesweeper \
    && install -d -o minesweeper -g minesweeper /data

COPY --from=builder /workspace/build/minesweeper-server /usr/local/bin/minesweeper-server

ENV MINESWEEPER_MODE=server
ENV MINESWEEPER_PORT=7575
ENV MINESWEEPER_DATA_DIR=/data
ENV MINESWEEPER_LEADERBOARD_PATH=/data/leaderboard.csv

VOLUME ["/data"]

EXPOSE 7575

USER minesweeper

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
  CMD ["nc", "-z", "127.0.0.1", "7575"]

ENTRYPOINT ["/usr/local/bin/minesweeper-server"]
