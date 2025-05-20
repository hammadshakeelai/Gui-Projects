// ─────────────────────────────────────────────────────────────────────────────
// 1) Headers and Libraries
// ─────────────────────────────────────────────────────────────────────────────
#include <windows.h>    // Win32 API core (window creation, messages, GDI)
#include <vector>       // std::vector for dynamic arrays of pillars
#include <cstdlib>      // std::rand, std::srand
#include <ctime>        // std::time for RNG seeding
#include <string>       // std::string for text handling
#include <fstream>      // std::ifstream, std::ofstream for high score persistence

// ─────────────────────────────────────────────────────────────────────────────
// 2) Global Configuration Constants
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int windowWidth = 1520;         // width of game window (pixels)
static constexpr int windowHeight = 820;         // height of game window (pixels)
static constexpr int spawnInterval = 60;         // frames between pillar spawns
static constexpr int timerInterval = 12;         // ms per frame (~1000/12 ≈ 83 FPS)
static constexpr int messageDisplayTime = 2000;  // ms to show “You Lost” message

// Win32 window class & title names
static constexpr LPCSTR CLASS_NAME   = "FlappyClass"; // window class identifier
static constexpr LPCSTR WINDOW_TITLE = "Flappy Bird"; // window title bar text

// ─────────────────────────────────────────────────────────────────────────────
// 3) Bird Class: represents the player’s bird
// ─────────────────────────────────────────────────────────────────────────────
class Bird {
public:
    // Constructor: set initial x, y, velocity
    Bird(int x, int y) : x_(x), y_(y), velocity_(0.0f) {}

    // update position each frame: apply gravity, then move
    void update() {
        velocity_ += gravity_;          // increase downward speed
        y_        += static_cast<int>(velocity_); // move bird by velocity
    }

    // on flap input: give the bird an upward boost
    void flap() {
        velocity_ = -10.0f;             // instant upward velocity
    }

    // draw the bird as a filled circle
    void draw(HDC hdc) const {
        HBRUSH brush    = CreateSolidBrush(RGB(255, 215, 0)); // gold fill
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        // draw circle centered at (x_, y_) with diameter size_
        Ellipse(hdc,
                x_ - size_/2, y_ - size_/2,
                x_ + size_/2, y_ + size_/2);
        SelectObject(hdc, oldBrush);    // restore old brush
        DeleteObject(brush);            // free our brush
    }

    // get the bird’s bounding box for collision checks
    RECT getBounds() const {
        return { x_ - size_/2, y_ - size_/2,
                 x_ + size_/2, y_ + size_/2 };
    }

private:
    int   x_, y_;                      // position
    float velocity_;                   // vertical speed
    static constexpr int   size_    = 30;   // pixel diameter
    static constexpr float gravity_ = 0.4f; // downward acceleration
};

// ─────────────────────────────────────────────────────────────────────────────
// 4) Pillar Class: represents obstacles the bird must avoid
// ─────────────────────────────────────────────────────────────────────────────
class Pillar {
public:
    // Constructor: start off-screen at x, with vertical gap centered at gapY
    Pillar(int x, int gapY) : x_(x), gapY_(gapY) {}

    // move pillar left each frame
    void update() {
        x_ -= speed_;                   // slide left by speed_
    }

    // draw top and bottom rectangles with a gap in between
    void draw(HDC hdc) const {
        HBRUSH brush    = CreateSolidBrush(RGB(0, 128, 0)); // green fill
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

        // top pillar
        RECT top = getTopRect();
        Rectangle(hdc, top.left, top.top, top.right, top.bottom);

        // bottom pillar
        RECT bottom = getBottomRect();
        Rectangle(hdc, bottom.left, bottom.top,
                  bottom.right, bottom.bottom);

        SelectObject(hdc, oldBrush);    // restore old brush
        DeleteObject(brush);            // free our brush
    }

    // check if the pillar has completely moved off the left edge
    bool isOffScreen() const {
        return x_ + width_ < 0;
    }

