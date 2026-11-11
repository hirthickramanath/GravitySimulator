# Gravity – 2D Gravitational Physics & Orbital Simulator

**P L Hirthick Ramanath** (26PT20) · **Svithin S** (26PT35)
Integrated M.Sc. Theoretical Computer Science, PSG College of Technology – Semester 1

## About

A two-body gravity simulator written in C with [raylib](https://www.raylib.com/).
Pick two bodies (a planet and the Sun, the Earth and the Moon, or anything you type in) and the program works out how they move under Newton's law of gravitation, using the semi-implicit Euler method.

- The time step is worked out automatically from Kepler's third law, so every run covers one full orbit (or a long enough stretch if the bodies escape).
- Bodies are saved in `bodies.txt` and can be loaded by name, added, edited or deleted.
- Every step of the run is written to `data.csv` (position, velocity, distance, force and energy).
- The run is played back in 3D with a live readout of time, distance, force, speed, energy, energy drift and orbit type.
- The window can be resized and everything scales with it.

## Installing raylib

| OS | How |
|---|---|
| Windows | Download the installer from **https://raysan5.itch.io/raylib** (linked from https://www.raylib.com). It installs raylib and a ready-to-use gcc (w64devkit) in `C:\raylib`. |
| Linux (Debian/Ubuntu) | Follow https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux, or run the commands below. |
| macOS | `brew install raylib pkg-config` (needs [Homebrew](https://brew.sh)). See https://github.com/raysan5/raylib/wiki/Working-on-macOS |

Linux, building raylib from source:
```
sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
git clone --depth 1 https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

## Compiling and running

Run these from inside this folder.

**Windows**

First, one-time setup so gcc can find itself and raylib from any folder:

1. Open Start, search **Edit the system environment variables**, then click **Environment Variables...**
2. Under *User variables*, select **Path** → **Edit** → **New**, and add `C:\raylib\w64devkit\bin`
3. Still under *User variables*, click **New** twice to add:
   - `C_INCLUDE_PATH` = `C:\raylib\raylib\src`
   - `LIBRARY_PATH` = `C:\raylib\raylib\src`
4. Press OK, then open a **new** terminal and check with `gcc --version`

Then compile and run:
```
gcc *.c -o gravity.exe -lraylib -lopengl32 -lgdi32 -lwinmm
gravity.exe
```

**Linux**
```
gcc *.c -o gravity -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./gravity
```

**macOS**
```
clang *.c -o gravity $(pkg-config --cflags --libs raylib)
./gravity
```

Keep `bodies.txt` in the same folder as the program.

## Files

| File | What it does |
|---|---|
| `main.c` | Opens the window and switches between screens |
| `screens.c` | Main menu, Load & Run, New Simulation, saving new bodies |
| `settings.c` | Auto or manual time step |
| `ui.c` | Background, buttons, text boxes, messages |
| `playback.c` | Reads `data.csv` and plays the run in 3D |
| `physics.c` | Orbit check, time step and the simulation |
| `loadData.c` | Reading and writing `bodies.txt` |
| `gravity.h` | Constants, structs and shared function declarations |
| `bodies.txt` | Saved bodies: `name symbol mass x y vx vy` (SI units) |

## Controls during a run

Space: pause · Up/Down: speed · V: velocity and force arrows · drag: rotate · mouse wheel: zoom · drag the bar: jump in time · Esc: back
