# Minesweeper — C + SDL2

A classic **Minesweeper** game built in C with a full graphical interface powered by **SDL2**. Features multiple difficulty levels, explosion animations, sound effects, high score tracking, and a smooth menu launcher.

---

## Features

- **Graphical 2D gameplay** using SDL2 (window, renderer, textures, fonts)
- **Three difficulty levels** — Easy, Medium, Hard (up to 20×20 grid)
- **Explosion particle animation** when a mine is hit
- **Sound effects** — click, flag, explosion, game over, win
- **High score tracking** (top 5 scores per difficulty)
- **Flag system** — right-click to flag suspected mines
- **Safe first click** — no mine on the first tile you reveal
- **Flashing highlight** on hover
- **Terminal menu launcher** (text-based) + SDL2 GUI launcher

---

## Project Structure

```
Minesweeper/
│
├── 2d.c          # Full SDL2 graphical Minesweeper (main game)
├── 2d (1).c      # Terminal/console version of the game logic
├── page.c        # Terminal-based menu launcher
├── page (1).c    # SDL2 GUI menu launcher
└── README.md
```

> **Note**: `2d.c` is the primary playable version. `page.c` / `page (1).c` are launcher menus that call the game executable.

---

## Dependencies

| Library | Purpose |
|---------|---------|
| `SDL2` | Window creation, rendering, input handling |
| `SDL2_ttf` | Font rendering for numbers and UI text |
| `SDL2_image` | Loading image textures (flag, bomb icons) |
| `SDL2_mixer` | Sound effects playback |

---

## Installation

### On Linux / Ubuntu

```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev
```

### On Windows
Download the SDL2 development libraries from [libsdl.org](https://libsdl.org) and link them via your compiler (MinGW / MSVC).

---

## Build & Run

### Compile the main game

```bash
gcc 2d.c -o minesweeper \
    $(sdl2-config --cflags --libs) \
    -lSDL2_ttf -lSDL2_image -lSDL2_mixer \
    -lm -o minesweeper
```

### Run

```bash
./minesweeper
```

### Compile and run the terminal menu launcher

```bash
gcc page.c -o menu
./menu
```

---

## How to Play

| Action | Control |
|--------|---------|
| Reveal tile | Left Click |
| Flag / Unflag tile | Right Click |
| Restart game | R key |
| Exit | Close window / ESC |

- **Numbers** on revealed tiles show how many mines are in the 8 surrounding tiles.
- **Flag** all mines and reveal all safe tiles to **win**.
- **Hit a mine** → explosion animation + game over.

---

## Game Modes

| Difficulty | Grid Size | Mines |
|------------|-----------|-------|
| Easy | Small | Few |
| Medium | Medium | Moderate |
| Hard | 20×20 | Many |

---

## Authors

Developed as a C programming project using SDL2.

## Author

**Mridupawan Kalita**
B.Tech ES | Indian Institute of Technology Jodhpur
[Github Profile](https://github.com/MriduKalita23)

**Ankit Kumar Goyal**
B.Tech ME | Indian Institute of Technology Jodhpur
[Github Profile](https://github.com/ankit2006173)

**Harshal Joshi**
B.Tech CI | Indian Institute of Technology Jodhpur
[Github Profile](https://github.com/Harshal2108)

---

## License

This project is for educational purposes.
