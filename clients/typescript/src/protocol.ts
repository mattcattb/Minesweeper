import {Buffer} from "node:buffer";
import {TextDecoder} from "node:util";
import {z} from "zod";
import {MinesweeperClientError} from "./errors.js";
import {minesweeperRuntimeEventSchema} from "./messages.js";
import type {
  MinesweeperCommand,
  MinesweeperRuntimeEvent,
  MinesweeperTile,
} from "./types.js";

export const MINESWEEPER_PROTOCOL_VERSION = 2;
export const MAX_MINESWEEPER_FRAME_BYTES = 4 * 1024 * 1024;
const MAX_U16 = 0xffff;
const utf8Decoder = new TextDecoder("utf-8", {fatal: true});

export const minesweeperGameIdSchema = z
  .string()
  .min(1)
  .refine((value) => Buffer.byteLength(value, "utf8") <= MAX_U16, {
    message: "Game ID exceeds the Minesweeper protocol limit",
  });

class BinaryWriter {
  private readonly chunks: Buffer[] = [];

  writeU8(value: number) {
    const buffer = Buffer.allocUnsafe(1);
    buffer.writeUInt8(value);
    this.chunks.push(buffer);
  }

  writeU16(value: number) {
    const buffer = Buffer.allocUnsafe(2);
    buffer.writeUInt16BE(value);
    this.chunks.push(buffer);
  }

  writeString(value: string) {
    const encoded = Buffer.from(value, "utf8");
    if (encoded.byteLength > MAX_U16) {
      throw new MinesweeperClientError(
        "input",
        "String exceeds the Minesweeper protocol limit",
      );
    }
    this.writeU16(encoded.byteLength);
    this.chunks.push(encoded);
  }

  finish() {
    return Buffer.concat(this.chunks);
  }
}

class BinaryReader {
  private offset = 0;

  constructor(private readonly bytes: Buffer) {}

  readU8() {
    this.require(1);
    return this.bytes.readUInt8(this.offset++);
  }

  readU16() {
    this.require(2);
    const value = this.bytes.readUInt16BE(this.offset);
    this.offset += 2;
    return value;
  }

  readU32() {
    this.require(4);
    const value = this.bytes.readUInt32BE(this.offset);
    this.offset += 4;
    return value;
  }

  readI32() {
    this.require(4);
    const value = this.bytes.readInt32BE(this.offset);
    this.offset += 4;
    return value;
  }

  readU64() {
    this.require(8);
    const value = this.bytes.readBigUInt64BE(this.offset);
    this.offset += 8;
    if (value > BigInt(Number.MAX_SAFE_INTEGER)) {
      throw new Error("Revision exceeds JavaScript's safe integer limit");
    }
    return Number(value);
  }

  readString() {
    const length = this.readU16();
    this.require(length);
    const value = utf8Decoder.decode(
      this.bytes.subarray(this.offset, this.offset + length),
    );
    this.offset += length;
    return value;
  }

  get remaining() {
    return this.bytes.byteLength - this.offset;
  }

  get finished() {
    return this.remaining === 0;
  }

  private require(length: number) {
    if (length > this.remaining) {
      throw new Error("Truncated Minesweeper response");
    }
  }
}

const commandKinds = {
  "game.create": 1,
  "tile.reveal": 2,
  "tile.flag.toggle": 3,
  "game.restart": 4,
  "game.resync": 5,
} satisfies Record<MinesweeperCommand["type"], number>;

export const encodeMinesweeperRequest = (
  requestId: string,
  gameId: string,
  command: MinesweeperCommand,
) => {
  const writer = new BinaryWriter();
  writer.writeU16(MINESWEEPER_PROTOCOL_VERSION);
  writer.writeString(requestId);
  writer.writeU8(commandKinds[command.type]);
  writer.writeString(gameId);

  switch (command.type) {
    case "game.create":
      writer.writeU16(command.payload.rows);
      writer.writeU16(command.payload.cols);
      writer.writeU16(command.payload.mines);
      break;
    case "tile.reveal":
    case "tile.flag.toggle":
      writer.writeU16(command.payload.row);
      writer.writeU16(command.payload.col);
      break;
    case "game.restart":
    case "game.resync":
      break;
  }

  return writer.finish();
};

