import {describe, expect, test} from "bun:test";
import {Buffer} from "node:buffer";
import {createServer, type Server, type Socket} from "node:net";
import {
  MinesweeperClient,
  MinesweeperClientError,
} from "../src/index";

const listen = (server: Server) =>
  new Promise<number>((resolve, reject) => {
    server.once("error", reject);
    server.listen(0, "127.0.0.1", () => {
      server.off("error", reject);
      const address = server.address();
      if (!address || typeof address === "string") {
        reject(new Error("Test server did not expose a TCP port"));
        return;
      }
      resolve(address.port);
    });
  });

const close = (server: Server) =>
  new Promise<void>((resolve, reject) => {
    server.close((error) => (error ? reject(error) : resolve()));
  });

const readString = (payload: Buffer, offset: number) => {
  const length = payload.readUInt16BE(offset);
  const start = offset + 2;
  return {
    value: payload.subarray(start, start + length).toString("utf8"),
    offset: start + length,
  };
};

const writeResponse = (
  socket: Socket,
  requestId: string,
  gameId: string,
) => {
  const encodeString = (value: string) => {
    const content = Buffer.from(value, "utf8");
    const length = Buffer.allocUnsafe(2);
    length.writeUInt16BE(content.byteLength);
    return Buffer.concat([length, content]);
  };
  const fields = Buffer.alloc(21);
  fields.writeBigUInt64BE(0n, 0);
  fields.writeUInt8(1, 8);
  fields.writeUInt32BE(0, 9);
  fields.writeInt32BE(1, 13);
  fields.writeUInt16BE(2, 17);
  fields.writeUInt16BE(2, 19);
  const payload = Buffer.concat([
    Buffer.from([0, 2, 1]),
    encodeString(requestId),
    Buffer.from([1]),
    encodeString(gameId),
    fields,
    Buffer.from([1, 0, 1, 0, 1, 0, 1, 0]),
  ]);
  const header = Buffer.allocUnsafe(4);
  header.writeUInt32BE(payload.byteLength);
  const frame = Buffer.concat([header, payload]);

  socket.write(frame.subarray(0, 2));
  setTimeout(() => socket.write(frame.subarray(2)), 5);
};

describe("Minesweeper transport", () => {
  test("correlates a framed response delivered in partial chunks", async () => {
    const server = createServer((socket) => {
      let input = Buffer.alloc(0);
      socket.on("data", (chunk) => {
        input = Buffer.concat([input, chunk]);
        if (input.byteLength < 4) return;
        const length = input.readUInt32BE(0);
        if (input.byteLength < length + 4) return;

        const payload = input.subarray(4, length + 4);
        const requestId = readString(payload, 2);
        const gameId = readString(payload, requestId.offset + 1);
        writeResponse(socket, requestId.value, gameId.value);
      });
    });
    const port = await listen(server);
    const client = new MinesweeperClient({
      endpoint: `minesweeper://127.0.0.1:${port}`,
    });

    try {
      const event = await client.createGame("test-game", {
        rows: 2,
        cols: 2,
        mines: 1,
      });
      expect(event).toMatchObject({
        type: "game.snapshot",
        gameId: "test-game",
      });
    } finally {
      client.close();
      await close(server);
    }
  });

  test("reports timeout uncertainty without retrying the command", async () => {
    const server = createServer(() => {});
    const port = await listen(server);
    const client = new MinesweeperClient({
      endpoint: `minesweeper://127.0.0.1:${port}`,
      requestTimeoutMs: 20,
    });

    try {
      await expect(
        client.createGame("timeout-game", {
          rows: 2,
          cols: 2,
          mines: 1,
        }),
      ).rejects.toMatchObject<MinesweeperClientError>({
        kind: "timeout",
      });
    } finally {
      client.close();
      await close(server);
    }
  });
});
