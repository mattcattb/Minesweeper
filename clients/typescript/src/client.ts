import {MinesweeperClientError} from "./errors.js";
import {minesweeperCommandSchema} from "./messages.js";
import {minesweeperGameIdSchema} from "./protocol.js";
import {MinesweeperTransport} from "./transport.js";
import type {
  MinesweeperCommand,
  MinesweeperGameConfig,
} from "./types.js";

export type MinesweeperClientOptions = {
  endpoint: string;
  requestTimeoutMs?: number;
};

export class MinesweeperClient {
  private readonly transport: MinesweeperTransport;

  constructor(options: MinesweeperClientOptions) {
    let endpoint: URL;
    try {
      endpoint = new URL(options.endpoint);
    } catch (cause) {
      throw new MinesweeperClientError(
        "configuration",
        "Invalid Minesweeper endpoint",
        {cause},
      );
    }
    if (
      endpoint.protocol !== "minesweeper:" ||
      !endpoint.hostname ||
      !endpoint.port
    ) {
      throw new MinesweeperClientError(
        "configuration",
        "Minesweeper endpoint must be minesweeper://host:port",
      );
    }

    const requestTimeoutMs = options.requestTimeoutMs ?? 5_000;
    if (!Number.isFinite(requestTimeoutMs) || requestTimeoutMs <= 0) {
      throw new MinesweeperClientError(
        "configuration",
        "Request timeout must be a positive number",
      );
    }
    this.transport = new MinesweeperTransport(endpoint, requestTimeoutMs);
  }

  createGame(gameId: string, config: MinesweeperGameConfig) {
    return this.send(gameId, {type: "game.create", payload: config});
  }

  revealTile(gameId: string, row: number, col: number) {
    return this.send(gameId, {type: "tile.reveal", payload: {row, col}});
  }

  toggleFlag(gameId: string, row: number, col: number) {
    return this.send(gameId, {
      type: "tile.flag.toggle",
      payload: {row, col},
    });
  }

  restartGame(gameId: string) {
    return this.send(gameId, {type: "game.restart"});
  }

  getSnapshot(gameId: string) {
    return this.send(gameId, {type: "game.resync"});
  }

  send(gameId: string, command: MinesweeperCommand) {
    const parsedGameId = minesweeperGameIdSchema.safeParse(gameId);
    if (!parsedGameId.success) {
      throw new MinesweeperClientError(
        "input",
        parsedGameId.error.issues[0]?.message ?? "Invalid game ID",
        {cause: parsedGameId.error},
      );
    }

    const parsedCommand = minesweeperCommandSchema.safeParse(command);
    if (!parsedCommand.success) {
      throw new MinesweeperClientError(
        "input",
        parsedCommand.error.issues[0]?.message ??
          "Invalid Minesweeper command",
        {cause: parsedCommand.error},
      );
    }

    return this.transport.request(parsedGameId.data, parsedCommand.data);
  }

  close() {
    this.transport.close();
  }
}
