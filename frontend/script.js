const wordElement = document.getElementById("word");
const keyboardElement = document.getElementById("keyboard");
const messageElement = document.getElementById("message");
const livesElement = document.getElementById("lives");
const wrongElement = document.getElementById("wrong");
const newGameButton = document.getElementById("new-game");

const bodyParts = ["head", "body", "left-arm", "right-arm", "left-leg", "right-leg"];
let game = null;

function buildKeyboard() {
    const letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("");

    letters.forEach(function (letter) {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "key";
        button.textContent = letter;
        button.dataset.letter = letter.toLowerCase();
        button.addEventListener("click", function () {
            makeGuess(button.dataset.letter);
        });
        keyboardElement.appendChild(button);
    });
}

async function requestJson(url, options) {
    const response = await fetch(url, options);
    const data = await response.json();

    if (!response.ok) {
        throw new Error(data.message || "The server returned an error.");
    }

    return data;
}

function renderGame(data) {
    game = data;

    livesElement.textContent = Math.max(data.maxWrong - data.wrong, 0);
    wrongElement.textContent = data.wrong;

    messageElement.className = "message";
    messageElement.textContent = data.message;

    if (data.won) {
        messageElement.classList.add("win");
    }

    if (data.lost) {
        messageElement.classList.add("lose");
    }

    if (data.answer) {
        messageElement.textContent += " The word was " + data.answer.toUpperCase() + ".";
    }

    wordElement.innerHTML = "";

    data.masked.split("").forEach(function (letter) {
        const box = document.createElement("span");
        box.className = "letter-box";
        box.textContent = letter === "_" ? "" : letter.toUpperCase();

        if (letter === "_") {
            box.classList.add("empty");
        }

        wordElement.appendChild(box);
    });

    bodyParts.forEach(function (partId, index) {
        document.getElementById(partId).classList.toggle("hidden", index >= data.wrong);
    });

    document.querySelectorAll(".key").forEach(function (button) {
        const letter = button.dataset.letter;
        const alreadyGuessed = data.guessed.includes(letter);

        button.disabled = data.gameOver || alreadyGuessed;
        button.classList.toggle("correct", data.correctLetters.includes(letter));
        button.classList.toggle("wrong", data.wrongLetters.includes(letter));
    });
}

async function startNewGame() {
    messageElement.className = "message";
    messageElement.textContent = "Starting a new game...";

    try {
        const data = await requestJson("/api/new", { method: "POST" });
        renderGame(data);
    } catch (error) {
        messageElement.textContent = "Could not reach the server. Is the C++ server running?";
        messageElement.classList.add("lose");
    }
}

async function makeGuess(letter) {
    if (!game || game.gameOver) {
        return;
    }

    try {
        const data = await requestJson("/api/guess", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: "letter=" + encodeURIComponent(letter)
        });

        renderGame(data);
    } catch (error) {
        messageElement.textContent = "Could not send your guess. Check that the server is running.";
        messageElement.classList.add("lose");
    }
}

document.addEventListener("keydown", function (event) {
    if (event.ctrlKey || event.altKey || event.metaKey) {
        return;
    }

    const letter = event.key.toLowerCase();

    if (/^[a-z]$/.test(letter)) {
        makeGuess(letter);
    }

    if (event.key === "Enter") {
        startNewGame();
    }
});

newGameButton.addEventListener("click", startNewGame);

buildKeyboard();
startNewGame();
