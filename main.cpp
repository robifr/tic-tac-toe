// Copyright (c) 2023 robifr
// This code is licensed under the MIT License.
// For details, visit: https://opensource.org/licenses/MIT

#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <random>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <cmath>

class Board;
class Player;

struct TextColor {
    static const std::string DEFAULT;
    static const std::string CYAN;
};

struct ConnectedCell {
    int row;
    int column;
    int verticalChain;
    int horizontalChain;
    int diagonalLeftChain;
    int diagonalRightChain;
    int totalConnected;
};

class Screen : public std::enable_shared_from_this<Screen> {
public:
    enum RetainedTextKey { 
        MAIN_MENU, 
        GAME_MODE_HEADER, 
        SELECTED_CELL_HISTORY
    };

    std::map<RetainedTextKey, std::string>& retainedText() const;

    std::unique_ptr<Board> const& board() const;

    void setBoard(std::unique_ptr<Board> board);

    std::unique_ptr<Board> requireGameMode();

    std::vector<std::shared_ptr<Player>> requirePlayers();

    int requireGridSize();

    void clear();

private:
    mutable std::map<RetainedTextKey, std::string> _retainedText {};
    std::unique_ptr<Board> _board = nullptr;
};

class Player {
public:
    Player(std::shared_ptr<Screen> const screen, int number, std::string marker);

    virtual ~Player() = default;

    virtual std::string name();

    virtual int requireCellSelection();

    const int& number() const;

    const std::string& marker() const;

    const int& score() const;

    void setScore(int score);

    const int& lastScore() const;

    void reset();

protected:
    std::weak_ptr<Screen> const _screen;
    const int _number;
    const std::string _marker;
    int _score = 0;
    int _lastScore = 0;
};

class Bot : public Player {
public:
    Bot(std::shared_ptr<Screen> const screen, int number, std::string marker);

    std::string name() override;

    int requireCellSelection() override;

private:
    std::vector<ConnectedCell> _rankAvailableCells(
        Player const& player, 
        std::set<int> const& availableCellNumbers);

    /**
     * Compute which one from two have shortest turn distance from this bot.
     * Assuming we compares second and fourth player,
     *    { p1, p2, bot, p4, p5 }
     * The one have shortest turn distance from this bot is fourth player,
     * with second player have longest turn.
     */
    Player const& _compareNextTurn(Player const& player1, Player const& player2);
};

class Board {
public:
    Board(
        std::shared_ptr<Screen> const screen, 
        const std::vector<std::shared_ptr<Player>> players, 
        int gridSize);

    virtual ~Board() = default;

    virtual bool isCompleted() = 0;

    const std::vector<std::shared_ptr<Player>>& players() const;

    const std::shared_ptr<Player>& playerTurn() const;

    int columnByCellNumber(int cellNumber);

    int rowByCellNumber(int cellNumber);

    int cellNumberByPosition(int row, int column);

    std::set<int> findAvailableCellNumbers();

    ConnectedCell findConnectedCell(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    std::string scoreText();

    std::string gridLayoutText();

    std::string playerTurnText();

    std::string resultText();

    bool markCellByNumber(int cellNumber, std::string marker);

    void reset();

    void togglePlayerTurn();

    int requireGridSelection();

    bool requireRematch();

protected:
    std::weak_ptr<Screen> const _screen;
    const std::vector<std::shared_ptr<Player>> _players;
    const int _gridSize;
    std::vector<std::vector<std::string>> _grid;
    std::shared_ptr<Player> _playerTurn = nullptr;

    int _countChainByDirection(
        int row, 
        int column, 
        int deltaRow, 
        int deltaColumn,
        std::string targetMarker,
        int maxChain);

    int _countCurrentTopChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countCurrentBottomChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countCurrentLeftChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countCurrentRightChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countTopLeftChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countTopRightChain(
        int row,
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countBottomLeftChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());

    int _countBottomRightChain(
        int row, 
        int column, 
        std::string targetMarker,
        int maxChain = std::numeric_limits<int>::max());
};

class ClassicBoard : public Board {
public:
    ClassicBoard(
        std::shared_ptr<Screen> const screen, 
        const std::vector<std::shared_ptr<Player>> players);

