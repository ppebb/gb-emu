# gb-emu

Toy emulator I wrote for a course final project. This can probably only run Dr.
Mario, despite the MBC1 mapper being supported. It has timing issues so Tetris
does not run.

Passes all of Blaarg's CPU instruction tests. Fails memory and instruction
timing tests. I have not tried anything else.

## Building

1. Ensure you have GLFW 3 and glibc >= 2.17
2. Run `make`
3. The binary will be output in `./build/gb`

## Running

Use `gb [ROM]` to run a ROM file. See `gb -h` for additional options.

## Debugging

This project includes a step-by-step debugger. Enable it using `--step-debug`.
While paused, 3 commands are available
* bXXXX   - breakpoint a given memory address (only one may be set at a
            time)
* s       - disable step-by-step debugging, run until the next breakpoint
            (or forever)
* memXXXX - view 16 bytes in memory, beginning at the nearest alignment to
            the provided address

## Key Bindings

| Key   | Gameboy Key |
|-------|-------------|
| Z     | B           |
| X     | A           |
| I     | UP          |
| J     | LEFT        |
| K     | DOWN        |
| L     | RIGHT       |
| CTRL  | SELECT      |
| ENTER | START       |