    // rectangle covering the top obstacle
    RECT getTopRect() const {
        return { x_, 0, x_ + width_, gapY_ - gap_/2 };
    }

    // rectangle covering the bottom obstacle
    RECT getBottomRect() const {
        return { x_, gapY_ + gap_/2,
                 x_ + width_, windowHeight };
    }

private:
    int   x_, gapY_;                     // horizontal pos, center of gap
    static constexpr int width_ = 50;    // pillar thickness
    static constexpr int gap_   = 150;   // vertical gap size
    static constexpr int speed_ = 6;     // horizontal speed per frame
};

// ─────────────────────────────────────────────────────────────────────────────
// 5) Game Class: holds all game state & logic
// ─────────────────────────────────────────────────────────────────────────────
class Game {
public:
    // constructor: init bird, seed RNG, load high score
    Game()
      : bird_(100, 300),
        score_(0),
        highestPillarsCrossed_(0),
        gameOver_(false),
        frameCount_(0),
        pillarsCrossed_(0),
        showLostMessage_(false),
        showHighScoreMessage_(false)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));// random seed
        loadHighestPillarsCrossed();    // read high score from file
    }

    // start or restart the game: clear pillars, reset counters
    void init() {
        pillars_.clear();               // remove all existing pillars
        bird_ = Bird(100, 300);         // reset bird position & velocity
        score_ = 0;                     // reset current score
        gameOver_ = false;              // resume gameplay
        frameCount_ = 0;                // reset spawn timer
        pillarsCrossed_ = 0;            // reset crossed pillars
        showLostMessage_ = false;       // hide “lost” text
        showHighScoreMessage_ = false;  // hide “new high score” text
    }

    // called each timer tick: update game state
    void update() {
        // if game over, only handle “lost” message timer
        if (gameOver_) {
            if (showLostMessage_) {
                lostMessageTimer_ += timerInterval;
                if (lostMessageTimer_ >= messageDisplayTime)
                    showLostMessage_ = false; // hide after delay
            }
            return;                       // skip rest
        }

        bird_.update();                  // move the bird

        // spawn new pillar every spawnInterval frames
        if (++frameCount_ >= spawnInterval) {
            int gapY = 150 + std::rand() % (windowHeight - 300);
            pillars_.emplace_back(windowWidth, gapY);
            frameCount_ = 0;
        }

        // move pillars & remove off-screen ones
        for (auto &p : pillars_) p.update();
        if (!pillars_.empty() && pillars_.front().isOffScreen()) {
            pillars_.erase(pillars_.begin());
            pillarsCrossed_++;            // increment score
            // check for new high score
            if (pillarsCrossed_ > highestPillarsCrossed_) {
                highestPillarsCrossed_ = pillarsCrossed_;
                showHighScoreMessage_   = true;
            }
        }

        // collision: bird hits top/bottom of window?
        RECT b = bird_.getBounds();
        if (b.top < 0 || b.bottom > windowHeight) {
            gameOver_ = true;
        }
        // collision: bird touches any pillar?
        for (auto &p : pillars_) {
            RECT topR    = p.getTopRect();
            RECT bottomR = p.getBottomRect();
            if (IntersectRect(&b, &topR, &b) ||
                IntersectRect(&b, &bottomR, &b))
            {
                gameOver_        = true;
                showLostMessage_ = true;
                break;
            }
        }

        // if just died, save new high score
        if (gameOver_) saveHighestPillarsCrossed();
    }

    // draw all game visuals
    void draw(HDC hdc) {
        bird_.draw(hdc);                // redraw the bird
        for (auto &p : pillars_) p.draw(hdc); // draw obstacles

        // draw current score (pillars crossed)
        SetTextColor(hdc, RGB(0, 0, 0));
        std::string s = "Score: " + std::to_string(pillarsCrossed_);
        TextOut(hdc, 10, 10, s.c_str(), (int)s.size());

        // draw high score
        std::string h = "Highest: " + std::to_string(highestPillarsCrossed_);
        TextOut(hdc, 10, 30, h.c_str(), (int)h.size());

        // optionally draw “You Lost!” in red
        if (showLostMessage_) {
            SetTextColor(hdc, RGB(255, 0, 0));
            std::string lost = "You Lost!";
            TextOut(hdc,
                    windowWidth/2 - 50,
                    windowHeight/2,
                    lost.c_str(),
                    (int)lost.size());
        }

        // optionally draw “New High Score!” in green
        if (showHighScoreMessage_) {
            SetTextColor(hdc, RGB(0, 255, 0));
            std::string highMsg = "You Beat Your High Score!";
            TextOut(hdc,
                    windowWidth/2 - 100,
                    windowHeight/2 + 30,
                    highMsg.c_str(),
                    (int)highMsg.size());
        }
    }

    // on mouse click: if game over restart, else make bird flap
    void flap() {
        if (gameOver_) init();
        bird_.flap();
    }

    bool isGameOver() const { return gameOver_; }