    static const std::string nameAndDescription();

    bool isCompleted() override;
};

class FrenzyBoard : public Board {
public:
    FrenzyBoard(
        std::shared_ptr<Screen> const screen, 
        const std::vector<std::shared_ptr<Player>> players, 
        int gridSize);

    static const std::string nameAndDescription();

    bool isCompleted() override;
};

const std::string TextColor::DEFAULT = "\033[0m";

const std::string TextColor::CYAN = "\033[96m";

std::map<Screen::RetainedTextKey, std::string>& Screen::retainedText() const { return this->_retainedText; }

std::unique_ptr<Board> const& Screen::board() const { return this->_board; }

void Screen::setBoard(std::unique_ptr<Board> board) { this->_board = std::move(board); }

std::unique_ptr<Board> Screen::requireGameMode() {
    this->_retainedText[RetainedTextKey::MAIN_MENU] = 
        "Tic-Tac-Toe\n"
        "-----------\n"
        "1. Classic\n"
        "2. Frenzy\n";
    std::unique_ptr<Board> board;
    int gameMode = -1;

    this->clear();
    std::cout << this->_retainedText[RetainedTextKey::MAIN_MENU] << "\n";

    while (true) {
        std::string line;
        
        std::cout << "Select game mode: ";
        std::getline(std::cin, line);
        std::cout << "\n";

        std::stringstream lineStream(line);

        if (lineStream >> gameMode 
            && lineStream.eof()
            && gameMode >= 1
            && gameMode <= 2) {
            switch (gameMode) {
                case 1: {
                    this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] = 
                        ClassicBoard::nameAndDescription();
                    const std::vector<std::shared_ptr<Player>> players = this->requirePlayers();
                    board = std::make_unique<ClassicBoard>(shared_from_this(), players);
                    break;
                }

                case 2: {
                    this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] = 
                        FrenzyBoard::nameAndDescription();
                    const int gridSize = this->requireGridSize();
                    const std::vector<std::shared_ptr<Player>> players = this->requirePlayers(); 
                    board = std::make_unique<FrenzyBoard>(shared_from_this(), players, gridSize);
                    break;
                }       	
            }

            break;
        }

        gameMode = -1;
        this->clear();

        std::cout << this->_retainedText[RetainedTextKey::MAIN_MENU]  
            << "\n** Invalid game mode, please reselect!\n";
    }

    board->togglePlayerTurn();
    return board;
}

