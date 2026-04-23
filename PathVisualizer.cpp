#include <SFML/Graphics.hpp>
#include <vector>
#include <queue>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <limits.h>
#include <string>
#include <mutex>
#include <iostream>
#include <filesystem>
#include <random>
#include <stack>
#include <map>
#include <cstdint>
#include <list>

// ==============================================================================
// CONFIGURATION CONSTANTS
// These constants define the dimensions and structural behavior of the visualizer
// ==============================================================================
const int COLS = 50;                     // Total number of columns in the grid
const int NODE_WIDTH = 25;               // Size of each square node in pixels (25x25)
const int ROWS = 34;                     // Total number of rows in the grid
const int GRID_WIDTH = COLS * NODE_WIDTH; // Total window width for the grid area (1250px)
const int GRID_HEIGHT = ROWS * NODE_WIDTH;// Total window height for the grid area (850px)
const int UI_HEIGHT = 130;               // Height of the bottom control panel (130px)
const int WINDOW_WIDTH = GRID_WIDTH;     // Full window width
const int WINDOW_HEIGHT = GRID_HEIGHT + UI_HEIGHT; // Full window height (Grid + UI)

// Mathematical constants for heuristic calculations
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Movement Costs
// We use 10 for normal cost, 30 for grass, and 50 for mud.
// Diagonal cost is scaled to 1.414 (sqrt(2)), which resolves to 14 when cast to int.
const double DIAGONAL_COST = 1.414; 

// ==============================================================================
// STATE ENUMS
// Represent the different algorithmic modes, execution states, and drawing tools
// ==============================================================================

// Defines the currently selected graph traversal algorithm
enum class Algorithm
{
    A_STAR,         // A* Search (Uses both distance from start G, and distance to target H)
    DIJKSTRA,       // Dijkstra's Algorithm (Explores purely based on distance from start G)
    GREEDY_BFS,     // Greedy Best-First Search (Explores purely based on distance to target H)
    BFS             // Breadth-First Search (Explores unweighted uniform layers)
};

// Controls whether the main thread should allow user input or wait for calculation
enum class ProgramState
{
    IDLE,           // User can click, drag, change algorithms
    RUNNING         // An algorithm is actively calculating on a separate thread
};

// Defines the current "brush" the user is holding to paint the grid
enum class Tool
{
    START,          // Place the starting node (Emerald Green)
    END,            // Place the target node (Coral Red)
    WALL,           // Draw impassable walls (Dark Slate)
    GRASS,          // Draw low-penalty terrain (Cost 30)
    MUD             // Draw high-penalty terrain (Cost 50)
};

// ==============================================================================
// PREMIUM COLOR PALETTE
// Adjusted to a modern, professional, high-contrast visual aesthetic
// ==============================================================================
const sf::Color COLOR_BG = sf::Color(245, 245, 245);          // Clean light grey background for the grid
const sf::Color COLOR_WALL = sf::Color(33, 43, 54);           // Rich dark slate for impassable walls
const sf::Color COLOR_START = sf::Color(0, 167, 111);         // Vibrant Emerald Green for the starting position
const sf::Color COLOR_END = sf::Color(255, 86, 48);           // Vibrant Coral Red for the target destination
const sf::Color COLOR_OPEN = sf::Color(142, 202, 230, 150);   // Soft pastel blue for nodes queued for exploration
const sf::Color COLOR_CLOSED = sf::Color(255, 183, 3, 100);   // Warm amber for nodes that have been fully explored
const sf::Color COLOR_PATH = sf::Color(251, 133, 0);          // Bright solid orange for the final calculated path
const sf::Color COLOR_GRID = sf::Color(223, 227, 232);        // Subtle light grey for the grid intersection lines
const sf::Color COLOR_GRASS = sf::Color(170, 219, 170);       // Soft mint green indicating low-penalty terrain
const sf::Color COLOR_MUD = sf::Color(188, 154, 136);         // Earthy clay color indicating high-penalty terrain

// Premium Dark Mode UI Panel Colors
const sf::Color COLOR_PANEL_BG = sf::Color(22, 28, 36);       // Deep dark slate for the bottom control panel
const sf::Color COLOR_BUTTON = sf::Color(40, 50, 60);         // Sleek dark grey for unselected buttons
const sf::Color COLOR_BUTTON_HOVER = sf::Color(55, 68, 80);   // Slightly lighter grey when the mouse hovers
const sf::Color COLOR_BUTTON_ACTIVE = sf::Color(0, 167, 111); // Brilliant green matching the start node for active state
const sf::Color COLOR_BUTTON_RUN = sf::Color(34, 197, 94);    // Energetic bright green for the RUN action
const sf::Color COLOR_BUTTON_CLEAR = sf::Color(239, 68, 68);  // Alert bright red for the Clear All action
const sf::Color COLOR_TEXT = sf::Color(244, 246, 248);        // Crisp off-white text for excellent readability
const sf::Color COLOR_TEXT_LIGHT = sf::Color::White;          // Pure white text for active highlighting
const sf::Color COLOR_SHADOW = sf::Color(0, 0, 0, 120);       // Deep semi-transparent black for button drop-shadows
const sf::Color COLOR_WEIGHT_TEXT = sf::Color(33, 43, 54, 180); // Subtle dark text for terrain weight labels

