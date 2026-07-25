import {describe, expect, test} from "bun:test";
import {minesweeperGameConfigSchema} from "../src/messages";
import {
  decodeMinesweeperResponse,
  encodeMinesweeperRequest,
} from "../src/protocol";

describe("Minesweeper protocol", () => {
  test("encodes the C++ create-game golden vector", () => {
    expect(
      [
        ...encodeMinesweeperRequest("r", "game", {
          type: "game.create",
          payload: {rows: 4, cols: 5, mines: 3},
        }),
      ],
    ).toEqual([
      0,
      2,
      0,
      1,
      "r".charCodeAt(0),
      1,
      0,
      4,
      ..."game".split("").map((value) => value.charCodeAt(0)),
      0,
      4,
      0,
      5,
      0,
      3,
    ]);
  });

  test("validates game configuration before it reaches the wire", () => {
    expect(
      minesweeperGameConfigSchema.safeParse({
        rows: 2,
        cols: 2,
        mines: 4,
      }).success,
    ).toBe(false);
  });

  test("decodes a runtime snapshot", () => {
    const event = decodeMinesweeperResponse(
      Buffer.from([
        0, 2,
        1,
        0, 7, ..."request".split("").map((value) => value.charCodeAt(0)),
        2,
        0, 4, ..."game".split("").map((value) => value.charCodeAt(0)),
        0, 0, 0, 0, 0, 0, 0, 0,
        1,
        0, 0, 0, 0,
        0, 0, 0, 1,
        0, 1,
        0, 1,
        1, 0,
      ]),
    );

    expect(event).toEqual({
      type: "game.snapshot",
      audience: "connection",
      gameId: "game",
      requestId: "request",
      payload: {
        revision: 0,
        status: "playing",
        elapsedSeconds: 0,
        remainingMines: 1,
        rows: 1,
        cols: 1,
        tiles: [
          {
            row: 0,
            col: 0,
            state: "hidden",
            flagged: false,
          },
        ],
      },
    });
  });

  test("decodes a correlated runtime error", () => {
    const event = decodeMinesweeperResponse(
      Buffer.from([
        0, 2,
        2,
        0, 1, "r".charCodeAt(0),
        3,
        0, 7, ..."blocked".split("").map((value) => value.charCodeAt(0)),
      ]),
    );

    expect(event).toEqual({
      type: "error",
      requestId: "r",
      payload: {code: "INVALID_ACTION", message: "blocked"},
    });
  });

  test("rejects trailing response data", () => {
    expect(() =>
      decodeMinesweeperResponse(
        Buffer.from([
          0, 2,
          2,
          0, 1, "r".charCodeAt(0),
          1,
          0, 0,
          99,
        ]),
      ),
    ).toThrow("invalid binary event");
  });
});
