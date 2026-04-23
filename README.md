# Graph Traversal Algorithm Visualizer

A premium, highly interactive C++ desktop application designed to visualize and mathematically demonstrate the core mechanics of Graph Traversal and Pathfinding algorithms. Built using **SFML 3.2** and **CMake**, this tool allows you to physically draw walls, place dynamic weights (Grass and Mud), and watch standard algorithms explore the grid in real-time.

This project was built as part of the **Design and Analysis of Algorithms (DAA)** course.

## 🚀 Features

- **Interactive Grid System**: Draw impassable walls, dynamic penalty weights (Cost 30 Grass, Cost 50 Mud), and move Start/End nodes seamlessly.
- **Cinematic Visualization**: Employs Frame-Batching thread architecture for smooth, visually traceable algorithmic expansion waves.
- **Dynamic Terrain Memory**: Advanced terrain tracking ensures the algorithm safely traverses over Mud and Grass without erasing them from the board.
- **Real-Time Maze Generation**: Instantly carve complex Spanning-Tree Mazes using a randomized Depth-First Search backtracker.
- **Multi-Threaded Architecture**: Pathfinding runs on a detached background thread, ensuring the UI remains fluid and responsive even during 10,000+ node evaluations.

## 🧠 Core Algorithms Implemented

### 1. A* Search Algorithm (Optimal & Fast)
A* evaluates nodes using the formula `F = G + H`. It combines the actual distance traveled from the start (`G-cost`) with a mathematical heuristic estimating the distance to the target (`H-cost`). This guarantees the shortest possible path while exploring significantly fewer nodes than Dijkstra.

### 2. Dijkstra's Algorithm (Optimal)
Dijkstra expands uniformly outward from the start node by evaluating purely based on the actual distance traveled (`G-cost`). It guarantees the absolute shortest path but is "blind" to the target's direction, making it computationally heavy.

### 3. Greedy Best-First Search (Sub-optimal, Extremely Fast)
Greedy BFS ignores the distance traveled and evaluates purely on the heuristic (`H-cost`) to aggressively head straight toward the target. It is incredibly fast but does not guarantee the shortest path, especially around heavy obstacles.

### 4. Breadth-First Search (Unweighted)
Standard BFS expands layer by layer. It is unweighted (treating Grass, Mud, and Normal terrain identically as 1 step). It guarantees the shortest path *only* in unweighted grid configurations.

### 5. Depth-First Search Maze Generator
Utilizes a stack-based randomized DFS to carve corridors through a solid block of walls. It leaps 2 grid spaces at a time, knocking down the intermediate wall, to create a perfect, loop-free spanning tree maze.

---

## 📂 Project Directory Structure

```text
.
├── .vscode/                 # IDE Configuration (IntelliSense)
├── assets/                  # Fonts and media resources
│   └── GoogleSans-Medium.ttf
├── build/                   # Compiled binaries and object files (Generated)
├── CMakeLists.txt           # Build configuration script
├── PathVisualizer.cpp       # Core Monolithic Source Code (1000+ Lines)
├── README.md                # Project documentation
├── setup_and_run.ps1        # Automated setup script (Windows PowerShell)
└── .gitignore               # Git untracked files logic
```

---

## 🛠️ Setup & Installation

This project is built using C++17 and relies on **SFML 3.2** for graphics rendering. 

### Method 1: The Automated Setup Script (Convenient)
If you already have your C++ compiler and SFML installed, you can skip the manual build process.
1. Right-click the `setup_and_run.ps1` file and click **Run with PowerShell**.
2. The script will automatically create the build folder, run CMake, compile the source code, and launch the visualizer immediately.

### Method 2: MSYS2 Installation (Windows Recommended)
If you are starting from scratch on Windows, we highly recommend using the MSYS2 (UCRT64) environment to effortlessly install the compiler and SFML binaries.

1. Download and install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal.
3. Run the following command to download the compiler, CMake, and SFML 3.x libraries instantly:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sfml
   ```
4. Navigate to the project directory in your terminal:
   ```bash
   cd path/to/Design_and_Analysis_of_Algorithms
   ```
5. Build the project using CMake:
   ```bash
   mkdir build
   cd build
   cmake -G Ninja ..
   cmake --build .
   ```
6. Run the executable:
   ```bash
   ./PathVisualizer.exe
   ```

---

## 🎮 How to Use the Visualizer

- **Algorithm Radio Buttons**: Select the graph traversal algorithm you want to demonstrate (A*, Dijkstra, Greedy, BFS).
- **Tool Palette**: Select a "Brush" to paint the grid.
  - **Wall (Dark Slate)**: Impassable terrain. The algorithm must find a way around it.
  - **Grass (Cost 30)**: Semi-difficult terrain. It costs 3x more energy to walk through than normal space.
  - **Mud (Cost 50)**: Highly difficult terrain. Costs 5x more energy.
- **Start / End Nodes**: Place your origin and destination.
- **Diagonal Toggle**: Switches the physics engine to allow 8-way directional movement (Octile Math) instead of 4-way orthogonal movement (Manhattan Math).
- **RUN**: Dispatches the algorithm to a background thread to calculate the path visually.
- **Clear Path**: Wipes the algorithmic visualization (the blue/yellow wave) while perfectly preserving your drawn walls, grass, and mud.
- **Clear All**: Nuke the board completely.

---