private:
    Bird               bird_;               // player object
    std::vector<Pillar> pillars_;            // active obstacles
    int                score_;              // unused (we use pillarsCrossed_)
    int                highestPillarsCrossed_; // persisted best score
    bool               gameOver_;           // is the game currently over?
    int                frameCount_;         // frame counter for spawns
    int                pillarsCrossed_;     // current session score
    bool               showLostMessage_;    // toggles red “lost” text
    bool               showHighScoreMessage_; // toggles green “new high score”
    int                lostMessageTimer_ = 0; // ms since game over

    // load high score from disk
    void loadHighestPillarsCrossed() {
        std::ifstream file("highest_pillars_crossed.txt");
        if (file.is_open()) {
            file >> highestPillarsCrossed_;
            file.close();
        } else {
            highestPillarsCrossed_ = 0; // default if no file yet
        }
    }

    // save high score to disk
    void saveHighestPillarsCrossed() {
        std::ofstream file("highest_pillars_crossed.txt");
        if (file.is_open()) {
            file << highestPillarsCrossed_;
            file.close();
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 6) Global Game Instance
// ─────────────────────────────────────────────────────────────────────────────
static Game game;  // single shared Game object across callbacks

// ─────────────────────────────────────────────────────────────────────────────
// 7) Win32 Window Procedure: handles all window messages
// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        // window just created: start our timer
        SetTimer(hwnd, 1, timerInterval, nullptr);
        break;
    case WM_TIMER:
        // on each timer tick: update logic & force redraw
        game.update();
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    case WM_LBUTTONDOWN:
        // left mouse click anywhere = flap or restart
        game.flap();
        break;
    case WM_PAINT: {
        // request to repaint window: draw game frame
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        game.draw(hdc);
        EndPaint(hwnd, &ps);
    } break;
    case WM_DESTROY:
        // window closing: kill timer & quit message loop
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
    default:
        // all other messages: default handling
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0; // we handled the message
}

// ─────────────────────────────────────────────────────────────────────────────
// 8) WinMain: program entry point for GUI apps
// ─────────────────────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    // define window class properties
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;              // our message handler
    wc.hInstance     = hInst;                // current app instance
    wc.lpszClassName = CLASS_NAME;           // must match CreateWindowEx
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // default background

    RegisterClass(&wc);                      // register with OS

    // create the window (no maximize or resize)
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,                          // registered class
        WINDOW_TITLE,                        // window title
        WS_OVERLAPPEDWINDOW
          & ~WS_MAXIMIZEBOX
          & ~WS_THICKFRAME,                  // no maximize or resize
        CW_USEDEFAULT, CW_USEDEFAULT,        // default position
        windowWidth + 16, windowHeight + 39, // account for borders
        nullptr, nullptr, hInst, nullptr     // no menus or user data
    );
    ShowWindow(hwnd, nShow);                // show the window

    // main message loop: dispatch until WM_QUIT
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);              // translate key strokes
        DispatchMessage(&msg);               // call WndProc
    }
    return 0;                                // exit code
}
