import {afterAll, describe, expect, test} from "bun:test";
import {mkdtemp, rm} from "node:fs/promises";
import {createConnection, createServer} from "node:net";
import {tmpdir} from "node:os";
import {join, resolve} from "node:path";
import {MinesweeperClient} from "../src/index";

const minesweeperRoot = resolve(import.meta.dir, "../../..");
const dataDirectory = await mkdtemp(
  join(tmpdir(), "minesweeper-client-integration-"),
);

const reservePort = async () => {
  const server = createServer();
  await new Promise<void>((resolveListen, reject) => {
    server.once("error", reject);
    server.listen(0, "127.0.0.1", resolveListen);
  });
  const address = server.address();
  if (!address || typeof address === "string") {
    throw new Error("Unable to reserve an integration-test port");
  }
  await new Promise<void>((resolveClose, reject) =>
    server.close((error) => (error ? reject(error) : resolveClose())),
  );
  return address.port;
};

const waitForPort = async (port: number) => {
  for (let attempt = 0; attempt < 100; attempt += 1) {
    const connected = await new Promise<boolean>((resolveConnection) => {
      const socket = createConnection({host: "127.0.0.1", port});
      socket.once("connect", () => {
        socket.destroy();
        resolveConnection(true);
      });
      socket.once("error", () => resolveConnection(false));
    });
    if (connected) return;
    await Bun.sleep(20);
  }
  throw new Error("Minesweeper runtime did not start");
};

const port = await reservePort();
const runtime = Bun.spawn(["./build/minesweeper-server"], {
  cwd: minesweeperRoot,
  env: {
    ...process.env,
    MINESWEEPER_MODE: "server",
    MINESWEEPER_PORT: String(port),
    MINESWEEPER_DATA_DIR: dataDirectory,
  },
  stdout: "ignore",
  stderr: "inherit",
});
await waitForPort(port);

afterAll(async () => {
  runtime.kill();
  await runtime.exited;
  await rm(dataDirectory, {recursive: true, force: true});
});

describe("Minesweeper client and C++ runtime", () => {
  test("create, mutate, and resynchronize a game", async () => {
    const client = new MinesweeperClient({
      endpoint: `minesweeper://127.0.0.1:${port}`,
    });

    try {
      const created = await client.createGame("integration-game", {
        rows: 4,
        cols: 4,
        mines: 3,
      });
      expect(created).toMatchObject({
        type: "game.snapshot",
        audience: "game",
        payload: {revision: 0, rows: 4, cols: 4},
      });

      const flagged = await client.toggleFlag("integration-game", 0, 0);
      expect(flagged).toMatchObject({
        type: "game.snapshot",
        payload: {revision: 1},
      });

      const snapshot = await client.getSnapshot("integration-game");
      expect(snapshot).toMatchObject({
        type: "game.snapshot",
        audience: "connection",
        payload: {revision: 1},
      });

      const restarted = await client.restartGame("integration-game");
      expect(restarted).toMatchObject({
        type: "game.snapshot",
        payload: {revision: 2},
      });

      const revealed = await client.revealTile("integration-game", 0, 0);
      expect(revealed).toMatchObject({
        type: "game.snapshot",
        payload: {revision: 3},
      });

      const missing = await client.getSnapshot("missing-game");
      expect(missing).toMatchObject({
        type: "error",
        payload: {code: "GAME_NOT_FOUND"},
      });

      const invalid = await client.revealTile("integration-game", 99, 99);
      expect(invalid).toMatchObject({
        type: "error",
        payload: {code: "BAD_REQUEST"},
      });
    } finally {
      client.close();
    }
  });
});