// ==============================================================================
// NODE CLASS
// Represents a single square cell on the pathfinding grid.
// Maintains its logical state, physical bounds, neighboring connections, and rendering.
// ==============================================================================
class Node
{
public:
    int row, col;           // The logical grid indices
    int x, y;               // The physical pixel coordinates on the window
    int width, height;      // The pixel dimensions of the node
    sf::Color color;        // The current visual color mapping to its state
    std::vector<Node *> neighbors; // Adjacent nodes (Up, Down, Left, Right, Diagonals)
    int cost;               // The traversal cost (10 = normal, 30 = grass, 50 = mud, INT_MAX = wall)
    int original_cost;      // Remembers the terrain cost so we can safely clear the path visuals later

    /**
     * @brief Construct a new Node object
     * Initializes physical pixel placement and sets default background color and traversal cost.
     */
    Node(int row, int col, int node_width, int node_height)
        : row(row), col(col), width(node_width), height(node_height)
    {
        x = col * width;
        y = row * height;
        color = COLOR_BG;
        cost = 10;
        original_cost = 10;
    }

    /**
     * @brief Checks if the node is completely impassable.
     * @return true if the node is a Wall.
     */
    bool is_wall() const { return cost == INT_MAX; }

    // --- State Modifier Functions ---
    // These functions change the node's visual color and mathematical traversal cost.

    void reset()
    {
        color = COLOR_BG;
        cost = 10;
        original_cost = 10;
    }
    void make_wall()
    {
        color = COLOR_WALL;
        cost = INT_MAX; // Mathematical infinity to block traversal
        original_cost = INT_MAX;
    }
    void make_start()
    {
        color = COLOR_START;
        cost = 10;
        original_cost = 10;
    }
    void make_end()
    {
        color = COLOR_END;
        cost = 10;
        original_cost = 10;
    }
    
    // Algorithm visual states (do not change underlying terrain cost)
    void make_open() { color = COLOR_OPEN; }
    void make_closed() { color = COLOR_CLOSED; }
    void make_path() { color = COLOR_PATH; }
    
    // Terrain modifications
    void make_grass()
    {
        color = COLOR_GRASS;
        cost = 30; // 3x harder to cross than a standard node
        original_cost = 30;
    }
    void make_mud()
    {
        color = COLOR_MUD;
        cost = 50; // 5x harder to cross than a standard node
        original_cost = 50;
    }

    /**
     * @brief Renders the physical node to the screen.
     * @param window The main SFML window
     * @param font The loaded font for drawing text
     */
    void draw(sf::RenderWindow &window, const sf::Font &font) const
    {
        // We inset the drawing shape by 1 pixel on all sides to naturally form the grid lines
        const float gap = 1.f;
        sf::RectangleShape shape;
        shape.setPosition({static_cast<float>(x) + gap, static_cast<float>(y) + gap});
        shape.setSize({static_cast<float>(width) - gap, static_cast<float>(height) - gap});
        shape.setFillColor(color);
        window.draw(shape);

        // If the node represents difficult terrain, draw a small text label indicating its cost
        bool is_grass = (original_cost == 30);
        bool is_mud = (original_cost == 50);
        if (is_grass || is_mud)
        {
            sf::Text weightLabel(font, is_grass ? "30" : "50", 11);
            weightLabel.setFillColor(COLOR_WEIGHT_TEXT);
            sf::FloatRect lb = weightLabel.getLocalBounds();
            // Perfectly center the text inside the node bounds
            weightLabel.setOrigin({lb.position.x + lb.size.x / 2.f,
                                   lb.position.y + lb.size.y / 2.f});
            weightLabel.setPosition({static_cast<float>(x) + width / 2.f,
                                     static_cast<float>(y) + height / 2.f});
            window.draw(weightLabel);
        }
    }

    /**
     * @brief Dynamically updates the neighbor list.
     * Evaluates adjacent grid cells (Up, Down, Left, Right) and conditionally (Diagonals)
     * to verify if they are valid traversal targets (i.e. not walls or out of bounds).
     * @param grid The complete 2D grid
     * @param diagonal_enabled True if 8-way movement is allowed
     */
    void update_neighbors(std::vector<std::vector<Node>> &grid, bool diagonal_enabled)
    {
        neighbors.clear();
        neighbors.reserve(8); // Max possible neighbors is 8

        int total_rows = grid.size();
        int total_cols = grid[0].size();

        // 4-way orthogonal checks
        if (row < total_rows - 1 && !grid[row + 1][col].is_wall()) // DOWN
            neighbors.push_back(&grid[row + 1][col]);
        if (row > 0 && !grid[row - 1][col].is_wall()) // UP
            neighbors.push_back(&grid[row - 1][col]);
        if (col < total_cols - 1 && !grid[row][col + 1].is_wall()) // RIGHT
            neighbors.push_back(&grid[row][col + 1]);
        if (col > 0 && !grid[row][col - 1].is_wall()) // LEFT
            neighbors.push_back(&grid[row][col - 1]);

        // 4-way diagonal checks (if active)
        if (diagonal_enabled)
        {
            // Bug Fix: We enforce that both adjacent orthogonal nodes must NOT be walls.
            // This prevents "Corner Cutting" (squeezing magically through the sharp edges of two walls).
            
            if (row > 0 && col > 0 && !grid[row - 1][col - 1].is_wall()) // TOP-LEFT
                if (!grid[row - 1][col].is_wall() && !grid[row][col - 1].is_wall())
                    neighbors.push_back(&grid[row - 1][col - 1]);
                    
            if (row < total_rows - 1 && col > 0 && !grid[row + 1][col - 1].is_wall()) // BOTTOM-LEFT
                if (!grid[row + 1][col].is_wall() && !grid[row][col - 1].is_wall())
                    neighbors.push_back(&grid[row + 1][col - 1]);
                    
            if (row > 0 && col < total_cols - 1 && !grid[row - 1][col + 1].is_wall()) // TOP-RIGHT
                if (!grid[row - 1][col].is_wall() && !grid[row][col + 1].is_wall())
                    neighbors.push_back(&grid[row - 1][col + 1]);
                    
            if (row < total_rows - 1 && col < total_cols - 1 && !grid[row + 1][col + 1].is_wall()) // BOTTOM-RIGHT
                if (!grid[row + 1][col].is_wall() && !grid[row][col + 1].is_wall())
                    neighbors.push_back(&grid[row + 1][col + 1]);
        }
    }
};

