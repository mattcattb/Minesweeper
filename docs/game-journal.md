# Game journal learning plan

The journal should persist accepted game states without becoming part of the
Minesweeper rules. `GameState` remains the authoritative domain value; the
journal only translates complete accepted states to and from an ordered byte
stream.

## First record model

Start with a complete state record rather than command or event deltas:

```cpp
struct GameRecord {
  std::string game_id;
  std::uint64_t revision;
  minesweeper::GameState state;
};
```

A complete record is larger than a `RevealTile` command, but it preserves the
exact mine layout and remains recoverable if gameplay rules change later.

## File framing

Store a sequence of length-prefixed JSON payloads:

```text
[4-byte unsigned big-endian length][JSON GameRecord]
[4-byte unsigned big-endian length][JSON GameRecord]
...
```

The framing layer answers where one record ends. JSON answers how fields inside
that record are represented. Keep those two concerns separate while coding.

Before allocating a payload buffer, reject lengths above a fixed maximum based
on the largest supported board. A clean EOF before any byte of the next length
means replay is complete. EOF after part of a length or payload means the tail
is truncated and recovery must fail explicitly.

## Concrete API to build

Create `src/game_journal.hpp/.cpp` only when beginning the exercise:

```cpp
class GameJournal {
public:
  explicit GameJournal(std::filesystem::path path);

  std::vector<GameRecord> replay() const;
  void append(const GameRecord& record);

private:
  std::filesystem::path path_;
};
```

This is deliberately concrete. Do not add a generic repository or persistence
interface until another implementation creates an actual need for one.

## Append and command ordering

Use the copyable core state as a transaction candidate:

```text
copy current GameState
  -> apply reveal/flag/restart to candidate
  -> create GameRecord with revision + 1
  -> append and flush the complete record
  -> replace live state with candidate
  -> advance live revision
  -> publish the success snapshot
```

If append fails, discard the candidate. The live game and visible revision stay
unchanged. This gives the client a meaningful durability boundary: published
success corresponds to a journaled state.

`RequestSnapshot` is read-only and must not append a record. Rejected commands
must not append records either.

## Replay behavior

Replay into a temporary map rather than directly into `GameRuntime::games_`:

```text
read frame
  -> parse JSON
  -> decode GameRecord
  -> validate_game(record.state)
  -> validate revision ordering
  -> replace that game in a temporary map
  -> continue until clean EOF
  -> install the complete temporary map
```

For an uncompacted first journal, require the first record for a game to have
revision `0` and each later record to have the previous revision plus one.
Reject duplicate, decreasing, or skipped revisions.

Do not start the socket listener until replay finishes successfully. A corrupt
journal should fail startup rather than silently create an empty runtime.

## Flush and sync policy

Implement and understand these levels separately:

1. `write`: bytes enter the C++ or operating-system buffering path.
2. `flush`: C++ userspace buffering is sent to the file descriptor.
3. `fsync`: the operating system is asked to make file contents durable.
4. `close`: resources are released and pending errors can surface.

Begin by flushing each accepted record and checking every write. Then add a
configurable durability policy as a separate exercise:

```text
always       fsync before acknowledging every mutation
every-second flush each record, fsync on a periodic owner-thread command
no-sync      rely on operating-system writeback
```

A timer thread must enqueue a sync command; it must not touch the journal or
game map concurrently with the server's mutation owner.

## Suggested implementation milestones

1. Encode and decode one `GameRecord` through an in-memory string.
2. Frame and read one record through an in-memory byte stream.
3. Read several adjacent frames and confirm reader-position behavior.
4. Reject oversized, malformed, and truncated frames.
5. Append records to a temporary file and replay them after reopening.
6. Enforce per-game revision ordering during replay.
7. Connect append-before-commit to create, reveal, flag, and restart.
8. Replay before the server accepts CLI or socket commands.
9. Add explicit flush/fsync behavior and failure tests.
10. Add compaction only after journal growth becomes visible.

## Tests that define completion

- Create, reveal, flag, stop, replay, and recover the same public snapshot.
- Preserve exact hidden mine locations internally.
- Preserve elapsed duration while excluding server downtime.
- Ignore no errors: malformed JSON, invalid state, revision gaps, and partial
  tails must all be distinguishable.
- A failed append must not mutate or publish the candidate state.
- Read-only and rejected commands must not grow the journal.

## Later: snapshot plus journal

Once replay time or file growth becomes a real problem, add an atomic snapshot:

```text
load latest snapshot at revision/sequence N
  -> replay journal records newer than N
  -> reconstruct current state
```

Compaction writes the latest complete states to a temporary file, flushes and
syncs it, atomically renames it over the old checkpoint, and only then retires
the covered journal records. This later stage connects directly to Redis AOF
rewriting, database checkpoints, and blockchain state snapshots.
