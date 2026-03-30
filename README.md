# Number Grid Calculator

A lightweight desktop calculator built with **C++17** and **X11/Xft** — no GUI toolkit required.

![Number Grid Calculator](screenshot.png)

## Features

- Add multiple numbers to a list
- Select and delete individual entries
- Operations: **Sum** (more operations can be added)
- Clean X11 native UI with anti-aliased text via Xft

## Requirements

- g++ with C++17 support
- X11 development libraries
- Xft library

On RHEL/Fedora:
```bash
sudo dnf install gcc-c++ libX11-devel libXft-devel
```

On Debian/Ubuntu:
```bash
sudo apt install g++ libx11-dev libxft-dev
```

## Build

```bash
make
```

## Run

```bash
./myapp
```

## Usage

1. Click **Add Number** to enter a number
2. Select a number from the list to highlight it
3. Click **Delete Selected** to remove it
4. Choose an operation (e.g. Sum) via the `[v]` button
5. Click **Calculate** to see the result

## License

MIT
