export type MinesweeperGameConfig = {
  rows: number;
  cols: number;
  mines: number;
};

export type MinesweeperCommand =
  | {type: "game.create"; payload: MinesweeperGameConfig}
  | {type: "tile.reveal"; payload: {row: number; col: number}}
  | {type: "tile.flag.toggle"; payload: {row: number; col: number}}
  | {type: "game.restart"}
  | {type: "game.resync"};

export type MinesweeperTile =
  | {
      row: number;
      col: number;
      state: "hidden";
      flagged: boolean;
    }
  | {
      row: number;
      col: number;
      state: "revealed";
      value: "mine" | number;
    };

export type MinesweeperGameSnapshot = {
  revision: number;
  status: "playing" | "won" | "lost";
  elapsedSeconds: number;
  remainingMines: number;
  rows: number;
  cols: number;
  tiles: MinesweeperTile[];
};

export type MinesweeperError = {
  code:
    | "BAD_REQUEST"
    | "GAME_NOT_FOUND"
    | "INVALID_ACTION"
    | "SYSTEM_UNAVAILABLE";
  message: string;
};

export type MinesweeperRuntimeEvent =
  | {
      type: "game.snapshot";
      audience: "game" | "connection";
      gameId: string;
      requestId: string;
      payload: MinesweeperGameSnapshot;
    }
  | {
      type: "error";
      requestId: string;
      payload: MinesweeperError;
    };