export const decodeMinesweeperResponse = (
  frame: Buffer,
): MinesweeperRuntimeEvent => {
  try {
    const reader = new BinaryReader(frame);
    const version = reader.readU16();
    if (version !== MINESWEEPER_PROTOCOL_VERSION) {
      throw new Error(`Unsupported Minesweeper protocol version ${version}`);
    }

    const eventKind = reader.readU8();
    const requestId = reader.readString();
    let decoded: unknown;

    if (eventKind === 1) {
      const audienceKind = reader.readU8();
      const audience =
        audienceKind === 1
          ? "game"
          : audienceKind === 2
            ? "connection"
            : undefined;
      if (!audience) {
        throw new Error(
          `Unknown Minesweeper audience kind ${audienceKind}`,
        );
      }

      const gameId = reader.readString();
      const revision = reader.readU64();
      const statusKind = reader.readU8();
      const status =
        statusKind === 1
          ? "playing"
          : statusKind === 2
            ? "won"
            : statusKind === 3
              ? "lost"
              : undefined;
      if (!status) {
        throw new Error(`Unknown Minesweeper status kind ${statusKind}`);
      }

      const elapsedSeconds = reader.readU32();
      const remainingMines = reader.readI32();
      const rows = reader.readU16();
      const cols = reader.readU16();
      if (rows === 0 || cols === 0) {
        throw new Error("Minesweeper snapshot dimensions must be positive");
      }

      const tileCount = rows * cols;
      if (tileCount > Math.floor(reader.remaining / 2)) {
        throw new Error("Truncated Minesweeper snapshot tiles");
      }
      const tiles: MinesweeperTile[] = [];
      for (let index = 0; index < tileCount; index += 1) {
        const state = reader.readU8();
        const detail = reader.readU8();
        const row = Math.floor(index / cols);
        const col = index % cols;
        if (state === 1 && (detail === 0 || detail === 1)) {
          tiles.push({
            row,
            col,
            state: "hidden",
            flagged: detail === 1,
          });
        } else if (state === 2 && detail <= 9) {
          tiles.push({
            row,
            col,
            state: "revealed",
            value: detail === 9 ? "mine" : detail,
          });
        } else {
          throw new Error(
            `Invalid Minesweeper tile state ${state}:${detail}`,
          );
        }
      }

      decoded = {
        type: "game.snapshot",
        audience,
        gameId,
        requestId,
        payload: {
          revision,
          status,
          elapsedSeconds,
          remainingMines,
          rows,
          cols,
          tiles,
        },
      };
    } else if (eventKind === 2) {
      const codeKind = reader.readU8();
      const code =
        codeKind === 1
          ? "BAD_REQUEST"
          : codeKind === 2
            ? "GAME_NOT_FOUND"
            : codeKind === 3
              ? "INVALID_ACTION"
              : codeKind === 4
                ? "SYSTEM_UNAVAILABLE"
                : undefined;
      if (!code) {
        throw new Error(`Unknown Minesweeper error code ${codeKind}`);
      }
      decoded = {
        type: "error",
        requestId,
        payload: {code, message: reader.readString()},
      };
    } else {
      throw new Error(`Unknown Minesweeper event kind ${eventKind}`);
    }

    if (!reader.finished) {
      throw new Error("Minesweeper response has trailing data");
    }

    const result = minesweeperRuntimeEventSchema.safeParse(decoded);
    if (!result.success) {
      throw result.error;
    }
    return result.data;
  } catch (cause) {
    if (cause instanceof MinesweeperClientError) throw cause;
    throw new MinesweeperClientError(
      "protocol",
      "Minesweeper runtime sent an invalid binary event",
      {cause},
    );
  }
};
