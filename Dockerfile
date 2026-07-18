FROM alpine:3.21 AS builder

RUN apk add --no-cache g++ make

WORKDIR /src
COPY makefile ./makefile
COPY src ./src
RUN make server

FROM alpine:3.21

RUN apk add --no-cache libstdc++ \
    && addgroup -S -g 10001 minesweeper \
    && adduser -S -D -H -u 10001 -G minesweeper minesweeper \
    && install -d -o minesweeper -g minesweeper /data

COPY --from=builder /src/minesweeper-server /usr/local/bin/minesweeper-server

ENV MINESWEEPER_MODE=server
ENV MINESWEEPER_DATA_DIR=/data
ENV MINESWEEPER_LEADERBOARD_PATH=/data/leaderboard.csv

VOLUME ["/data"]

USER minesweeper

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
  CMD ["sh", "-c", "kill -0 1"]

ENTRYPOINT ["/usr/local/bin/minesweeper-server"]
