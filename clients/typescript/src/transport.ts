import {Buffer} from "node:buffer";
import {randomUUID} from "node:crypto";
import {createConnection, type Socket} from "node:net";
import {MinesweeperClientError} from "./errors.js";
import {
  decodeMinesweeperResponse,
  encodeMinesweeperRequest,
  MAX_MINESWEEPER_FRAME_BYTES,
} from "./protocol.js";
import type {
  MinesweeperCommand,
  MinesweeperRuntimeEvent,
} from "./types.js";

type PendingRequest = {
  resolve: (event: MinesweeperRuntimeEvent) => void;
  reject: (error: Error) => void;
  timeout: ReturnType<typeof setTimeout>;
};

export class MinesweeperTransport {
  private socket?: Socket;
  private connecting?: Promise<void>;
  private input = Buffer.alloc(0);
  private closed = false;
  private readonly pending = new Map<string, PendingRequest>();

  constructor(
    private readonly endpoint: URL,
    private readonly requestTimeoutMs: number,
  ) {}

  async request(gameId: string, command: MinesweeperCommand) {
    await this.connect();

    const socket = this.socket;
    if (!socket || socket.destroyed) {
      throw new MinesweeperClientError(
        "connection",
        "Minesweeper runtime is not connected",
      );
    }

    const requestId = randomUUID();
    const payload = encodeMinesweeperRequest(requestId, gameId, command);
    if (payload.byteLength > MAX_MINESWEEPER_FRAME_BYTES) {
      throw new MinesweeperClientError(
        "input",
        "Minesweeper command exceeds the frame limit",
      );
    }

    const header = Buffer.allocUnsafe(4);
    header.writeUInt32BE(payload.byteLength);
    const frame = Buffer.concat([header, payload]);

    const response = new Promise<MinesweeperRuntimeEvent>((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pending.delete(requestId);
        reject(
          new MinesweeperClientError(
            "timeout",
            "Minesweeper request timed out; the command may have executed",
          ),
        );
      }, this.requestTimeoutMs);
      this.pending.set(requestId, {resolve, reject, timeout});
    });

    try {
      socket.write(frame, (error) => {
        if (error) this.disconnect(socket, error);
      });
    } catch (cause) {
      this.disconnect(
        socket,
        new MinesweeperClientError(
          "connection",
          "Unable to write to the Minesweeper runtime",
          {cause},
        ),
      );
    }

    return response;
  }

  close() {
    if (this.closed) return;
    this.closed = true;

    const error = new MinesweeperClientError(
      "closed",
      "Minesweeper client was closed",
    );
    this.rejectPending(error);
    this.input = Buffer.alloc(0);

    const socket = this.socket;
    this.socket = undefined;
    socket?.destroy();
  }

  private async connect() {
    if (this.closed) {
      throw new MinesweeperClientError(
        "closed",
        "Minesweeper client is closed",
      );
    }
    if (this.connecting) return this.connecting;
    if (this.socket && !this.socket.destroyed) return;

    const connecting = new Promise<void>((resolve, reject) => {
      const socket = createConnection({
        host: this.endpoint.hostname,
        port: Number(this.endpoint.port),
      });
      this.socket = socket;

      const cleanup = () => {
        socket.off("error", onError);
        socket.off("close", onClose);
      };
      const fail = (cause?: Error) => {
        cleanup();
        if (this.socket === socket) this.socket = undefined;
        reject(
          this.closed
            ? new MinesweeperClientError(
                "closed",
                "Minesweeper client was closed",
              )
            : new MinesweeperClientError(
                "connection",
                "Unable to connect to the Minesweeper runtime",
                cause ? {cause} : undefined,
              ),
        );
      };
      const onError = (cause: Error) => fail(cause);
      const onClose = () =>
        fail(new Error("Connection closed before it was established"));

      socket.once("error", onError);
      socket.once("close", onClose);
      socket.once("connect", () => {
        cleanup();
        if (this.closed) {
          socket.destroy();
          fail();
          return;
        }

        this.input = Buffer.alloc(0);
        socket.on("data", (chunk) => this.receive(socket, chunk));
        socket.on("error", (cause) => this.disconnect(socket, cause));
        socket.on("close", () =>
          this.disconnect(
            socket,
            new Error("Minesweeper runtime disconnected"),
          ),
        );
        resolve();
      });
    });

    this.connecting = connecting;
    try {
      await connecting;
    } finally {
      if (this.connecting === connecting) this.connecting = undefined;
    }
  }

  private receive(socket: Socket, chunk: Buffer) {
    if (this.socket !== socket) return;
    this.input = Buffer.concat([this.input, chunk]);

    while (this.input.byteLength >= 4) {
      const frameLength = this.input.readUInt32BE(0);
      if (
        frameLength === 0 ||
        frameLength > MAX_MINESWEEPER_FRAME_BYTES
      ) {
        this.disconnect(
          socket,
          new MinesweeperClientError(
            "protocol",
            "Minesweeper runtime sent an invalid frame length",
          ),
        );
        return;
      }
      if (this.input.byteLength < frameLength + 4) return;

      const frame = this.input.subarray(4, frameLength + 4);
      this.input = this.input.subarray(frameLength + 4);

      let event: MinesweeperRuntimeEvent;
      try {
        event = decodeMinesweeperResponse(frame);
      } catch (cause) {
        this.disconnect(
          socket,
          cause instanceof Error
            ? cause
            : new MinesweeperClientError(
                "protocol",
                "Unable to decode the Minesweeper response",
              ),
        );
        return;
      }

      if (!event.requestId) {
        this.disconnect(
          socket,
          new MinesweeperClientError(
            "protocol",
            event.type === "error"
              ? event.payload.message
              : "Minesweeper response has no request ID",
          ),
        );
        return;
      }

      const pending = this.pending.get(event.requestId);
      if (!pending) continue;
      clearTimeout(pending.timeout);
      this.pending.delete(event.requestId);
      pending.resolve(event);
    }
  }

  private disconnect(socket: Socket, cause: Error) {
    if (this.socket !== socket) return;
    this.socket = undefined;
    this.input = Buffer.alloc(0);
    socket.destroy();
    this.rejectPending(
      cause instanceof MinesweeperClientError
        ? cause
        : new MinesweeperClientError(
            "connection",
            "Minesweeper runtime disconnected",
            {cause},
          ),
    );
  }

  private rejectPending(error: Error) {
    for (const request of this.pending.values()) {
      clearTimeout(request.timeout);
      request.reject(error);
    }
    this.pending.clear();
  }
}