std::vector<std::shared_ptr<Player>> Screen::requirePlayers() {
    std::vector<std::shared_ptr<Player>> players {};

    this->clear();
    std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] << "\n";

    while (true) {
        std::string line;

        //Requiring for number of players.
        std::cout << "Input number of players (min 2): ";
        std::getline(std::cin, line);

        this->clear();
        std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER];

        int totalPlayers = 0;
        std::stringstream lineStream(line);

        if (!(lineStream >> totalPlayers) || !lineStream.eof() || totalPlayers < 2) {   
            std::cout << "\n** Invalid number of players, please reinput!\n";
            continue;
        }

        const std::function<std::string()> readyPlayersText = [&players, &totalPlayers]() {
            std::ostringstream text;
            std::ostringstream readyPlayersText;

            for (const std::shared_ptr<Player>& player : players) {
                readyPlayersText << player->name() << "-" << player->number()
                    << " (" << player->marker() << ") is ready!\n";
            }

            const std::string readyPlayers = readyPlayersText.str().empty() 
                ? "" 
                : "\n" + readyPlayersText.str();
            text << players.size() << "/" << totalPlayers << " Players are set.\n" << readyPlayers;

            return text.str();
        };
        const std::function<std::string()> setupPlayerNumberText = [&players]() {
            return "\nSetting up player-" + std::to_string(players.size() + 1) + "...\n";
        };
        std::set<std::string> usedMarkers {};

        std::cout << "\n" << readyPlayersText() << setupPlayerNumberText();

        while (true) {
            //Requiring for player's unique marker.
            std::cout << "Marker: (1 char) ";
            std::getline(std::cin, line);

            this->clear();
            std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER]
                << "\n" << readyPlayersText() << setupPlayerNumberText();

            lineStream.clear();
            lineStream.str(line);

            if (line.length() != 1 //Only allow single character.
                || usedMarkers.find(line) != usedMarkers.end()) { //Disallow using same marker.
                std::cout << "\n** Invalid marker, please reinput!\n";
                continue;
            }

            const std::string marker = line;
            usedMarkers.insert(marker);

            std::cout << "Marker: " << marker << "\n";

            while (true) {
                //Requiring for player's type, whether bot or human.
                std::cout << "As a bot? (y/n): ";
                std::getline(std::cin, line);

                this->clear();
                std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] 
                    << "\n" << readyPlayersText() << setupPlayerNumberText() << "Marker: " << marker << "\n";

                for (char& ch : line) ch = std::tolower(ch);

                std::shared_ptr<Player> player;

                if (line == "y" || line == "yes") {
                    player = std::make_shared<Bot>(shared_from_this(), players.size() + 1, marker);

                } else if (line == "n" || line == "no") {
                    player = std::make_shared<Player>(shared_from_this(), players.size() + 1, marker);

                } else {
                    std::cout << "\n** Invalid player option, please reselect!\n";                 
                    continue;
                }

                players.push_back(player);
                break;
            }

            this->clear();
            std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] << "\n" << readyPlayersText();

            //Keep looping until every player has been created.
            if (players.size() == totalPlayers) break;

            std::cout << setupPlayerNumberText();
        }

        std::cout << "\nInput anything to start...";
        std::getline(std::cin, line);
        std::cout << "\n";
        break;
    }

    return players;
}

int Screen::requireGridSize() {
    int gridSize = 0;

    this->clear();
    std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] << "\n";

    while (true) {
        std::string line;
        std::cout << "Input grid size (min 3): ";
        std::getline(std::cin, line);
        std::cout << "\n";

        std::stringstream lineStream(line);

        if (lineStream >> gridSize && lineStream.eof() && gridSize >= 3) break;

        gridSize = 0;
        this->clear();

        std::cout << this->_retainedText[RetainedTextKey::GAME_MODE_HEADER] 
            << "\n** Invalid grid size, please reinput!\n";
    }  

    return gridSize;
}

void Screen::clear() { std::cout << "\033[H\033[2J\033[3J"; }

Player::Player(std::shared_ptr<Screen> const screen, int number, std::string marker) : 
    _screen(screen),
    _number(number),
    _marker(marker) {
}

std::string Player::name() { return "Player"; }

int Player::requireCellSelection() {
    const std::shared_ptr<Screen> screen = this->_screen.lock();
    if (screen == nullptr) throw std::runtime_error("Weak pointer expired.");

    return screen->board()->requireGridSelection(); 
}

const int& Player::number() const { return this->_number; }

const std::string& Player::marker() const { return this->_marker; }

const int& Player::score() const { return this->_score; }

void Player::setScore(int score) { 
    this->_lastScore = this->_score;
    this->_score = score;
}

const int& Player::lastScore() const { return this->_lastScore; }

void Player::reset() {
    this->_score = 0;
    this->_lastScore = 0;
}

Bot::Bot(std::shared_ptr<Screen> const screen, int number, std::string marker) : 
    Player(screen, number, marker) {
}

std::string Bot::name() { return "Bot"; }

