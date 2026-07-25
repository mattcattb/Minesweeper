export {
  MinesweeperClient,
  type MinesweeperClientOptions,
} from "./client.js";
export {
  MinesweeperClientError,
  type MinesweeperClientErrorKind,
} from "./errors.js";
export {
  minesweeperCommandSchema,
  minesweeperErrorSchema,
  minesweeperGameConfigSchema,
  minesweeperGameSnapshotSchema,
  minesweeperRuntimeEventSchema,
} from "./messages.js";
export {MINESWEEPER_PROTOCOL_VERSION} from "./protocol.js";
export type {
  MinesweeperCommand,
  MinesweeperError,
  MinesweeperGameConfig,
  MinesweeperGameSnapshot,
  MinesweeperRuntimeEvent,
  MinesweeperTile,
} from "./types.js";
