#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

const int DEFAULT_PORT = 8081;
const int MAX_WRONG = 6;

const std::vector<std::string> WORDS = {
    "apple",  "banana", "orange", "tiger",  "house",
    "school", "pencil", "garden", "rocket", "summer",
    "winter", "coffee", "monkey", "planet", "yellow",
    "puzzle", "bridge", "window", "flower", "music"
};

struct Game {
    std::string word;
    std::string guessed;
    int wrong = 0;
    bool won = false;
    bool lost = false;
};

struct HttpRequest {
    std::string method;
    std::string target;
    std::string body;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain; charset=utf-8";
    std::string body;
};

Game currentGame;

char toLowerChar(char character) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

bool containsLetter(const std::string& letters, char letter) {
    return letters.find(letter) != std::string::npos;
}

bool gameIsOver(const Game& game) {
    return game.won || game.lost;
}

void resetGame(Game& game) {
    static std::mt19937 randomEngine(std::random_device{}());
    std::uniform_int_distribution<size_t> pickWord(0, WORDS.size() - 1);

    game.word = WORDS[pickWord(randomEngine)];
    game.guessed.clear();
    game.wrong = 0;
    game.won = false;
    game.lost = false;
}

std::string maskedWord(const Game& game) {
    std::string result;

    for (char letter : game.word) {
        if (containsLetter(game.guessed, letter)) {
            result += letter;
        } else {
            result += '_';
        }
    }

    return result;
}

bool wordIsComplete(const Game& game) {
    for (char letter : game.word) {
        if (!containsLetter(game.guessed, letter)) {
            return false;
        }
    }

    return true;
}

std::string guessLetter(Game& game, char rawLetter) {
    char letter = toLowerChar(rawLetter);

    if (gameIsOver(game)) {
        return "The game is over. Start a new game.";
    }

    if (letter < 'a' || letter > 'z') {
        return "Please guess a letter from A to Z.";
    }

    if (containsLetter(game.guessed, letter)) {
        return "You already guessed that letter.";
    }

    game.guessed += letter;

    if (game.word.find(letter) != std::string::npos) {
        if (wordIsComplete(game)) {
            game.won = true;
            return "You won!";
        }

        return "Good guess!";
    }

    game.wrong++;

    if (game.wrong >= MAX_WRONG) {
        game.lost = true;
        return "Out of lives!";
    }

    return "Wrong guess. Try again.";
}

std::string jsonEscape(const std::string& text) {
    std::string result;

    for (char character : text) {
        if (character == '"') {
            result += "\\\"";
        } else if (character == '\\') {
            result += "\\\\";
        } else if (character == '\n') {
            result += "\\n";
        } else if (character == '\r') {
            result += "\\r";
        } else if (character == '\t') {
            result += "\\t";
        } else {
            result += character;
        }
    }

    return result;
}

std::string jsonLetterArray(const std::vector<char>& letters) {
    std::string result = "[";

    for (size_t index = 0; index < letters.size(); index++) {
        if (index > 0) {
            result += ",";
        }

        result += '"';
        result += letters[index];
        result += '"';
    }

    result += "]";
    return result;
}

std::string gameToJson(const Game& game, const std::string& message) {
    std::vector<char> guessedLetters(game.guessed.begin(), game.guessed.end());
    std::vector<char> correctLetters;
    std::vector<char> wrongLetters;

    for (char letter : guessedLetters) {
        if (game.word.find(letter) != std::string::npos) {
            correctLetters.push_back(letter);
        } else {
            wrongLetters.push_back(letter);
        }
    }

    bool over = gameIsOver(game);
    std::string answer = over ? game.word : "";

    std::ostringstream json;
    json << "{";
    json << "\"masked\":\"" << maskedWord(game) << "\",";
    json << "\"wrong\":" << game.wrong << ",";
    json << "\"maxWrong\":" << MAX_WRONG << ",";
    json << "\"guessed\":" << jsonLetterArray(guessedLetters) << ",";
    json << "\"correctLetters\":" << jsonLetterArray(correctLetters) << ",";
    json << "\"wrongLetters\":" << jsonLetterArray(wrongLetters) << ",";
    json << "\"gameOver\":" << (over ? "true" : "false") << ",";
    json << "\"won\":" << (game.won ? "true" : "false") << ",";
    json << "\"lost\":" << (game.lost ? "true" : "false") << ",";
    json << "\"answer\":\"" << jsonEscape(answer) << "\",";
    json << "\"message\":\"" << jsonEscape(message) << "\"";
    json << "}";

    return json.str();
}

HttpResponse jsonResponse(const std::string& body, int status = 200) {
    return {status, "application/json; charset=utf-8", body};
}

bool readFile(const std::string& path, std::string& content) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
}

HttpResponse serveFile(const std::string& path, const std::string& contentType) {
    std::string content;

    if (!readFile(path, content)) {
        return {404, "text/plain; charset=utf-8", "File not found: " + path};
    }

    return {200, contentType, content};
}

std::string urlDecode(const std::string& value) {
    std::string result;

    for (size_t index = 0; index < value.size(); index++) {
        if (value[index] == '+') {
            result += ' ';
        } else if (value[index] == '%' && index + 2 < value.size()) {
            std::string hex = value.substr(index + 1, 2);
            char decoded = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result += decoded;
            index += 2;
        } else {
            result += value[index];
        }
    }

    return result;
}

std::string formValue(const std::string& body, const std::string& key) {
    size_t start = 0;

    while (start <= body.size()) {
        size_t ampersand = body.find('&', start);
        size_t length = (ampersand == std::string::npos) ? std::string::npos : ampersand - start;
        std::string pair = body.substr(start, length);
        size_t equals = pair.find('=');

        if (equals != std::string::npos && pair.substr(0, equals) == key) {
            return urlDecode(pair.substr(equals + 1));
        }

        if (ampersand == std::string::npos) {
            break;
        }

        start = ampersand + 1;
    }

    return "";
}

