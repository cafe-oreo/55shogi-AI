# MICS2_3A – 5×5 Shogi AI Implementation

This project implements a 5×5 Shogi (mini-shogi) AI using game-tree search  
based on the Negamax algorithm with Alpha–Beta pruning.

The code was developed as part of the **MICS Experiment 2 (3a)** assignment.

---

## Overview

- Game: 5×5 Shogi (mini-shogi)
- Search algorithms:
  - Negamax search
  - Alpha–Beta pruning
- Goals:
  - Implement a basic Shogi AI
  - Experiment with game-tree search and evaluation functions

---

## Build

Command-line build with g++ and make:

```bash
cd source
make COMPILER=g++ -j
```

---

## Run

After building with make, run:

```bash
./minishogi-by-gcc
```

Use the corresponding executable name on your platform if it differs.

---

## Project structure

```text
MICS2_3A/
├── .vs/ # Visual Studio workspace files
├── .vscode/ # VS Code settings
├── Ayane/ # Local resources / personal data (optional)
├── build/ # Build output directory
├── docs/ # Documentation, reports, notes
├── lecture/ # Assignment and lecture material
├── obj/ # Object files
├── script/ # Helper scripts (build, run, etc.)
├── source/ # C++ source code for the AI
│ └── ... # main.cpp and other implementation files
├── .gitignore
├── CppProperties.json # VS Code C++ configuration
├── mics2_3a.sln # Visual Studio solution
└── README.md
```
