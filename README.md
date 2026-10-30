# Hangman Game

A beginner-friendly Hangman game with a **C++ backend** and an **HTML / CSS / JavaScript frontend**.

The C++ program runs a small web server on your computer. The browser loads the game page from that server, and the frontend sends guesses to the C++ backend.

**Java is not needed for this project.** JavaScript and Java are different languages.

## Features

- Random word from a small built-in word list
- Six wrong guesses allowed
- On-screen A-Z keyboard
- Physical keyboard support
- Hangman drawing that updates after each wrong guess
- New Game button
- No external libraries or packages

## Standalone browser version

Open `index.html` directly in any browser to play the single-file version.
All HTML, CSS, and JavaScript are inside that one file, so no server or C++
compilation is needed.

## Project structure

```text
hangman-game/
|-- backend/
|   `-- main.cpp          C++ game logic and web server
|-- frontend/
|   |-- index.html        Page structure
|   |-- style.css         Page design
|   `-- script.js         Browser logic and API calls
|-- index.html            Standalone single-file browser version
|-- build.bat             Compiles the C++ backend
|-- run.bat               Starts the game server
`-- README.md
```

## Requirements

- Windows
- A C++ compiler with `g++` available in the terminal (MinGW-w64 is recommended)
- A web browser

Java is not required.

## How to build and run

### Option 1: Double-click

1. Double-click `build.bat`.
2. Double-click `run.bat`.
3. Open <http://localhost:8081> in your browser.

Keep the server window open while playing.

### Option 2: Command prompt

Open a terminal in the `hangman-game` folder and run:

```bat
build.bat
run.bat
```

Or compile and run manually:

```bat
g++ backend\main.cpp -std=c++17 -Wall -Wextra -o hangman.exe -lws2_32
hangman.exe
```

Then open <http://localhost:8081>.

> Important: run `hangman.exe` from the `hangman-game` folder. The server reads the frontend files from the `frontend` folder.

## How to play

1. A hidden word is chosen automatically.
2. Click a letter or press a letter on your keyboard.
3. A correct letter is shown in the word.
4. Each wrong letter adds one part to the hangman drawing.
5. You win by finding every letter before making six wrong guesses.
6. Click **New Game** to start again.

## How it works

### Backend (`backend/main.cpp`)

The C++ backend:

1. Opens a Windows socket on `127.0.0.1:8081`.
2. Serves the HTML, CSS, and JavaScript files.
3. Keeps one Hangman game in memory.
4. Returns the updated game state as JSON.

The game state contains the masked word, wrong guess count, guessed letters, game status, and a message.

### Frontend (`frontend/`)

The frontend:

1. Loads the page and asks the backend for a new game.
2. Builds the A-Z keyboard with JavaScript.
3. Sends a guess to the backend when a letter is clicked or typed.
4. Updates the word, lives, message, keyboard colors, and hangman drawing.

## API endpoints

| Method | Address | Purpose |
| --- | --- | --- |
| `GET` | `/` | Loads the game page |
| `GET` | `/style.css` | Loads the stylesheet |
| `GET` | `/script.js` | Loads the browser script |
| `GET` | `/api/state` | Returns the current game state |
| `POST` | `/api/new` | Starts a new game |
| `POST` | `/api/guess` | Guesses a letter |

The guess request uses form data:

```text
letter=a
```

## Easy changes for beginners

### Add or change words

Open `backend/main.cpp` and edit the `WORDS` list:

```cpp
const std::vector<std::string> WORDS = {
    "apple", "banana", "orange", "tiger", "house"
};
```

Words must use lowercase letters only.

### Change the number of lives

Open `backend/main.cpp` and change:

```cpp
const int MAX_WRONG = 6;
```

The drawing shows six body parts, so keeping this at 6 is recommended.

### Change the port

The default port is `8081`. You can start the server on another port without
editing the code:

```bat
run.bat 9090
```

Or run the program directly:

```bat
hangman.exe 9090
```

Then open `http://localhost:9090`.

To change the default port, open `backend/main.cpp` and change:

```cpp
const int DEFAULT_PORT = 8081;
```

## Troubleshooting

### `g++` is not recognized

MinGW-w64 is not installed, or its `bin` folder is not in PATH. Install MinGW-w64, then add its `bin` folder to the Windows PATH. Close and reopen the terminal before trying again.

### Port 8081 is already in use

Another program is using that port. Start the server on a different port with
`hangman.exe 8090` (or another free port), or change `DEFAULT_PORT` in
`backend/main.cpp` and rebuild.

### The page says it cannot reach the server

Make sure the server window is still open and that you opened <http://localhost:8081>. Do not open `frontend/index.html` directly from File Explorer, because the backend API will not be available. For a version that works without the server, open the root `index.html` instead.

### The game page loads but styles or letters are missing

Run the server from the `hangman-game` folder. The server serves the frontend files using relative paths.

## Notes

- This is a learning project, not a production web server.
- All browser tabs share the same game state.
- The C++ server only listens on your local computer (`127.0.0.1`).
- The project uses Windows Sockets, so the backend is designed for Windows.