// ==============================================================================
// BUTTON CLASS
// Handles rendering, hover interpolation, and click logic for the UI panel.
// ==============================================================================
class Button
{
public:
    sf::RectangleShape shape;   // The main button box
    sf::RectangleShape shadow;  // The drop shadow box rendered slightly offset behind it
    sf::Text label;             // The button text
    sf::Color baseColor;        // The default color
    sf::Color hoverColor;       // The color when hovered by mouse
    sf::Color currentColor;     // The currently transitioning color
    bool is_active = false;     // True if the button is currently the selected tool/mode
    bool is_toggle = false;     // True if the button acts as an on/off switch instead of a radio select

    /**
     * @brief Construct a new Button object
     * Positions the button, constructs the drop shadow, and mathematically centers the text.
     */
    Button(sf::Vector2f position, sf::Vector2f size, sf::Color color, sf::Color hover,
           std::string text, sf::Font &font)
        : label(font, text, 20), baseColor(color), hoverColor(hover), currentColor(color)
    {
        // Drop-shadow configuration: offsets 3 pixels down and right
        shadow.setSize(size);
        shadow.setPosition({position.x + 3.f, position.y + 3.f});
        shadow.setFillColor(COLOR_SHADOW);

        shape.setPosition(position);
        shape.setSize(size);
        shape.setFillColor(baseColor);
        // Subtle inner outline to give a crisp border
        shape.setOutlineThickness(1);
        shape.setOutlineColor(sf::Color(100, 100, 100, 150));

        label.setFillColor(COLOR_TEXT);

        // Precise text centering logic using local bounds
        sf::FloatRect tb = label.getLocalBounds();
        label.setOrigin({tb.position.x + tb.size.x / 2.f,
                         tb.position.y + tb.size.y / 2.f});
        label.setPosition({position.x + size.x / 2.f,
                           position.y + size.y / 2.f});
    }

    /**
     * @brief Updates hover states and color interpolation.
     * Called every frame to allow smooth color blending.
     * @param mouse_pos Current mouse coordinate vector
     */
    void update(sf::Vector2i mouse_pos)
    {
        // Check if mouse is physically over the button bounds
        bool hovered = shape.getGlobalBounds().contains(
            {static_cast<float>(mouse_pos.x), static_cast<float>(mouse_pos.y)});

        // Determine target color based on activity and hover
        sf::Color target = is_active ? COLOR_BUTTON_ACTIVE : baseColor;
        if (hovered && !is_active)
            target = hoverColor;

        // Linear Interpolation (Lerp) function to smoothly transition RGB values
        auto lerp = [](std::uint8_t a, std::uint8_t b, float t) -> std::uint8_t
        {
            return static_cast<std::uint8_t>(a + (b - a) * t);
        };
        const float T = 0.18f; // Transition speed
        
        // Update current color smoothly towards the target color
        currentColor = sf::Color(
            lerp(currentColor.r, target.r, T),
            lerp(currentColor.g, target.g, T),
            lerp(currentColor.b, target.b, T),
            255);
        shape.setFillColor(currentColor);

        // Highlight text pure white if the button is active
        label.setFillColor(is_active ? COLOR_TEXT_LIGHT : COLOR_TEXT);
    }

    /**
     * @brief Validates if the button was clicked.
     */
    bool is_clicked(sf::Vector2i mouse_pos) const
    {
        return shape.getGlobalBounds().contains(
            {static_cast<float>(mouse_pos.x), static_cast<float>(mouse_pos.y)});
    }

    /**
     * @brief Flips the active state if this is a toggleable button.
     */
    void toggle()
    {
        if (is_toggle)
            is_active = !is_active;
    }

    void draw(sf::RenderWindow &window) const
    {
        window.draw(shadow);
        window.draw(shape);
        window.draw(label);
    }
};

// ==============================================================================
// CORE ALGORITHM HELPER FUNCTIONS
// Defines heuristic math, grid management, and validation logic.
// ==============================================================================

/**
 * @brief Heuristic function (H-Score) used by A* and Greedy BFS to estimate distance to target.
 * We use the Manhattan Distance for orthogonal grids, and Octile Distance for diagonal grids.
 * @param node1 Current node
 * @param node2 Target end node
 * @param diagonal_enabled True if 8-way movement is active
 * @return int The mathematically estimated cost to reach the end
 */
int h(const Node *node1, const Node *node2, bool diagonal_enabled)
{
    int dx = std::abs(node1->row - node2->row);
    int dy = std::abs(node1->col - node2->col);

    if (diagonal_enabled)
    {
        // Octile Distance Formula. 
        // 10 is the base orthogonal cost. 14 is the scaled diagonal cost (10 * 1.414).
        return 10 * (dx + dy) + (14 - 2 * 10) * std::min(dx, dy);
    }
    else
    {
        // Manhattan Distance Formula
        return 10 * (dx + dy);
    }
}

