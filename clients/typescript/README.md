# `@break-my-system/minesweeper-client`

Server-side TypeScript client for the Minesweeper runtime protocol.

```ts
import {MinesweeperClient} from "@break-my-system/minesweeper-client";

const client = new MinesweeperClient({
  endpoint: "minesweeper://127.0.0.1:7575",
});

const result = await client.createGame("example", {
  rows: 9,
  cols: 9,
  mines: 10,
});

client.close();
```

Consumers that only need compile-time protocol types can avoid loading Zod or
the server-only TCP client:

```ts
import type {
  MinesweeperGameSnapshot,
  MinesweeperRuntimeEvent,
} from "@break-my-system/minesweeper-client/types";
```

The package owns protocol validation, request encoding, TCP framing, request
correlation, timeouts, and connection cleanup. Applications remain responsible
for endpoint configuration and for adapting runtime events to their own HTTP or
WebSocket APIs.

Protocol version 2 uses four-byte length-prefixed binary payloads in both
directions. See the runtime's `docs/protocol-v2.md` for the byte-level contract
and compatibility rules.