int Bot::requireCellSelection() {
    const std::shared_ptr<Screen> screen = this->_screen.lock();
    if (screen == nullptr) throw std::runtime_error("Weak pointer expired.");

    const int totalPlayers = screen->board()->players().size();
    const std::set<int>& availableCells = screen->board()->findAvailableCellNumbers();
    const std::vector<ConnectedCell>& rankedCells = this->_rankAvailableCells(*this, availableCells);
    std::vector<ConnectedCell> playerToBlockCells {};
    std::shared_ptr<const Player> playerToBlock = nullptr;
    
    //Searching player to block.
    //Iterate from the next player turn — right side in the array — from this bot, 
    //up until latest player turn — left side in the array —, whilist ignoring this bot index.
    //This bot index same as this bot number - 1.
    for (int n = this->_number % totalPlayers, i = n;
        n < totalPlayers + this->_number % totalPlayers && i != this->_number - 1;
        n++, i = n % totalPlayers) {
        const std::shared_ptr<const Player>& player = screen->board()->players().at(i);
        const std::vector<ConnectedCell>& playerCells = this->_rankAvailableCells(*player, availableCells);   
        const Player& playerShortestNextTurn = playerToBlock == nullptr
            ? *player
            : this->_compareNextTurn(*playerToBlock, *player);
            
        if (playerToBlockCells.empty()
            //Replace targeted player when the new one have more cell to connect.
            || playerCells.at(0).totalConnected > playerToBlockCells.at(0).totalConnected
            //Prioritize to block player with shortest turn distance from this bot.
            || (playerCells.at(0).totalConnected == playerToBlockCells.at(0).totalConnected
                && &playerShortestNextTurn == &*player)) {
            playerToBlockCells = playerCells;
            playerToBlock = player;
        }
    }
    
    std::unique_ptr<const ConnectedCell> bestCell = rankedCells.at(0).totalConnected >= 3
        ? std::make_unique<ConnectedCell>(rankedCells.at(0))
        : nullptr;
     
    //Looking whether it's necessary to block.
    for (const ConnectedCell& playerCell : playerToBlockCells) {
        //No need to block when they have nothing to connect.
        if (playerCell.totalConnected == 0) break;
        
        //Immediately block when the amount of connected cell is bigger than this bot.
        if (playerCell.totalConnected > rankedCells.at(0).totalConnected) {
            bestCell = std::make_unique<ConnectedCell>(playerCell);
            break;
        } 
        
        bool isBestCellFound = false;
        
        //When the targeted player has an equal maximum amount of cell to connect.
        //Try to find a cell that's not only can block them but also create a new chain.
        //If can't be found, just block any cell with the same total connected cell.
        //
        //Assume this bot ranked cells : { 11, 12, 13, ... }
        //Targeted player ranked cells : { 16, 17, 18, 13, ... }
        //If they all have the same total connected cell. That's means 13 is the best cell,
        //not only for this bot to make a chain, but also effectively block them.
        for (const ConnectedCell& rankedCell : rankedCells) {
            if (playerCell.totalConnected == rankedCells.at(0).totalConnected 
                && playerCell.totalConnected == rankedCell.totalConnected) {
                bestCell = std::make_unique<ConnectedCell>(playerCell);
                const bool isSamePosition = playerCell.row == rankedCell.row 
                    && playerCell.column == rankedCell.column;

                //Keep iterating until we find it.
                if (isSamePosition) isBestCellFound = true;
                else continue;
            }
            
            break;
        }
        
        if (isBestCellFound) break;
    }
    
    //Found the best cell.
    if (bestCell != nullptr) {
        return screen->board()->cellNumberByPosition(bestCell->row, bestCell->column);

    //There's atleast a cell to chain.
    } else if (rankedCells.at(0).verticalChain >= 1
        || rankedCells.at(0).horizontalChain >= 1
        || rankedCells.at(0).diagonalLeftChain >= 1
        || rankedCells.at(0).diagonalRightChain >= 1) {
        return screen->board()->cellNumberByPosition(rankedCells.at(0).row, rankedCells.at(0).column);
    }

    const std::vector<int> availableCellsArray(availableCells.begin(), availableCells.end());
    std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<int> randomIndex(
        0, 
        std::max(0, static_cast<int>(availableCells.size()) - 1)
    );

    //Pick randomly when there's nothing to chain.
    return availableCellsArray.at(randomIndex(engine));
}