/**
 * @brief Comparator functor for the Priority Queue.
 * Sorts nodes so that the node with the lowest F-Score is evaluated first.
 */
struct CompareNode
{
    bool operator()(const std::pair<int, Node *> &a, const std::pair<int, Node *> &b) const
    {
        return a.first > b.first;
    }
};

/**
 * @brief Constructs the 2D grid matrix of Node objects.
 */
std::vector<std::vector<Node>> make_grid(int rows, int cols, int node_width, int node_height)
{
    std::vector<std::vector<Node>> grid;
    grid.reserve(rows);

    for (int j = 0; j < rows; ++j)
    {
        grid.emplace_back();
        grid[j].reserve(cols);
        for (int i = 0; i < cols; ++i)
        {
            grid[j].emplace_back(j, i, node_width, node_height);
        }
    }
    return grid;
}

/**
 * @brief Renders physical line vectors separating the node squares.
 */
void draw_grid_lines(sf::RenderWindow &window, int rows, int cols, int node_width, int node_height)
{
    for (int i = 0; i <= cols; ++i)
    {
        sf::Vertex line[2];
        line[0].position = {static_cast<float>(i * node_width), 0.f};
        line[0].color = COLOR_GRID;
        line[1].position = {static_cast<float>(i * node_width), static_cast<float>(GRID_HEIGHT)};
        line[1].color = COLOR_GRID;
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
    for (int j = 0; j <= rows; ++j)
    {
        sf::Vertex line[2];
        line[0].position = {0.f, static_cast<float>(j * node_height)};
        line[0].color = COLOR_GRID;
        line[1].position = {static_cast<float>(GRID_WIDTH), static_cast<float>(j * node_height)};
        line[1].color = COLOR_GRID;
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

/**
 * @brief Calculates which exact Node pointer the mouse coordinates currently hover over.
 */
Node *get_clicked_node(sf::Vector2i mouse_pos, const std::vector<std::vector<Node>> &grid)
{
    if (mouse_pos.y >= GRID_HEIGHT || mouse_pos.y < 0) return nullptr;
    if (mouse_pos.x >= GRID_WIDTH || mouse_pos.x < 0) return nullptr;

    int col_index = mouse_pos.x / NODE_WIDTH;
    int row_index = mouse_pos.y / NODE_WIDTH;

    if (row_index >= 0 && row_index < grid.size() && col_index >= 0 && col_index < grid[0].size())
    {
        return const_cast<Node *>(&grid[row_index][col_index]);
    }
    return nullptr;
}

/**
 * @brief Wipes purely algorithmic data (path, open, closed states) while preserving walls/terrain.
 * BUG FIX: This accurately restores Grass and Mud based on their original_cost instead of wiping them.
 */
void clear_path(std::vector<std::vector<Node>> &grid)
{
    for (auto &row_vec : grid)
    {
        for (auto &node : row_vec)
        {
            if (node.color == COLOR_OPEN || node.color == COLOR_CLOSED || node.color == COLOR_PATH)
            {
                if (node.original_cost == 30) node.make_grass();
                else if (node.original_cost == 50) node.make_mud();
                else node.reset();
            }
            // Ensure terrain costs remain locked in place if they weren't touched
            if (node.color == COLOR_GRASS) node.make_grass();
            if (node.color == COLOR_MUD) node.make_mud();
            if (node.color == COLOR_START) node.make_start();
            if (node.color == COLOR_END) node.make_end();
        }
    }
}

/**
 * @brief Nuclear wipe of the entire board state. Returns to absolute blank canvas.
 */
void clear_all(std::vector<std::vector<Node>> &grid, Node *&start, Node *&end)
{
    for (auto &row_vec : grid)
        for (auto &node : row_vec)
            node.reset();
            
    start = nullptr;
    end = nullptr;
}

/**
 * @brief Validates if the pathfinding algorithm has the required parameters to run.
 */
bool validate_pathfinding_input(Node *start, Node *end)
{
    if (!start || !end) return false;
    if (start == end) return false;
    if (start->is_wall() || end->is_wall()) return false;
    return true;
}

// ==============================================================================
// MAZE GENERATION (DFS)
// Implements a Randomized Depth-First Search algorithm to carve a spanning tree maze.
// ==============================================================================
void generate_maze_dfs(std::vector<std::vector<Node>> &grid, Node *start_node, std::mt19937 &rng)
{
    std::stack<Node *> stack;
    std::unordered_set<Node *> visited;

    // Step 1: Fill the entire grid with impenetrable walls
    for (auto &row : grid)
        for (auto &node : row)
            node.make_wall();

    // Step 2: Carve the starting point
    Node *current = start_node;
    current->reset();
    stack.push(current);
    visited.insert(current);

    int total_rows = grid.size();
    int total_cols = grid[0].size();

    // Step 3: Continuously dig passages 2 spaces at a time and knock out the wall between
    while (!stack.empty())
    {
        current = stack.top();
        std::vector<Node *> unvisited_neighbors;
        
        int r = current->row;
        int c = current->col;

        // Check for eligible unvisited cells located 2 indices away
        if (c > 1 && visited.find(&grid[r][c - 2]) == visited.end())
            unvisited_neighbors.push_back(&grid[r][c - 2]);
        if (c < total_cols - 2 && visited.find(&grid[r][c + 2]) == visited.end())
            unvisited_neighbors.push_back(&grid[r][c + 2]);
        if (r > 1 && visited.find(&grid[r - 2][c]) == visited.end())
            unvisited_neighbors.push_back(&grid[r - 2][c]);
        if (r < total_rows - 2 && visited.find(&grid[r + 2][c]) == visited.end())
            unvisited_neighbors.push_back(&grid[r + 2][c]);

        if (!unvisited_neighbors.empty())
        {
            // Pick a random direction
            std::uniform_int_distribution<int> dist(0, unvisited_neighbors.size() - 1);
            Node *chosen = unvisited_neighbors[dist(rng)];

            // Knock out the intermediate wall block bridging the gap
            int wall_r = (current->row + chosen->row) / 2;
            int wall_c = (current->col + chosen->col) / 2;
            grid[wall_r][wall_c].reset();

            // Open the new cell
            chosen->reset();
            visited.insert(chosen);
            stack.push(chosen);
        }
        else
        {
            // Dead end, backtrack
            stack.pop();
        }
    }
}

// ==============================================================================
// MASTER PATHFINDING ROUTINE
// A monolithic implementation supporting A*, Dijkstra, Greedy BFS, and standard BFS.
// Executed on a detached thread so the UI remains fluid and responsive.
// ==============================================================================
struct GridUpdate
{
    Node *node;
    sf::Color color;
};

bool pathfinding_algorithm(
    std::mutex &grid_mutex,
    bool &needs_redraw,
    std::vector<std::vector<Node>> &grid,
    Node *start,
    Node *end,
    Algorithm algo_type,
    bool diagonal_enabled,
    bool &path_found,
    std::atomic<ProgramState> &state)
{
    path_found = false;
    
    // Priority queue to continuously select the most promising node mathematically
    std::priority_queue<std::pair<int, Node *>, std::vector<std::pair<int, Node *>>, CompareNode> open_set;
    open_set.push({0, start});

    // Hash map to track the "bread crumb" trail for final path reconstruction
    std::unordered_map<Node *, Node *> came_from;
    
    // G-Score: Actual absolute distance traveled from the start to the current node
    std::unordered_map<Node *, int> g_score;
    // F-Score: Total expected cost (G-Score + H-Score). This is what priority queue sorts by.
    std::unordered_map<Node *, int> f_score;

    // Initialization: Assume infinite distance to all nodes initially
    for (auto &row_vec : grid)
    {
        for (auto &node : row_vec)
        {
            g_score[&node] = INT_MAX;
            f_score[&node] = INT_MAX;
        }
    }
    g_score[start] = 0;

    // Calculate initial heuristic if required
    int initial_h = 0;
    if (algo_type == Algorithm::A_STAR || algo_type == Algorithm::GREEDY_BFS)
    {
        initial_h = h(start, end, diagonal_enabled);
    }
    f_score[start] = initial_h;

    // Optimization: Track nodes currently in the queue for fast lookup
    std::unordered_set<Node *> open_set_hash;
    open_set_hash.insert(start);

    // Frame batching variables to bypass the Windows OS thread scheduler ignoring microsecond sleeps.
    // Instead of sleeping a microsecond per node, we evaluate N nodes instantly, then sleep a clean 15ms.
    int nodes_processed = 0;
    int nodes_per_frame = (algo_type == Algorithm::BFS || algo_type == Algorithm::DIJKSTRA) ? 15 : 3;

    std::vector<GridUpdate> pending_updates;
    pending_updates.reserve(100);

    // CORE EVALUATION LOOP
    while (!open_set.empty())
    {
        // Monitor for user cancelling the operation midway
        if (state.load() != ProgramState::RUNNING) return false;

        // Extract the best mathematical node
        int current_f = open_set.top().first;
        Node *current = open_set.top().second;
        open_set.pop();
        
        // BUG FIX: Skip stale duplicate queue entries where we already found a cheaper F-score earlier
        if (current_f > f_score[current]) continue;

        open_set_hash.erase(current);

        // TARGET REACHED: Path Found!
        if (current == end)
        {
            path_found = true;
            Node *temp = end;
            
            // Reconstruct the path backwards via the came_from map
            while (temp != start)
            {
                // BUG FIX: Immediate UI Deadlock Check. 
                // Aborts path drawing instantly if the user clicks "Clear".
                if (state.load() != ProgramState::RUNNING) return false; 
                
                {
                    std::lock_guard<std::mutex> lock(grid_mutex);
                    temp->make_path();
                }
                needs_redraw = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                temp = came_from[temp];
            }
            start->make_start();
            return true;
        }

        // Evaluate all physically connected neighbors
        for (Node *neighbor : current->neighbors)
        {
            int move_cost = neighbor->cost;
            
            // Correctly apply penalities for diagonal movement math
            if (diagonal_enabled && (neighbor->row != current->row && neighbor->col != current->col))
            {
                // base 10 * 1.414 yields 14.
                move_cost = static_cast<int>(move_cost * DIAGONAL_COST);
            }

            int temp_g_score = g_score[current] + move_cost;

            // --- Algorithm Differentiation ---
            // Unweighted BFS treats every step as identically distance 1
            if (algo_type == Algorithm::BFS) temp_g_score = g_score[current] + 1;
            // Greedy BFS purely ignores G-Score entirely
            if (algo_type == Algorithm::GREEDY_BFS) temp_g_score = 0;

            // If we found a structurally cheaper path to this neighbor
            if (temp_g_score < g_score[neighbor])
            {
                // Record the path
                came_from[neighbor] = current;
                g_score[neighbor] = temp_g_score;

                // Heuristic evaluation (H)
                int h_score = 0;
                if (algo_type == Algorithm::A_STAR || algo_type == Algorithm::GREEDY_BFS)
                {
                    h_score = h(neighbor, end, diagonal_enabled);
                }

                f_score[neighbor] = temp_g_score + h_score;

                // BUG FIX: Always push to priority queue to guarantee optimal path processing order
                open_set.push({f_score[neighbor], neighbor});
                
                if (open_set_hash.find(neighbor) == open_set_hash.end())
                {
                    open_set_hash.insert(neighbor);
                    if (neighbor != end)
                    {
                        pending_updates.push_back({neighbor, COLOR_OPEN});
                    }
                }
            }
        }

        if (current != start)
        {
            pending_updates.push_back({current, COLOR_CLOSED});
        }

        nodes_processed++;

        // Batch rendering: Process a chunk of nodes before pausing the thread
        if (nodes_processed >= nodes_per_frame)
        {
            std::lock_guard<std::mutex> lock(grid_mutex);
            for (auto &update : pending_updates)
            {
                update.node->color = update.color;
            }
            pending_updates.clear();
            needs_redraw = true;
            nodes_processed = 0;
            
            // Stable 15ms sleep forces Windows to actually pause, creating a smooth visual wave
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    // Process leftover graphic render queue
    if (!pending_updates.empty())
    {
        std::lock_guard<std::mutex> lock(grid_mutex);
        for (auto &update : pending_updates)
        {
            update.node->color = update.color;
        }
        needs_redraw = true;
    }

    return false;
}

// ==============================================================================
// MAIN PROGRAM ENTRY
// Handles setup, event loops, graphics initialization, and threading distribution.
// ==============================================================================
int main()
{
    // Initialize the primary window using our dynamic constants
    sf::RenderWindow window(sf::VideoMode({static_cast<unsigned int>(WINDOW_WIDTH),
                                           static_cast<unsigned int>(WINDOW_HEIGHT)}),
                            "Graph Traversal Algorithm Visualizer",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    // --- Font Loader System ---
    // Safely iterates through an array of locations to guarantee text rendering succeeds
    sf::Font font;
    std::vector<std::string> font_paths = {
        "assets/GoogleSans-Medium.ttf",
        "C:/Windows/Fonts/Arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc"
    };
    
    bool font_loaded = false;
    for (const auto& path : font_paths) {
        if (font.openFromFile(path)) {
            font_loaded = true;
            break;
        }
    }
    
    if (!font_loaded) {
        std::cerr << "Warning: Could not load any fallback fonts. Text rendering will fail." << std::endl;
    }

    // Initialize logic board state
    auto grid = make_grid(ROWS, COLS, NODE_WIDTH, NODE_WIDTH);
    Node *start = nullptr;
    Node *end = nullptr;

    // Concurrency variables
    std::atomic<ProgramState> state(ProgramState::IDLE);
    Algorithm current_algo = Algorithm::A_STAR;
    Tool current_tool = Tool::WALL;
    bool diagonal_enabled = false;
    std::thread pathfinding_thread;
    std::mutex grid_mutex;
    bool needs_redraw = true;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    // --- Render UI Background ---
    sf::RectangleShape ui_panel;
    ui_panel.setSize({static_cast<float>(GRID_WIDTH), static_cast<float>(UI_HEIGHT)});
    ui_panel.setPosition({0.f, static_cast<float>(GRID_HEIGHT)});
    ui_panel.setFillColor(COLOR_PANEL_BG);

    sf::RectangleShape separator;
    separator.setSize({static_cast<float>(GRID_WIDTH), 2.f});
    separator.setPosition({0.f, static_cast<float>(GRID_HEIGHT)});
    separator.setFillColor(sf::Color(10, 15, 20, 200));

    // --- Dynamic UI Button Layout Math ---
    // The panel aims to fit 7 buttons per row perfectly spaced across 1250 total pixels.
    const float margin = 15.f;               // Left/Right margin space
    const int buttons_per_row = 7;
    const float gap = 15.f;                  // Space between each button
    const float available_width = static_cast<float>(GRID_WIDTH) - (2.f * margin) - (gap * (buttons_per_row - 1));
    const float btn_w = available_width / buttons_per_row; // Dynamically scaling width
    const float btn_h = 45.f;
    const float btn_y1 = GRID_HEIGHT + 18.f; // Row 1 Y position
    const float btn_y2 = GRID_HEIGHT + 82.f; // Row 2 Y position

    // Helper lambda to calculate perfect X offset based on button index (0 to 6)
    auto getX = [&](int index) -> float {
        return margin + (index * (btn_w + gap));
    };

    // --- Instantiate Buttons ---
    // ROW 1: Algorithm Selector & Commands
    Button btn_astar({getX(0), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "A* (G+H)", font);
    Button btn_dijkstra({getX(1), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Dijkstra (G)", font);
    Button btn_greedy({getX(2), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Greedy (H)", font);
    Button btn_bfs({getX(3), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "BFS", font);
    btn_astar.is_active = true;

    Button btn_run({getX(4), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON_RUN, sf::Color(74, 222, 128), "RUN", font);
    Button btn_clearP({getX(5), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Clear Path", font);
    Button btn_clearA({getX(6), btn_y1}, {btn_w, btn_h}, COLOR_BUTTON_CLEAR, sf::Color(248, 113, 113), "Clear All", font);

    // ROW 2: Tools & Terrain
    Button btn_start({getX(0), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Start Node", font);
    Button btn_end({getX(1), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "End Node", font);
    Button btn_wall({getX(2), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Wall", font);
    Button btn_grass({getX(3), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Grass (30)", font);
    Button btn_mud({getX(4), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Mud (50)", font);
    btn_wall.is_active = true;

    Button btn_maze({getX(5), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Gen Maze", font);
    Button btn_diag({getX(6), btn_y2}, {btn_w, btn_h}, COLOR_BUTTON, COLOR_BUTTON_HOVER, "Diagonal", font);
    btn_diag.is_toggle = true;

    // Structuring logical arrays for radio-button behavior resets
    std::vector<Button *> algo_buttons = {&btn_astar, &btn_dijkstra, &btn_greedy, &btn_bfs};
    std::vector<Button *> tool_buttons = {&btn_start, &btn_end, &btn_wall, &btn_grass, &btn_mud};

    // ==============================================================================
    // MAIN RENDERING / EVENT LOOP
    // ==============================================================================
    while (window.isOpen())
    {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

        // --- Hardware Event Polling ---
        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                // Terminate thread safely upon application close
                if (pathfinding_thread.joinable())
                {
                    state.store(ProgramState::IDLE);
                    pathfinding_thread.join();
                }
                window.close();
            }

            if (auto *mbp = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mbp->button == sf::Mouse::Button::Left)
                {
                    if (state.load() == ProgramState::IDLE)
                    {
                        // --- UI Button Interaction Logic ---
                        if (btn_run.is_clicked(mouse_pos))
                        {
                            if (validate_pathfinding_input(start, end))
                            {
                                state.store(ProgramState::RUNNING);
                                {
                                    std::lock_guard<std::mutex> lock(grid_mutex);
                                    clear_path(grid);
                                    for (auto &row : grid)
                                        for (auto &node : row)
                                            node.update_neighbors(grid, diagonal_enabled);
                                }

                                if (pathfinding_thread.joinable())
                                    pathfinding_thread.join();

                                // Spawn the heavy computation onto a background thread
                                pathfinding_thread = std::thread([&]()
                                {
                                    bool path_found_result = false;
                                    pathfinding_algorithm(
                                        std::ref(grid_mutex), 
                                        std::ref(needs_redraw), 
                                        std::ref(grid), 
                                        start, 
                                        end, 
                                        current_algo, 
                                        diagonal_enabled,
                                        path_found_result,
                                        std::ref(state)
                                    );
                                    state.store(ProgramState::IDLE);
                                });
                            }
                        }
                        else if (btn_clearA.is_clicked(mouse_pos))
                        {
                            if (pathfinding_thread.joinable())
                            {
                                state.store(ProgramState::IDLE);
                                pathfinding_thread.join();
                            }
                            std::lock_guard<std::mutex> lock(grid_mutex);
                            clear_all(grid, start, end);
                            needs_redraw = true;
                        }
                        else if (btn_clearP.is_clicked(mouse_pos))
                        {
                            std::lock_guard<std::mutex> lock(grid_mutex);
                            clear_path(grid);
                            needs_redraw = true;
                        }
                        else if (btn_maze.is_clicked(mouse_pos))
                        {
                            if (pathfinding_thread.joinable())
                            {
                                state.store(ProgramState::IDLE);
                                pathfinding_thread.join();
                            }
                            std::lock_guard<std::mutex> lock(grid_mutex);
                            clear_all(grid, start, end);
                            generate_maze_dfs(grid, &grid[ROWS / 2][COLS / 2], rng);
                            needs_redraw = true;
                        }
                        else if (btn_diag.is_clicked(mouse_pos))
                        {
                            btn_diag.toggle();
                            diagonal_enabled = btn_diag.is_active;
                        }
                        
                        // Handle Algorithm Radio Switches
                        else if (btn_astar.is_clicked(mouse_pos))
                        {
                            current_algo = Algorithm::A_STAR;
                            for (auto b : algo_buttons) b->is_active = false;
                            btn_astar.is_active = true;
                        }
                        else if (btn_dijkstra.is_clicked(mouse_pos))
                        {
                            current_algo = Algorithm::DIJKSTRA;
                            for (auto b : algo_buttons) b->is_active = false;
                            btn_dijkstra.is_active = true;
                        }
                        else if (btn_greedy.is_clicked(mouse_pos))
                        {
                            current_algo = Algorithm::GREEDY_BFS;
                            for (auto b : algo_buttons) b->is_active = false;
                            btn_greedy.is_active = true;
                        }
                        else if (btn_bfs.is_clicked(mouse_pos))
                        {
                            current_algo = Algorithm::BFS;
                            for (auto b : algo_buttons) b->is_active = false;
                            btn_bfs.is_active = true;
                        }
                        
                        // Handle Drawing Tool Radio Switches
                        else if (btn_start.is_clicked(mouse_pos))
                        {
                            current_tool = Tool::START;
                            for (auto b : tool_buttons) b->is_active = false;
                            btn_start.is_active = true;
                        }
                        else if (btn_end.is_clicked(mouse_pos))
                        {
                            current_tool = Tool::END;
                            for (auto b : tool_buttons) b->is_active = false;
                            btn_end.is_active = true;
                        }
                        else if (btn_wall.is_clicked(mouse_pos))
                        {
                            current_tool = Tool::WALL;
                            for (auto b : tool_buttons) b->is_active = false;
                            btn_wall.is_active = true;
                        }
                        else if (btn_grass.is_clicked(mouse_pos))
                        {
                            current_tool = Tool::GRASS;
                            for (auto b : tool_buttons) b->is_active = false;
                            btn_grass.is_active = true;
                        }
                        else if (btn_mud.is_clicked(mouse_pos))
                        {
                            current_tool = Tool::MUD;
                            for (auto b : tool_buttons) b->is_active = false;
                            btn_mud.is_active = true;
                        }
                        // --- Grid Tile Mouse Click Processing ---
                        else
                        {
                            std::lock_guard<std::mutex> lock(grid_mutex);
                            Node *node = get_clicked_node(mouse_pos, grid);
                            if (node)
                            {
                                switch (current_tool)
                                {
                                case Tool::START:
                                    if (node != end)
                                    {
                                        if (start) start->reset();
                                        start = node;
                                        start->make_start();
                                    }
                                    break;
                                case Tool::END:
                                    if (node != start)
                                    {
                                        if (end) end->reset();
                                        end = node;
                                        end->make_end();
                                    }
                                    break;
                                case Tool::WALL:
                                    if (node != start && node != end) node->make_wall();
                                    break;
                                case Tool::GRASS:
                                    if (node != start && node != end) node->make_grass();
                                    break;
                                case Tool::MUD:
                                    if (node != start && node != end) node->make_mud();
                                    break;
                                }
                                needs_redraw = true;
                            }
                        }
                    }
                }
                // Mouse Right Click (Quick Erase Tool)
                else if (mbp->button == sf::Mouse::Button::Right)
                {
                    if (state.load() == ProgramState::IDLE)
                    {
                        std::lock_guard<std::mutex> lock(grid_mutex);
                        Node *node = get_clicked_node(mouse_pos, grid);
                        if (node)
                        {
                            node->reset();
                            if (node == start) start = nullptr;
                            if (node == end) end = nullptr;
                            needs_redraw = true;
                        }
                    }
                }
            }
        }

        // --- Continuous Mouse Dragging Support ---
        
        // Draw drag tracking
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && state.load() == ProgramState::IDLE)
        {
            if (mouse_pos.y < GRID_HEIGHT) // Prevent drawing over the UI
            {
                std::lock_guard<std::mutex> lock(grid_mutex);
                Node *node = get_clicked_node(mouse_pos, grid);
                if (node && node != start && node != end)
                {
                    if (current_tool == Tool::WALL) node->make_wall();
                    else if (current_tool == Tool::GRASS) node->make_grass();
                    else if (current_tool == Tool::MUD) node->make_mud();
                    needs_redraw = true;
                }
            }
        }

        // Erase drag tracking
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) && state.load() == ProgramState::IDLE)
        {
            std::lock_guard<std::mutex> lock(grid_mutex);
            Node *node = get_clicked_node(mouse_pos, grid);
            if (node)
            {
                node->reset();
                if (node == start) start = nullptr;
                if (node == end) end = nullptr;
                needs_redraw = true;
            }
        }

        // --- UI Frame Processing ---
        if (state.load() == ProgramState::IDLE)
        {
            // Process hover animations and color lerping
            btn_run.update(mouse_pos);
            btn_clearA.update(mouse_pos);
            btn_clearP.update(mouse_pos);
            btn_astar.update(mouse_pos);
            btn_dijkstra.update(mouse_pos);
            btn_greedy.update(mouse_pos);
            btn_bfs.update(mouse_pos);
            btn_start.update(mouse_pos);
            btn_end.update(mouse_pos);
            btn_wall.update(mouse_pos);
            btn_grass.update(mouse_pos);
            btn_mud.update(mouse_pos);
            btn_maze.update(mouse_pos);
            btn_diag.update(mouse_pos);

            // Optimization: Re-render only if mouse moved or clicked to save CPU/GPU cycles
            static sf::Vector2i last_mouse_pos = mouse_pos;
            if (mouse_pos != last_mouse_pos)
            {
                needs_redraw = true;
                last_mouse_pos = mouse_pos;
            }
        }

        // --- Display Buffer Rendering ---
        if (needs_redraw || state.load() == ProgramState::RUNNING)
        {
            needs_redraw = false;
            window.clear(COLOR_BG);

            // Paint the dynamic node grid
            {
                std::lock_guard<std::mutex> lock(grid_mutex);
                for (const auto &row_vec : grid)
                {
                    for (const auto &node : row_vec)
                    {
                        node.draw(window, font);
                    }
                }
                draw_grid_lines(window, ROWS, COLS, NODE_WIDTH, NODE_WIDTH);
            }

            // Paint UI Elements
            window.draw(ui_panel);
            window.draw(separator);
            
            // Draw all buttons
            btn_run.draw(window);
            btn_clearA.draw(window);
            btn_clearP.draw(window);
            btn_astar.draw(window);
            btn_dijkstra.draw(window);
            btn_greedy.draw(window);
            btn_bfs.draw(window);
            btn_start.draw(window);
            btn_end.draw(window);
            btn_wall.draw(window);
            btn_grass.draw(window);
            btn_mud.draw(window);
            btn_maze.draw(window);
            btn_diag.draw(window);

            window.display();
        }
    }

    // --- Process Safe Shutdown ---
    if (pathfinding_thread.joinable())
    {
        state.store(ProgramState::IDLE);
        pathfinding_thread.join();
    }

    return 0;
}