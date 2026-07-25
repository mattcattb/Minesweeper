export type MinesweeperClientErrorKind =
  | "configuration"
  | "input"
  | "connection"
  | "timeout"
  | "protocol"
  | "closed";

export class MinesweeperClientError extends Error {
  readonly name = "MinesweeperClientError";

  constructor(
    readonly kind: MinesweeperClientErrorKind,
    message: string,
    options?: ErrorOptions,
  ) {
    super(message, options);
  }
}