std::vector<ConnectedCell> Bot::_rankAvailableCells(
    Player const& player, 
    std::set<int> const& availableCellNumbers) {
    const std::shared_ptr<Screen> screen = this->_screen.lock();
    if (screen == nullptr) throw std::runtime_error("Weak pointer expired.");

    std::vector<ConnectedCell> cells {};

    for (int cell : availableCellNumbers) {
        const int row = screen->board()->rowByCellNumber(cell);
        const int column = screen->board()->columnByCellNumber(cell);
        const ConnectedCell connectedCell = screen->board()->findConnectedCell(row, column, player.marker());

        cells.push_back(connectedCell);
    }

    std::sort(cells.begin(), cells.end(), [](const ConnectedCell& a, const ConnectedCell& b) {
        const int aChains = a.verticalChain + a.horizontalChain + a.diagonalLeftChain + a.diagonalRightChain;
        const int bChains = b.verticalChain + b.horizontalChain + b.diagonalLeftChain + b.diagonalRightChain;

        return a.totalConnected > b.totalConnected
            //When total connected cells are equals, rank based on their chains instead.
            || (a.totalConnected == b.totalConnected && aChains > bChains);
    });
    return cells;
}

Player const& Bot::_compareNextTurn(Player const& player1, Player const& player2) {
    if ((player1.number() < player2.number() && player2.number() < this->_number) //P1 < P2 < Bot.
        || (player2.number() < this->_number && this->_number < player1.number()) //P2 < Bot < P1.
        || (this->_number < player1.number() && player1.number() < player2.number()) //Bot < P1 < P2.
        //It should never happen anyway as every number are unique.
        //Normally when this bot number equals to second player number, 
        //the one returned from this method will be second player, but we don't want that.
        || this->_number == player2.number()) {
        return player1;
    }

    return player2;
}

Board::Board(
    std::shared_ptr<Screen> const screen, 
    const std::vector<std::shared_ptr<Player>> players,
    int gridSize) :
    _screen(screen),
    _players(players),
    _gridSize(gridSize),
    _grid(this->_gridSize, std::vector<std::string>(this->_gridSize)) {
}

const std::vector<std::shared_ptr<Player>>& Board::players() const { return this->_players; }

const std::shared_ptr<Player>& Board::playerTurn() const { return this->_playerTurn; }

int Board::columnByCellNumber(int cellNumber) { return cellNumber % this->_gridSize; }

int Board::rowByCellNumber(int cellNumber) { return cellNumber / this->_gridSize; }

int Board::cellNumberByPosition(int row, int column) { return row * this->_gridSize + column; }

std::set<int> Board::findAvailableCellNumbers() {
    std::set<int> cells {};

    for (int row = 0; row < this->_grid.size(); row++) {
        for (int column = 0; column < this->_grid.at(row).size(); column++) {
            if (!this->_grid.at(row).at(column).empty()) continue;

            const int cellNumber = this->cellNumberByPosition(row, column);
            cells.insert(cellNumber);
        }
    }

    return cells;
}

