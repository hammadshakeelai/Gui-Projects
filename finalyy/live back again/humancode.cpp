// Hey there! I've had a go at "humanizing" this Flappy Bird clone code.
// I've added a few extra comments, some variable name tweaks, and occasional inconsistent formatting similar to what a human might produce in a hurry.
// Enjoy the slightly less pristine but more "natural" code :-)

#include <windows.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <fstream>
#include <memory> // For smart pointers, though I might not use them everywhere perfectly

// Game Constants (I often like these all-caps because they are constants)
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PILLAR_GAP = 200;
const int PILLAR_WIDTH = 80;
const int BIRD_SIZE = 30;
const int GROUND_HEIGHT = 50;

// Bird structure - represents the little hero flying around
struct Bird {
int posX;
int posY;
float currentVelocity;
// A couple of constant forces - gravity pulling it down and a jump "boost"
const float gravityForce = 0.4f;
const float jumpBoost = -8.0f;

arduino

Copy
// Default constructor
Bird() : posX(100), posY(SCREEN_HEIGHT/2), currentVelocity(0) {}

// Copy constructor (I know, it's a bit redundant, but for clarity)
Bird(const Bird& other) : 
    posX(other.posX), 
    posY(other.posY), 
    currentVelocity(other.currentVelocity),
    gravityForce(0.4f),  // reinitialize constants explicitly (even if repetitive)
    jumpBoost(-8.0f) { }

// Assignment operator (Remember to check for self-assignment)
Bird& operator=(const Bird& other) {
    if (this != &other) {
        posX = other.posX;
        posY = other.posY;
        currentVelocity = other.currentVelocity;
        // Not assigning const values as they are fixed
    }
    return *this;
}

// Update the bird's physics. 
// Note: This is a very simple physics integration, nothing fancy here.
void update() {
    currentVelocity += gravityForce;
    posY += static_cast<int>(currentVelocity);
}

// When the bird jumps, simply reset velocity to a negative value.
void jump() {
    currentVelocity = jumpBoost;
}

// Returns the bird's rectangle for collision detection
RECT getBoundingRect() {
    RECT r;
    // A bit of math to center the rectangle. Could be made prettier...
    r.left = posX - BIRD_SIZE/2;
    r.top = posY - BIRD_SIZE/2;
    r.right = posX + BIRD_SIZE/2;
    r.bottom = posY + BIRD_SIZE/2;
    return r;
}
};

// Pillar structure represents each obstacle
struct Pillar {
int posX;
int gapCenterY;
bool hasBeenPassed;

ini

Copy
// Constructor. I like using the member initializer list.
Pillar(int startX, int gapY) : posX(startX), gapCenterY(gapY), hasBeenPassed(false) {}

// Update pillar position by moving left
void update() {
    posX -= 3;  // There could be a constant here for speed but meh, it's fine.
}

// Check if the pillar has moved completely off the screen
bool offScreen() const {
    return posX < -PILLAR_WIDTH;
}

// Returns the top rectangle for collision check. 
RECT getTopRect() const {
    RECT topRect;
    topRect.left = posX;
    topRect.top = 0;
    topRect.right = posX + PILLAR_WIDTH;
    // Using half-gap to position it correctly 
    topRect.bottom = gapCenterY - PILLAR_GAP/2;
    return topRect;
}

// Returns the bottom rectangle for collision check.
RECT getBottomRect() const {
    RECT bottomRect;
    bottomRect.left = posX;
    bottomRect.top = gapCenterY + PILLAR_GAP/2;
    bottomRect.right = posX + PILLAR_WIDTH;
    bottomRect.bottom = SCREEN_HEIGHT - GROUND_HEIGHT;
    return bottomRect;
}
};

// Global Game Variables
// I know globals are not the best, but for a simple game like this it works fine.
static HWND g_windowHandle = nullptr;
static HDC g_mainDC = nullptr;
static bool g_isGameRunning = true;
static int g_currentScore = 0;
static int g_bestScore = 0;
static Bird g_flappyBird; // our main bird!
static std::vector<Pillar> g_obstaclePillars;
static int g_pillarSpawnTimer = 0;
static const int SPAWN_INTERVAL = 120; // frames until a new pillar appears

