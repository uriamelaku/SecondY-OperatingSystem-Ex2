#include <iostream>
#include <vector>
#include <string>

using namespace std;

// Function to check for a win on the board
bool checkWin(const vector<char>& board, char player) {
    // Check for a win in rows
    for (int i = 0; i < 3; i++) {
        if (board[i * 3] == player && board[i * 3 + 1] == player && board[i * 3 + 2] == player)
        {
            return true;
        } 
    }
    // Check for a win in columns
    for (int i = 0; i < 3; i++) {
        if (board[i] == player && board[i + 3] == player && board[i + 6] == player)
        { 
            return true;
        }
    }
    // Check for a win in diagonals
    if ((board[0] == player && board[4] == player && board[8] == player) ||
        (board[2] == player && board[4] == player && board[6] == player)) 
    {
        return true;
    }
    return false;
}

// Function to print the board
void printBoard(const vector<char>& board) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            cout << board[i * 3 + j];
            if (j < 2) cout << "|";
        }
        cout << endl;
        if (i < 2) cout << "-----" << endl;
    }
}

// Function to check if the computer's move is valid
bool isComputerTurnValid(const vector<char>& board, const string& strategy, int index) {
    int boardIndex = strategy[index] - '1';
    // Check if the board at the index is empty
    if(board[boardIndex] == ' ') {
        return true;
    }
    return false;
}

// Function to check if the player's move is valid
bool isPlayerTurnValid(const vector<char>& board, int index) {
    if (index !=1 && index != 2 && index !=3 && index !=4 && index !=5 && index !=6 && index !=7 && index !=8 && index !=9) {
        cout << "Invalid input: Index out of range" << endl;
        return false;
    }
    if (board[index - 1] == ' ') {
        return true;
    }
    cout << "Invalid move: Position already taken" << endl;
    return false;
}

// Function to check if the strategy is valid
bool isValidStrategy(const string& strategy) {
    if (strategy.length() != 9) {
        cout << "Error: Strategy length is not valid" << endl;
        return false;
    }

    // Check if the strategy contains only digits between 1-9
    for (char c : strategy) {
        if (c < '1' || c > '9') {
            cout << "Error: Digits are not between 1-9" << endl;
            return false;
        }
    }
    // Check if the strategy contains unique digits
    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 9; j++) {
            if (strategy[i] == strategy[j]) {
                cout << "Error: Digits are not unique" << endl;
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Error: Input one element" << endl;
        return 0;
    }

    string strategy = argv[1];

    if (!isValidStrategy(strategy)) {
        return 0;
    }

    cout << "Welcome to Tic Tac Toe!" << endl;

    vector<char> board = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    bool isGameOver = false;
    char currentPlayer = 'X';
    int numOfEmptyBoxes = 9;
    int computerIndex = 0;

    // The game continues until there is a winner or a draw
    while (!isGameOver) {
        printBoard(board);

        if (currentPlayer == 'X') { // Computer's turn
            // If the board at the index is empty, fill it with X, if not check the next index in the strategy
            while (!isComputerTurnValid(board, strategy, computerIndex)) {
                computerIndex++;
            }
            // Fill the board with X at the index
            int boardIndex = strategy[computerIndex] - '1';
            board[boardIndex] = currentPlayer;
            cout << "Computer move: " << strategy[computerIndex] << endl;
        } 
        else { // Player's turn
            cout << "Enter your mov (1-9): ";
            int index;
            string temp;
            cin >> temp;
            if(temp < "1" || temp > "9") {
                cout << "Invalid input" << endl;
                continue;
            }
            index = stoi(temp);
            
            while (!isPlayerTurnValid(board, index)) {
                cout << "Enter your move (1-9): ";
                cin >> temp;
                if(temp < "1" || temp > "9") {
                    cout << "Invalid input" << endl;
                    continue;
                }
                index = stoi(temp);
            }
            // Fill the board with O at the index
            board[index - 1] = currentPlayer;
        }

        if (checkWin(board, currentPlayer)) {
            printBoard(board);
            if (currentPlayer == 'X') {
                cout << "I win!" << endl;
            } 
            else {
                cout << "I lost!" << endl;
            }
            break;
        } 
        else if (numOfEmptyBoxes == 1) {
            printBoard(board);
            cout << "Draw" << endl;
            break;
        }
        numOfEmptyBoxes--;
        // Switch the player
        if(currentPlayer == 'X') {
            currentPlayer = 'O';
        } 
        else {
            currentPlayer = 'X';
        }
       
    }

    return 0;
}