ConnectedCell Board::findConnectedCell(int row, int column, std::string targetMarker, int maxChain) {
    //Sum each chain with their opposite direction — top and down, etc. — before totaling them all.
    //Because we don't want zig-zagging to count as connected cells.
    //Also imagine a case where user chained a character (x) between left (x1) and right (x2).
    //    [ x1 ][ x ][ x2 ]
    //Horizontal chain would be (x) with (x1), and (x) with (x2),
    //hence we have two chains here and it's valid to be count as three connected cells.

    int total = 0;

    const int verticalChain = this->_countCurrentTopChain(row, column, targetMarker, maxChain)
        + this->_countCurrentBottomChain(row, column, targetMarker, maxChain);
    total += verticalChain >= 2 ? verticalChain + 1 : 0; //+1 to include current cell.

    const int horizontalChain = this->_countCurrentLeftChain(row, column, targetMarker, maxChain)
        + this->_countCurrentRightChain(row, column, targetMarker, maxChain);
    total += horizontalChain >= 2 ? horizontalChain + 1 : 0; //+1 to include current cell.

    const int diagonalLeftChain = this->_countTopLeftChain(row, column, targetMarker, maxChain)
        + this->_countBottomRightChain(row, column, targetMarker, maxChain);
    total += diagonalLeftChain >= 2 ? diagonalLeftChain + 1 : 0; //+1 to include current cell.

    const int diagonalRightChain = this->_countTopRightChain(row, column, targetMarker, maxChain)
        + this->_countBottomLeftChain(row, column, targetMarker, maxChain);
    total += diagonalRightChain >= 2 ? diagonalRightChain + 1 : 0; //+1 to include current cell.

    return ConnectedCell {
        .row = row,
        .column = column,
        .verticalChain = verticalChain,
        .horizontalChain = horizontalChain,
        .diagonalLeftChain = diagonalLeftChain,
        .diagonalRightChain = diagonalRightChain,
        .totalConnected = total
    };
}

std::string Board::scoreText() {
    std::ostringstream text;
    text << "Score: \n";

    for (size_t i = 0; i < this->_players.size(); i++) {
        Player& player = *this->_players.at(i);   
        text << player.name() << "-" << i + 1 << " (" << player.marker() << "): " << player.score() << "\n";
    }

    return text.str();
}

std::string Board::gridLayoutText() {
    std::ostringstream text;
    int cellNumber = 0;

    for (int y = 0; y < this->_gridSize; y++) {
        //Note: Amount of strips here are equal to cell 2 width + 2 space around + 1 pipe (|).
        for (int x = 0; x < this->_gridSize; x++) text << "-----";
        text << "-\n| ";

        for (int x = 0; x < this->_gridSize; x++) {
            const std::string currentMarker = this->_grid.at(y).at(x);
            const std::string marker = currentMarker.empty()
                //Display its cell number instead of empty string.
                ? std::to_string(cellNumber % (this->_gridSize * this->_gridSize))
                : currentMarker;

            //Colorize current cell when they were chained with others.
            if (!currentMarker.empty() 
                && this->findConnectedCell(y, x, currentMarker, 3).totalConnected >= 3) {
                text << TextColor::CYAN;
            }

            text << std::setw(2) << marker << TextColor::DEFAULT << " | ";
            cellNumber++;
        }

        text << "\n";
    }

    for (int x = 0; x < this->_gridSize; x++) text << "-----";
    text << "-\n";

    return text.str();
}

std::string Board::playerTurnText() {
    if (this->_playerTurn == nullptr) throw std::runtime_error("Player turn hasn't been set.");

    std::ostringstream text;
    text << this->_playerTurn->name() << "-" << this->_playerTurn->number() 
        << " (" << this->_playerTurn->marker() << ") turn...\n";

    return text.str();
}

std::string Board::resultText() {
    std::shared_ptr<Player> topPlayer = nullptr;
    int topScore = 0;

    for (const std::shared_ptr<Player>& player : this->_players) {
        if (player->score() > topScore) {
            topPlayer = player;
            topScore = player->score();
            
        //Remove top player if multiple player have equal score.
        } else if (player->score() == topScore) {
            topPlayer = nullptr;
        }
    }

    std::ostringstream winText;
    const std::string drawText = "Game over! The game ends with draw.\n";

    if (topPlayer == nullptr) return drawText;

    winText << "Game over! " << topPlayer->name() << "-" << topPlayer->number()
        << " (" << topPlayer->marker() << ") has won!\n";  	
    return winText.str();
}