// GDI resource manager class to manage drawing resources
class GDIResourceManager {
public:
GDIResourceManager() = default;
~GDIResourceManager() {
// Cleaning up resources. A human might sometimes forget one!
if (skyBrush) DeleteObject(skyBrush);
if (groundBrush) DeleteObject(groundBrush);
if (pillarBrush) DeleteObject(pillarBrush);
if (birdBrush) DeleteObject(birdBrush);
if (scoreFont) DeleteObject(scoreFont);
}

arduino

Copy
// Initialize all our brushes and fonts. Very Windows-GDI style!
void setupResources(HDC hdc) {
    skyBrush = CreateSolidBrush(RGB(135, 206, 235));  // light blue for the sky
    groundBrush = CreateSolidBrush(RGB(139, 69, 19));   // brown-ish ground
    pillarBrush = CreateSolidBrush(RGB(0, 128, 0));      // green pillars
    birdBrush = CreateSolidBrush(RGB(255, 215, 0));      // bright yellow bird
    scoreFont = CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                           FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           VARIABLE_PITCH, "Arial");
}

HBRUSH getSkyBrush() const { return static_cast<HBRUSH>(skyBrush); }
HBRUSH getGroundBrush() const { return static_cast<HBRUSH>(groundBrush); }
HBRUSH getPillarBrush() const { return static_cast<HBRUSH>(pillarBrush); }
HBRUSH getBirdBrush() const { return static_cast<HBRUSH>(birdBrush); }
HGDIOBJ getScoreFont() const { return scoreFont; }
private:
HGDIOBJ skyBrush = nullptr;
HGDIOBJ groundBrush = nullptr;
HGDIOBJ pillarBrush = nullptr;
HGDIOBJ birdBrush = nullptr;
HGDIOBJ scoreFont = nullptr;
};

// initialize or reset the game state (I usually call this "initializeGame" in my projects)
void initializeGame() {
// Reset our bird to starting position
g_flappyBird = Bird();

arduino

Copy
// Clear out the old pillars if any exist
g_obstaclePillars.clear();

// Reset score and timer
g_pillarSpawnTimer = 0;
g_currentScore = 0;

// Load previous best score from a file - simple persistence
std::ifstream scoreFile("highscore.txt");
if (scoreFile.is_open()) {
    scoreFile >> g_bestScore;
    scoreFile.close();
} else {
    // Sometimes I forget this else, but it's okay if file not found.
    g_bestScore = 0;
}
}

// Function to spawn a new pillar. I sometimes add a TODO here to improve randomness.
void createPillar() {
// Suboptimal randomization... but it gets the job done :)
int gapCenter = 150 + (rand() % (SCREEN_HEIGHT - GROUND_HEIGHT - 300));
g_obstaclePillars.push_back(Pillar(SCREEN_WIDTH, gapCenter));
}

// Update game state. I like to keep update logic separate from drawing.
void updateGame() {
g_flappyBird.update();

scss

Copy
// Increase timer for pillar spawn; a bit fuzzy if it's frame-based.
g_pillarSpawnTimer++;
if (g_pillarSpawnTimer >= SPAWN_INTERVAL) {
    createPillar();
    g_pillarSpawnTimer = 0; // reset timer
}

// Update each pillar and check if the bird has passed them.
for (auto& obs : g_obstaclePillars) {
    obs.update();
    
    if (!obs.hasBeenPassed && (obs.posX + PILLAR_WIDTH < g_flappyBird.posX)) {
        obs.hasBeenPassed = true;
        g_currentScore++;
        // update best score if necessary
        if (g_currentScore > g_bestScore) {
            g_bestScore = g_currentScore;
            // Write new high score to file
            std::ofstream fileOut("highscore.txt");
            if (fileOut.is_open()) {
                fileOut << g_bestScore;
                fileOut.close();
            }
        }
    }
}

// Remove any pillars which have exited the screen completely.
if (!g_obstaclePillars.empty() && g_obstaclePillars.front().offScreen()) {
    // Sometimes I loop through all so that I don't rely on the first element only...
    g_obstaclePillars.erase(g_obstaclePillars.begin());
}

// Check collisions between the bird and the ground or ceiling.
RECT birdRect = g_flappyBird.getBoundingRect();

// Check ground collision (bird hitting ground resets game)
if (birdRect.bottom > SCREEN_HEIGHT - GROUND_HEIGHT) {
    // I usually add a little sound here if I had the time, but for now just reset.
    initializeGame();
    return;
}

// Check top (ceiling) collision
if (birdRect.top < 0) {
    initializeGame();
    return;
}

// Check collision with each pillar's rectangles
for (const auto& obs : g_obstaclePillars) {
    RECT topRect = obs.getTopRect();
    RECT bottomRect = obs.getBottomRect();
    
    // Simple collision detection: Check if bird is overlapping horizontally
    if (birdRect.right > topRect.left && birdRect.left < topRect.right) {
        // Then check vertical overlap for top or bottom pillars.
        if (birdRect.top < topRect.bottom || birdRect.bottom > bottomRect.top) {
            // Collision detected, so restart.
            initializeGame();
            return;
        }
    }
}
}

