# Finite State Machine Simulator

A command-driven deterministic finite state machine in C. The machine accepts
two symbols, `0` and `1`, and can be rewired while the program is running.
It reports states that cannot be reached from the selected start state and can
prune them without renumbering the remaining states.

## Build

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 src/fsm.c -o fsm
```

On Windows with GCC, use `-o fsm.exe`.

## Commands

Start with `fsm [number-of-states]` (default: 6). State IDs start at 0.
Every state initially loops to itself on both symbols.

| Command | Action |
| --- | --- |
| `set STATE SYMBOL TARGET` | Change one transition |
| `start STATE` | Select the start state and reset the current state |
| `reset` | Return to the start state |
| `step SYMBOL` | Take one transition from the current state |
| `run BITS` | Take a sequence of 0/1 transitions |
| `print` | Show active states and transitions |
| `reachable` | List states unreachable from the start state |
| `prune` | Remove unreachable states |
| `help` | Print command help |
| `quit` | Exit |

Blank lines and lines beginning with `#` are ignored. Commands with malformed
arguments report an error and leave the machine unchanged. Removed state IDs
cannot be reused in the current session.

## Example

```text
set 0 0 1
set 1 1 2
run 01
reachable
prune
print
quit
```

Run the integration checks with `python tests/test_cli.py`. Set `CC` to
select a different C compiler.

