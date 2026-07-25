# Minesweeper TCP protocol v2

Protocol v2 is the private runtime boundary shared by the C++ server and its
TypeScript SDK. It is distinct from the JSON WebSocket contract exposed by the
HTTP API to browsers.

## Framing and primitives

Every message is one frame:

```text
u32 payload byte length | payload
```

The length and all numeric fields are unsigned big-endian integers unless a
field is explicitly marked `i32`. A `string` is a `u16` UTF-8 byte length
followed by exactly that many bytes. The maximum frame payload is 4 MiB.

Readers must reject zero-length or oversized frames, truncated fields, invalid
UTF-8, unknown tags, unsupported versions, invalid field values, and trailing
bytes. Writers must reject values that do not fit their declared wire type.

## Request payload

```text
u16 version (= 2)
string requestId
u8 command
string gameId
command payload
```

`requestId` precedes the command and game ID so the server can correlate an
error after reading a valid request ID, including a version mismatch.

| Command | Tag | Payload |
| --- | ---: | --- |
| Create game | 1 | `u16 rows, u16 cols, u16 mines` |
| Reveal tile | 2 | `u16 row, u16 col` |
| Toggle flag | 3 | `u16 row, u16 col` |
| Restart game | 4 | none |
| Request snapshot | 5 | none |

Request and game IDs must be non-empty. The command payload must consume the
rest of the frame exactly.

## Response payload

Every response begins:

```text
u16 version (= 2)
u8 event
string requestId
event payload
```

An event tag of `1` is a snapshot:

```text
u8 audience
string gameId
u64 revision
u8 status
u32 elapsedSeconds
i32 remainingMines
u16 rows
u16 cols
(u8 tileState, u8 tileDetail) repeated rows * cols times
```

Audience tags are `1 = game` and `2 = connection`. Status tags are
`1 = playing`, `2 = won`, and `3 = lost`.

Tiles are encoded in row-major order, so coordinates are not repeated on the
wire. A hidden tile has state `1` and detail `0 = unflagged` or `1 = flagged`.
A revealed tile has state `2` and detail `0..8 = adjacent mine count` or
`9 = mine`. The sender must encode exactly `rows * cols` tiles; the receiver
reconstructs each coordinate from its index.

An event tag of `2` is an error:

```text
u8 errorCode
string message
```

| Error | Tag |
| --- | ---: |
| `BAD_REQUEST` | 1 |
| `GAME_NOT_FOUND` | 2 |
| `INVALID_ACTION` | 3 |
| `SYSTEM_UNAVAILABLE` | 4 |

The request ID may be empty only when the server could not decode it. Clients
cannot correlate such an error to one pending request and must treat it as a
connection-level protocol failure.

## Compatibility

Version 2 replaces version 1; the server and SDK must be deployed together.
There is intentionally no v1 compatibility decoder. Any incompatible wire
change increments the protocol version. Compatible validation fixes and SDK
behavior changes do not.

Golden byte-vector tests exist in both C++ and TypeScript. The integration test
starts the real C++ runtime and exercises the SDK over TCP, which catches drift
that unit tests on either side cannot catch alone.