// Draw everything on screen. I tend to group GDI calls like this when I'm in the mood.
void drawGame(HDC hdc) {
// Create resource manager and initialize it.
GDIResourceManager gfxManager;
gfxManager.setupResources(hdc);

reasonml

Copy
// First, clear the screen with the sky color.
RECT fullScreen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
FillRect(hdc, &fullScreen, gfxManager.getSkyBrush());

// Draw the ground in a separate rectangle.
RECT groundArea = {0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
FillRect(hdc, &groundArea, gfxManager.getGroundBrush());

// Draw each pillar (top and bottom parts)
for (const auto& obs: g_obstaclePillars) {
    RECT topPart = obs.getTopRect();
    RECT bottomPart = obs.getBottomRect();
    FillRect(hdc, &topPart, gfxManager.getPillarBrush());
    FillRect(hdc, &bottomPart, gfxManager.getPillarBrush());
}

// Draw our bird using its rectangle.
RECT birdRect = g_flappyBird.getBoundingRect();
FillRect(hdc, &birdRect, gfxManager.getBirdBrush());

// Draw the scores on the top-left.
SelectObject(hdc, gfxManager.getScoreFont());
std::string scoreStr = "Score: " + std::to_string(g_currentScore);
TextOut(hdc, 10, 10, scoreStr.c_str(), scoreStr.length());

std::string bestScoreStr = "High Score: " + std::to_string(g_bestScore);
TextOut(hdc, 10, 50, bestScoreStr.c_str(), bestScoreStr.length());

// If I had more time, I might add some animations here...
}

// Window procedure to handle messages like key presses and clicks.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
switch (message) {
case WM_DESTROY:
g_isGameRunning = false;
PostQuitMessage(0);
return 0;

hsp

Copy
    case WM_KEYDOWN:
        // Check if the space bar was pressed for a jump.
        if (wParam == VK_SPACE) {
            g_flappyBird.jump();
        }
        return 0;
        
    case WM_LBUTTONDOWN:
        // On mouse click, make the bird jump too!
        g_flappyBird.jump();
        return 0;
        
    default:
        break;
}
return DefWindowProc(hwnd, message, wParam, lParam);
}

// Entry point for the application
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow) {
// Register window class -- pretty standard stuff.
WNDCLASS windowClass = {};
windowClass.lpfnWndProc = WindowProc;
windowClass.hInstance = hInst;
windowClass.lpszClassName = "FlappyBirdWindowClass";
windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
RegisterClass(&windowClass);

reasonml

Copy
// Create the window (notice how I disabled resizing, typical for games)
g_windowHandle = CreateWindowEx(
    0, 
    "FlappyBirdWindowClass", 
    "Flappy Bird", 
    WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, 
    CW_USEDEFAULT, CW_USEDEFAULT, 
    SCREEN_WIDTH, SCREEN_HEIGHT, 
    NULL, 
    NULL, 
    hInst, 
    NULL);
    
ShowWindow(g_windowHandle, cmdShow);

// Initialize our game state and seed the random generator.
initializeGame();
srand(static_cast<unsigned int>(time(NULL)));

// Main game loop. Typical Windows message pump with some game updates.
MSG msg = {};
while (g_isGameRunning) {
    // Process all pending messages first.
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    updateGame();  // update game logic
    
    // Draw everything onto the window.
    HDC currentDC = GetDC(g_windowHandle);
    drawGame(currentDC);
    ReleaseDC(g_windowHandle, currentDC);
    
    // Pause a bit to cap frame rate. (I know this sleep isn't the best way!)
    Sleep(16);  // roughly 60 frames per second.
}

return 0;
}