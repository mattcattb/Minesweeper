import {z} from "zod";
import type {
  MinesweeperCommand,
  MinesweeperError,
  MinesweeperGameConfig,
  MinesweeperGameSnapshot,
  MinesweeperRuntimeEvent,
} from "./types.js";

const MAX_U16 = 0xffff;
const coordinateSchema = z.number().int().min(0).max(MAX_U16);

export const minesweeperGameConfigSchema = z
  .object({
    rows: z.number().int().positive().max(MAX_U16),
    cols: z.number().int().positive().max(MAX_U16),
    mines: z.number().int().positive().max(MAX_U16),
  })
  .strict()
  .refine(({rows, cols, mines}) => mines < rows * cols, {
    message: "Mine count must be smaller than the number of tiles",
    path: ["mines"],
  }) satisfies z.ZodType<MinesweeperGameConfig>;

const tileCoordinateSchema = z
  .object({
    row: coordinateSchema,
    col: coordinateSchema,
  })
  .strict();

export const minesweeperCommandSchema = z.discriminatedUnion("type", [
  z
    .object({
      type: z.literal("game.create"),
      payload: minesweeperGameConfigSchema,
    })
    .strict(),
  z
    .object({
      type: z.literal("tile.reveal"),
      payload: tileCoordinateSchema,
    })
    .strict(),
  z
    .object({
      type: z.literal("tile.flag.toggle"),
      payload: tileCoordinateSchema,
    })
    .strict(),
  z.object({type: z.literal("game.restart")}).strict(),
  z.object({type: z.literal("game.resync")}).strict(),
]) satisfies z.ZodType<MinesweeperCommand>;

const hiddenTileSchema = z
  .object({
    row: coordinateSchema,
    col: coordinateSchema,
    state: z.literal("hidden"),
    flagged: z.boolean(),
  })
  .strict();

const revealedTileSchema = z
  .object({
    row: coordinateSchema,
    col: coordinateSchema,
    state: z.literal("revealed"),
    value: z.union([z.literal("mine"), z.number().int().min(0).max(8)]),
  })
  .strict();

export const minesweeperGameSnapshotSchema = z
  .object({
    revision: z.number().int().nonnegative().safe(),
    status: z.enum(["playing", "won", "lost"]),
    elapsedSeconds: z.number().int().nonnegative().safe(),
    remainingMines: z.number().int().safe(),
    rows: z.number().int().positive().max(MAX_U16),
    cols: z.number().int().positive().max(MAX_U16),
    tiles: z.array(
      z.discriminatedUnion("state", [hiddenTileSchema, revealedTileSchema]),
    ),
  })
  .strict() satisfies z.ZodType<MinesweeperGameSnapshot>;

export const minesweeperErrorSchema = z
  .object({
    code: z.enum([
      "BAD_REQUEST",
      "GAME_NOT_FOUND",
      "INVALID_ACTION",
      "SYSTEM_UNAVAILABLE",
    ]),
    message: z.string(),
  })
  .strict() satisfies z.ZodType<MinesweeperError>;

export const minesweeperRuntimeEventSchema = z.discriminatedUnion("type", [
  z
    .object({
      type: z.literal("game.snapshot"),
      audience: z.enum(["game", "connection"]),
      gameId: z.string().min(1),
      requestId: z.string().min(1),
      payload: minesweeperGameSnapshotSchema,
    })
    .strict(),
  z
    .object({
      type: z.literal("error"),
      requestId: z.string(),
      payload: minesweeperErrorSchema,
    })
    .strict(),
]) satisfies z.ZodType<MinesweeperRuntimeEvent>;