std::string pathOnly(const std::string& target) {
    size_t questionMark = target.find('?');

    if (questionMark == std::string::npos) {
        return target;
    }

    return target.substr(0, questionMark);
}

HttpResponse handleRequest(const HttpRequest& request) {
    std::string path = pathOnly(request.target);

    if (request.method == "GET") {
        if (path == "/" || path == "/index.html") {
            return serveFile("frontend/index.html", "text/html; charset=utf-8");
        }

        if (path == "/style.css") {
            return serveFile("frontend/style.css", "text/css; charset=utf-8");
        }

        if (path == "/script.js") {
            return serveFile("frontend/script.js", "application/javascript; charset=utf-8");
        }

        if (path == "/api/state") {
            return jsonResponse(gameToJson(currentGame, "Current game."));
        }
    }

    if (request.method == "POST") {
        if (path == "/api/new") {
            resetGame(currentGame);
            return jsonResponse(gameToJson(currentGame, "New game started. Guess a letter!"));
        }

        if (path == "/api/guess") {
            std::string letter = formValue(request.body, "letter");

            if (letter.empty()) {
                return jsonResponse(gameToJson(currentGame, "Please choose a letter."), 400);
            }

            std::string message = guessLetter(currentGame, letter[0]);
            return jsonResponse(gameToJson(currentGame, message));
        }
    }

    return {404, "text/plain; charset=utf-8", "Page not found."};
}

std::string statusText(int status) {
    if (status == 200) {
        return "OK";
    }

    if (status == 400) {
        return "Bad Request";
    }

    if (status == 404) {
        return "Not Found";
    }

    return "OK";
}

std::string buildHttpResponse(const HttpResponse& response) {
    std::ostringstream output;

    output << "HTTP/1.1 " << response.status << " " << statusText(response.status) << "\r\n";
    output << "Content-Type: " << response.contentType << "\r\n";
    output << "Content-Length: " << response.body.size() << "\r\n";
    output << "Cache-Control: no-store\r\n";
    output << "Connection: close\r\n";
    output << "\r\n";
    output << response.body;

    return output.str();
}

std::string readRequest(SOCKET clientSocket) {
    std::string request;
    char buffer[4096];
    size_t headerEnd = std::string::npos;

    while (headerEnd == std::string::npos) {
        int received = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (received <= 0) {
            return "";
        }

        request.append(buffer, received);
        headerEnd = request.find("\r\n\r\n");

        if (request.size() > 1024 * 1024) {
            return "";
        }
    }

    std::string headers = request.substr(0, headerEnd);
    std::string lowerHeaders = headers;
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

    size_t contentLength = 0;
    size_t lengthPosition = lowerHeaders.find("content-length:");

    if (lengthPosition != std::string::npos) {
        lengthPosition += std::string("content-length:").size();
        size_t lineEnd = lowerHeaders.find("\r\n", lengthPosition);

        if (lineEnd == std::string::npos) {
            lineEnd = lowerHeaders.size();
        }

        std::string value = lowerHeaders.substr(lengthPosition, lineEnd - lengthPosition);

        try {
            contentLength = static_cast<size_t>(std::stoul(value));
        } catch (...) {
            contentLength = 0;
        }
    }

    size_t bodyStart = headerEnd + 4;

    while (request.size() - bodyStart < contentLength) {
        int received = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (received <= 0) {
            break;
        }

        request.append(buffer, received);
    }

    return request;
}

HttpRequest parseRequest(const std::string& rawRequest) {
    HttpRequest request;
    size_t headerEnd = rawRequest.find("\r\n\r\n");

    if (headerEnd == std::string::npos) {
        return request;
    }

    std::string headers = rawRequest.substr(0, headerEnd);
    size_t firstLineEnd = headers.find("\r\n");
    std::string requestLine = headers.substr(0, firstLineEnd);
    std::istringstream lineStream(requestLine);
    std::string httpVersion;
    lineStream >> request.method >> request.target >> httpVersion;
    request.body = rawRequest.substr(headerEnd + 4);

    return request;
}

bool sendAll(SOCKET clientSocket, const std::string& data) {
    size_t totalSent = 0;

    while (totalSent < data.size()) {
        int sent = send(clientSocket, data.data() + totalSent,
                        static_cast<int>(data.size() - totalSent), 0);

        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }

    return true;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number. Example: hangman.exe 8081\n";
            return 1;
        }
    }

    if (port < 1 || port > 65535) {
        std::cerr << "Port must be between 1 and 65535.\n";
        return 1;
    }

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Could not start Windows sockets.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Could not create the server socket.\n";
        WSACleanup();
        return 1;
    }

    int reuseAddress = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<char*>(&reuseAddress), sizeof(reuseAddress));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port));
    address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Could not use port " << port << ". It may already be in use.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Could not start listening for connections.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    resetGame(currentGame);

    std::cout << "Hangman server is running.\n";
    std::cout << "Open http://localhost:" << port << " in your browser.\n";
    std::cout << "Press Ctrl+C to stop the server.\n\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        DWORD receiveTimeout = 3000;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<char*>(&receiveTimeout), sizeof(receiveTimeout));

        std::string rawRequest = readRequest(clientSocket);

        if (!rawRequest.empty()) {
            HttpRequest request = parseRequest(rawRequest);
            std::cout << request.method << " " << request.target << "\n";
            HttpResponse response = handleRequest(request);
            sendAll(clientSocket, buildHttpResponse(response));
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