bool Board::markCellByNumber(int cellNumber, std::string marker) {
    const int x = this->columnByCellNumber(cellNumber);
    const int y = this->rowByCellNumber(cellNumber);

    if (x < 0 
        || x >= this->_gridSize 
        || y < 0 
        || y >= this->_gridSize
        || !this->_grid.at(y).at(x).empty()
        || this->_playerTurn == nullptr) {
        return false;
    }

    this->_grid.at(y).at(x) = marker;
    const int totalConnected = this->findConnectedCell(y, x, marker).totalConnected;
    const int score = this->_playerTurn->score() + totalConnected;

    this->_playerTurn->setScore(score);
    return true;
}

void Board::reset() {
    this->_grid = std::vector<std::vector<std::string>>(
        this->_gridSize,   
        std::vector<std::string>(this->_gridSize)
    );
    this->_playerTurn = nullptr;

    for (std::shared_ptr<Player> player : this->_players) player->reset();
}

void Board::togglePlayerTurn() {
    std::default_random_engine engine(std::random_device{}());
    std::uniform_int_distribution<int> randomIndex(
        0, 
        std::max(0, static_cast<int>(this->_players.size()) - 1)
    );
    int playerTurnIndex = this->_playerTurn == nullptr ? -1 : this->_playerTurn->number() - 1;

    //Only randomly pick the turn during initial run. 
    //From then on, it's incremented from previous player index.
    if (playerTurnIndex == -1) playerTurnIndex = randomIndex(engine);
    else if (playerTurnIndex + 1 >= this->_players.size()) playerTurnIndex = 0;
    else playerTurnIndex++;

    this->_playerTurn = this->_players.at(playerTurnIndex);
}

int Board::requireGridSelection() {
    std::shared_ptr<Screen> screen = this->_screen.lock();
    if (!screen) throw std::runtime_error("Weak pointer expired.");

    const int maxCell = std::max(0, (this->_gridSize * this->_gridSize) - 1);
    const int maxRowOrColumn = std::max(0, this->_gridSize - 1);
    int selectedCell = -1;

    screen->clear();
    std::cout << screen->retainedText()[Screen::RetainedTextKey::GAME_MODE_HEADER] << "\n"
        << screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY]
        << this->scoreText() << "\n" 
        << this->gridLayoutText() << "\n"
        << this->playerTurnText();

    while (true) {
        std::string line;
        std::cout << "Select cell by number: ";
        std::getline(std::cin, line);
        std::cout << "\n";

        std::stringstream lineStream(line);

        if (lineStream >> selectedCell && lineStream.eof()) {
            const int x = this->columnByCellNumber(selectedCell);
            const int y = this->rowByCellNumber(selectedCell);

            if (selectedCell >= 0
                && selectedCell <= maxCell
                && x >= 0 
                && x <= maxRowOrColumn
                && y >= 0 
                && y <= maxRowOrColumn
                && this->_grid.at(y).at(x).empty()) {
                break;
            }
        }

        selectedCell = -1;
        screen->clear();

        std::cout << screen->retainedText()[Screen::RetainedTextKey::GAME_MODE_HEADER] << "\n"
            << screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY]
            << this->scoreText() << "\n" 
            << this->gridLayoutText() << "\n"
            << this->playerTurnText() << "\n"
            << "** Invalid cell number, please reselect!\n";
    }

    return selectedCell;
}

bool Board::requireRematch() {
    std::string line;

    std::cout << "Rematch? (y/n) ";
    std::getline(std::cin, line);
    std::cout << "\n";

    for (char& ch : line) ch = std::tolower(ch);

    return line == "y" || line == "yes";
}

int Board::_countChainByDirection(
    int row, 
    int column, 
    int deltaRow, 
    int deltaColumn, 
    std::string targetMarker,
    int maxChain) {
    int chain = 0;       

    do {
        row += deltaRow;
        column += deltaColumn;

        //The loop traverse in the specified direction until a different character 
        //is encountered or the grid boundaries are reached.
        if (row < 0 
            || row >= this->_gridSize
            || column < 0 
            || column >= this->_gridSize
            || this->_grid.at(row).at(column) != targetMarker) {
            break;
        }        

        chain++;

    } while (chain < maxChain);

    return chain;
}

int Board::_countCurrentTopChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, -1, 0, targetMarker, maxChain);
}

int Board::_countCurrentBottomChain(int row, int column, std::string targetMarker, int maxChain)  {
    return this->_countChainByDirection(row, column, 1, 0, targetMarker, maxChain);
}

int Board::_countCurrentLeftChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, 0, -1, targetMarker, maxChain);
}

int Board::_countCurrentRightChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, 0, 1, targetMarker, maxChain);
}

int Board::_countTopLeftChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, -1, -1, targetMarker, maxChain);
}

int Board::_countTopRightChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, -1, 1, targetMarker, maxChain);
}

int Board::_countBottomLeftChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, 1, -1, targetMarker, maxChain);
}

int Board::_countBottomRightChain(int row, int column, std::string targetMarker, int maxChain) {
    return this->_countChainByDirection(row, column, 1, 1, targetMarker, maxChain);
}

ClassicBoard::ClassicBoard(
    std::shared_ptr<Screen> const screen, 
    const std::vector<std::shared_ptr<Player>> players) : 
    Board(screen, players, players.size() + 1) {
}

const std::string ClassicBoard::nameAndDescription() {
    const std::string title =  "Classic";
    std::string text = title + "\n";

    for (size_t i = 0; i < title.length(); i++) text += "-";

    text += "\nConnect three characters to win the game.\n";
    return text;
}

bool ClassicBoard::isCompleted() {
    if (this->findAvailableCellNumbers().empty()) return true;

    //Finish the game as soon someone scored.
    for (const std::shared_ptr<Player>& player : this->_players) {
        if (player->score() > 0) return true;
    }

    return false;
}

FrenzyBoard::FrenzyBoard(
    std::shared_ptr<Screen> const screen, 
    const std::vector<std::shared_ptr<Player>> players, 
    int gridSize) : 
    Board(screen, players, gridSize) {
}

const std::string FrenzyBoard::nameAndDescription() {
    const std::string title =  "Frenzy";
    std::string text = title + "\n";

    for (size_t i = 0; i < title.length(); i++) text += "-";

    text += "\nConnect three or more characters to earn points.\n"
        "The one with the most points wins.\n";
    return text;
}

bool FrenzyBoard::isCompleted() { return this->findAvailableCellNumbers().empty(); }

int main() {
    std::shared_ptr<Screen>screen = std::make_shared<Screen>();
    screen->setBoard(screen->requireGameMode());

    while (true) {
        if (screen->board()->isCompleted()) {
            std::cout << screen->board()->resultText() << "\n";

            screen->board()->reset();
            screen->board()->togglePlayerTurn();
            screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY] = "";

            //User don't want to rematch.
            if (!screen->board()->requireRematch()) {
                screen->retainedText().clear();
                screen->setBoard(screen->requireGameMode());
            }

            continue;
        }

        const std::shared_ptr<Player> currentPlayerTurn = screen->board()->playerTurn();
        const int selectedCell = currentPlayerTurn->requireCellSelection();

        screen->board()->markCellByNumber(selectedCell, currentPlayerTurn->marker());
        screen->board()->togglePlayerTurn();
        screen->clear();

        const int scoreGained = currentPlayerTurn->score() - currentPlayerTurn->lastScore();
        std::ostringstream selectedCellText;

        selectedCellText << screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY]
            << currentPlayerTurn->name() << "-" << currentPlayerTurn->number()
            << " (" << currentPlayerTurn->marker() << ") selected '" << selectedCell << "'";

        if (scoreGained > 0) selectedCellText << ", gained +" << scoreGained << " points";

        screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY] = selectedCellText.str() + "\n\n";
        std::cout << screen->retainedText()[Screen::RetainedTextKey::GAME_MODE_HEADER] << "\n"
            << screen->retainedText()[Screen::RetainedTextKey::SELECTED_CELL_HISTORY]
            << screen->board()->scoreText() << "\n" 
            << screen->board()->gridLayoutText() << "\n";
    }

    return 0;
};