// #define DEBUG_INIT_BOARD
// #define DEBUG_MOVE

#include <cstdlib>
#include <ctime>
#include <iostream>
#include "pawn.h"

int w_points, b_points;

vector<vector<Piece>> board;
vector<schar> whites, blacks;


/**
 * @brief Transforms given Move to corresponding string.
 * 
 * @param move move to transform to string
 * @return string corresponding to given Move
 */
string move_to_str(Move move){
    switch(move){
        case NW:
            return "NW";
            break;

        case North:
            return "North";
            break;

        case NE:
            return "NE";
            break;

        case SE:
            return "SE";
            break;

        case South:
            return "South";
            break;

        case SW:
            return "SW";
            break;

        default:
            break;
    }

    return "Invalid";
}

/**
 * @brief Transforms given string to a valid move.
 * 
 * @param str move as string
 * @return Move corresponding to given string
 */
Move str_to_move(string str){
    if(str.size() > 2 || str.size() == 0) return X_Move;

    // To Uppercase
    if(str[0] >= 'a') str[0] -= 32;
    if(str[1] >= 'a') str[1] -= 32;
    
    if(str[0] == 'S'){
        if(str.size() == 1) return South;

        if(str[1] == 'E') return SE;
        if(str[1] == 'W') return SW;

        return X_Move;
    }

    if(str[0] == 'N'){
        if(str.size() == 1) return North;

        if(str[1] == 'W') return NW;
        if(str[1] == 'E') return NE;
    
        return X_Move;
    }

    return X_Move;
}

/**
 * @brief Get the value to be saved in whites or blacks from position in board.
 * 
 * @param col Piece's column as int
 * @param row Piece's row as int
 * @return schar - pos to save in whites or blacks
 */
schar pair_to_pos(char col, char row){
    return (col << 4) | (row + 1);
}

/**
 * @brief Separates col from row in given pos.
 * 
 * @param pos value saved in whites or blacks
 * @return pair<char, char> containing col and row values, respectively
 */
pair<char, char> pos_to_pair(schar pos){
    char row = pos & 0b1111;

    if(row == 0) return {0, 0};

    return make_pair(pos >> 4, row - 1);
}

/**
 * @brief Converts pos into string format. (Ex: A1)
 * 
 * @param pos value saved in whites or blacks
 * @return string formed from pos
 */
string pos_to_str(schar pos){
    if(pos == 0) return "Out of board";

    pair<char, char> p = pos_to_pair(pos);
    
    string str = "--";
    str[0] = (char) (p.first + 'A');
    str[1] = (char) (p.second + '1');

    return str;
}

void print_piece(Piece piece){
    switch (piece){
        case White:
            cout << 'W';
            break;

        case Black:
            cout << 'B';
            break;
        
        default:
            cout << '-';
            break;
    }
}

/**
 * @brief Prints Board in console.
 */
void print_board(){
    cout << "  ";
    for(char pos = 'A'; pos <= 'H'; pos++){
        cout << ' ' << pos;
    }
    cout << '\n';

    int i = 1;
    for(vector<Piece> row: board){
        cout << i++ << " ";

        for(Piece piece: row){
            cout << ' ';
            
            print_piece(piece);
        }

        cout << '\n';
    }
}

/**
 * @brief Get the whole board.
 * 
 * @return vector<vector<Piece>> with all pieces
 */
vector<vector<Piece>> get_board(){
    return board;
}

/**
 * @brief Get the list of White piece's positions.
 * 
 * @return vector<schar> - White piece's positions.
 */
vector<schar> get_whites(){
    return whites;
}

/**
 * @brief Get the list of Black piece's positions.
 * 
 * @return vector<schar> - Black piece's positions.
 */
vector<schar> get_blacks(){
    return blacks;
}

/**
 * @brief Get the number of points scored by White pieces.
 * 
 * @return int - White points.
 */
int get_w_points(){
    return w_points;
}

/**
 * @brief Get the number of points scored by Black pieces.
 * 
 * @return int - Black points.
 */
int get_b_points(){
    return b_points;
}

/**
 * @brief Checks wether piece in given position can move.
 * 
 * @param pc piece's column in board as int
 * @param pr piece's row in board as int
 * @return Move available to given Piece (first found)
 */
Move check_can_move(char pc, char pr){
    if(pc < 0 || pc >= X_LIM
    || pr < 0 || pr >= Y_LIM) return X_Move;
    
    Piece p = board[pr][pc];

    if(p == White){
        // Can move South
        if(pr == Y_LIM-1 || board[pr+1][pc] == NONE) return South;
        
        // Can move SE
        if(pc < X_LIM-1 && board[pr+1][pc+1] == Black) return SE;

        // Can move SW
        if(pc > 0 && board[pr+1][pc-1] == Black) return SW;
    }
    
    if(p == Black){
        // Can move North
        if(pr == 0 || board[pr-1][pc] == NONE) return North;
        
        // Can move NW
        if(pc > 0 && board[pr-1][pc-1] == White) return NW;
        
        // Can move NE
        if(pc < X_LIM-1 && board[pr-1][pc+1] == White) return NE;
    }

    #ifdef DEBUG_BOARD
    cout << "No piece there\n";
    #endif

    return X_Move;
}

/**
 * @brief Checks wether any Piece can move.
 * 
 * @return int - 0 if none can move,
 *               1 if White can move,
 *               2 if Black can move,
 *               3 if Both can move.
 */
int end_game(){
    int canMove = 0;
    pair<char, char> p;

    for(schar pos: whites){
        p = pos_to_pair(pos);

        if(pos != -1 && check_can_move(p.first, p.second) != X_Move){
            canMove = 1;
            break;
        }
    }

    for(schar pos: blacks){
        p = pos_to_pair(pos);

        if(pos != -1 && check_can_move(p.first, p.second) != X_Move){
            canMove += 2;
            break;
        }
    }

    return canMove;
}

/**
 * @brief Returns type of Piece in given Position.
 * 
 * @param column column in board in format A-H
 * @param row row in board in format 1-8
 * @return Piece in given position
 */
Piece at(char column, char row){
    // To number
    if(column >= 'a') column -= 32;        
    column -= 'A';

    // To number
    row -= '1';

    if(column < 0 || column >= X_LIM || row < 0 || row >= Y_LIM) return X_Piece;

    return board[row][column];
}

/**
 * @brief Find index of Black piece in blacks vector.
 * 
 * @param pc piece's column as int
 * @param pr piece's row as int
 * @return int - index of found Black piece in blacks,
 * or -1 if none was found
 */
int find_black(char pc, char pr){
    if(pc < 0 || pc >= X_LIM
    || pr < 0 || pr >= Y_LIM) return -1;

    schar pos = pair_to_pos(pc, pr);

    if(pos == 0) return -1;

    // Find Black in given position
    for(int i = 0; i < X_LIM; i++){
        if(blacks[i] == pos){
            return i;
        }
    }

    return -1;
}

/**
 * @brief Find index of White piece in whites vector.
 * 
 * @param pc piece's column as int
 * @param pr piece's row as int
 * @return int - index of found White piece in whites,
 * or -1 if none was found
 */
int find_white(char pc, char pr){
    if(pc < 0 || pc >= X_LIM
    || pr < 0 || pr >= Y_LIM) return -1;

    schar pos = pair_to_pos(pc, pr);

    if(pos == 0) return -1;

    // Find White in given position
    for(int i = 0; i < X_LIM; i++){
        if(whites[i] == pos){
            return i;
        }
    }

    return -1;
}

/**
 * @brief Removes Black piece in given position from blacks vector (sets pos to 0) and board.
 * 
 * @param pc piece's column as an int
 * @param pr piece's row as an int
 * @return true if successful,
 * @return false if there is no white piece in given position
 */
bool eat_black(char pc, char pr){
    int b = find_black(pc, pr);

    if(b == -1) return false;

    blacks[b] = 0;
    board[pr][pc] = NONE;

    return true;
}

/**
 * @brief Removes White piece in given position from whites vector (sets pos to 0) and board.
 * 
 * @param pc piece's column as an int
 * @param pr piece's row as an int
 * @return true if successful,
 * @return false if there is no white piece in given position
 */
bool eat_white(char pc, char pr){
    int b = find_white(pc, pr);

    if(b == -1) return false;

    whites[b] = 0;
    board[pr][pc] = NONE;

    return true;
}

/**
 * @brief Moves piece in given position according to given move, if possible.
 * 
 * @param pc piece's column as A-H.
 * @param pr piece's row as 1-8.
 * @param move move to make.
 * @return true if successful.
 * @return false if move is invalid for selected piece.
 */
bool move_piece(char pc, char pr, Move move){
    // To number
    if(pc >= 'a') pc -= 32;
    pc -= 'A';

    pr -= '1';

    if(pc < 0 || pc > X_LIM
    || pr < 0 || pr > Y_LIM) return false;

    //

    Piece piece = board[pr][pc];

    switch(move){
        case NW:
            if(piece != Black || pr <= 0 || pc <= 0 || board[pr-1][pc-1] != White) return false;

            // Remove eaten White
            eat_white(pc-1, pr-1);

            board[pr-1][pc-1] = piece;
            board[pr][pc] = NONE;

            break;
            
        case North:
            if(pr == 0){
                // Remove from Board
                eat_black(pc, pr);
                return true;
            }
            if(piece != Black || pr < 0 || board[pr-1][pc] != NONE) return false;

            board[pr-1][pc] = piece;
            board[pr][pc] = NONE;

            break;

        case NE:
            if(piece != Black || pr <= 0 || pc >= X_LIM-1 || board[pr-1][pc+1] != White) return false;

            // Remove eaten White
            eat_white(pc+1, pr-1);

            board[pr-1][pc+1] = piece;
            board[pr][pc] = NONE;

            break;
            
        case SE:
            if(piece != White || pr >= Y_LIM-1 || pc >= X_LIM-1 || board[pr+1][pc+1] != Black) return false;

            // Remove eaten Black
            eat_black(pc+1, pr+1);

            board[pr+1][pc+1] = piece;
            board[pr][pc] = NONE;

            break;
        
        case South:
            if(pr == X_LIM-1){
                // Remove from board
                eat_white(pc, pr);
                return true;
            }
            if(piece != White || pr > X_LIM-1 || board[pr+1][pc] != NONE) return false;

            board[pr+1][pc] = piece;
            board[pr][pc] = NONE;

            break;
            
        case SW:
            if(piece != White || pr >= Y_LIM-1 || pr <= 0 || board[pr+1][pc-1] != Black) return false;

            // Remove eaten Black
            eat_black(pc-1, pr+1);

            board[pr+1][pc-1] = piece;
            board[pr][pc] = NONE;

            break;
        
        default:
            cout << "Invalid move\n";
            return false;
    }

    return true;
}

/**
 * @brief Moves White piece in i index.
 * 
 * @param i White piece's index in whites vector
 * @param move move to make
 * @return true if successful,
 * @return false if failed to move.
 */
bool move_white(int i, Move move){
    if(i < 0 || i >= X_LIM || whites[i] < 0) return false;

    bool moved = false;
    pair<char, char> pos = pos_to_pair(whites[i]);
    char col = pos.first, row = pos.second;

    #ifdef DEBUG_MOVE
    cout << "W " << i << ": " << pos_to_str(whites[i]) << " " << move_to_str(move) << "\n";
    #endif

    switch(move){
        case SE:
            if(row >= Y_LIM-1 || col >= X_LIM-1 || board[row+1][col+1] != Black) break;

            // Remove eaten Black
            eat_black(col+1, row+1);

            board[row+1][col+1] = White;
            board[row][col] = NONE;

            whites[i]++; // Move in column v
            whites[i] += 0b10000; // Change column >

            moved = true;
            break;

        case South:
            if(row == X_LIM-1){
                // Remove from board
                board[row][col] = NONE;

                whites[i] = -1;

                w_points++;

                moved = true;
                break;
            }
            if(row > X_LIM-1 || board[row+1][col] != NONE) break;

            board[row+1][col] = White;
            board[row][col] = NONE;

            whites[i]++; // Move in column v

            moved = true;
            break;

        case SW:
            if(row >= Y_LIM-1 || row <= 0 || board[row+1][col-1] != Black) break;

            // Remove eaten Black
            eat_black(col-1, row+1);

            board[row+1][col-1] = White;
            board[row][col] = NONE;

            whites[i]++; // Move in column v
            whites[i] -= 0b10000; // Change column <

            moved = true;
            break;

        default:
            break;
    }

    #ifdef DEBUG_MOVE
    if(moved) cout << "Moved to " << pos_to_str(whites[i]) << "\n";
    else cout << "Invalid move\n";
    #endif

    return moved;
}

/**
 * @brief Moves Black piece in i index.
 * 
 * @param i Black piece's index in blacks vector
 * @param move move to make
 * @return true if successful,
 * @return false if failed to move.
 */
bool move_black(int i, Move move){
    if(i < 0 || i >= X_LIM || blacks[i] < 0) return false;

    bool moved = false;
    pair<char, char> pos = pos_to_pair(blacks[i]);
    char col = pos.first, row = pos.second;

    #ifdef DEBUG_MOVE
    cout << "B " << i << ": " << pos_to_str(blacks[i]) << " " << move_to_str(move) << "\n";
    #endif

    switch(move){
        case NW:
            if(row <= 0 || col <= 0 || board[row-1][col-1] != White) break;

            // Remove eaten White
            eat_white(col-1, row-1);

            board[row-1][col-1] = Black;
            board[row][col] = NONE;

            blacks[i]--; // Move in column ^
            blacks[i] -= 0b10000; // Change column <

            moved = true;
            break;
            
        case North:
            if(row == 0){
                // Remove from Board
                board[row][col] = NONE;

                blacks[i] = -1;

                b_points++;
                
                moved = true;
                break;
            }
            if(row < 0 || board[row-1][col] != NONE) break;

            board[row-1][col] = Black;
            board[row][col] = NONE;

            blacks[i]--; // Move in column ^

            moved = true;
            break;

        case NE:
            if(row <= 0 || col >= X_LIM-1 || board[row-1][col+1] != White) break;

            // Remove eaten White
            eat_white(col+1, row-1);

            board[row-1][col+1] = Black;
            board[row][col] = NONE;

            blacks[i]--; // Move in column ^
            blacks[i] += 0b10000; // Change column >

            moved = true;
            break;

        default:
            break;
    }

    #ifdef DEBUG_MOVE
    if(moved) cout << "Moved to " << pos_to_str(blacks[i]) << "\n";
    else cout << "Invalid move\n";
    #endif

    return moved;
}

/**
 * @brief Move a random White piece.
 * 
 * @return true if successful.
 * @return false if no White can be moved.
 */
bool move_any_white(){
    pair<char, char> pos;
    vector<int> movable;
    movable.reserve(X_LIM);

    for(int i = 0; i < X_LIM; i++){
        pos = pos_to_pair(whites[i]);
        if(pos.second != 0 && check_can_move(pos.first, pos.second) != X_Move){
            movable.push_back(i);
        }
    }

    // If no Whites can be moved, exit
    if(movable.size() == 0) return false;

    // Select a random movable White
    int selected = movable[rand()%movable.size()];

    pos = pos_to_pair(whites[selected]);

    Move m = check_can_move(pos.first, pos.second);

    return move_white(selected, m);
}

/**
 * @brief Move a random Black piece.
 * 
 * @return true if successful.
 * @return false if no Black can be moved.
 */
bool move_any_black(){
    pair<char, char> pos;
    vector<int> movable;
    movable.reserve(X_LIM);

    for(int i = 0; i < X_LIM; i++){
        pos = pos_to_pair(blacks[i]);
        if(pos.second != 0 && check_can_move(pos.first, pos.second) != X_Move){
            movable.push_back(i);
        }
    }

    // If no Blacks can be moved, exit
    if(movable.size() == 0) return false;

    // Select a random movable Black
    int selected = movable[rand()%movable.size()];

    pos = pos_to_pair(blacks[selected]);

    Move m = check_can_move(pos.first, pos.second);

    return move_black(selected, m);
}

/**
 * @brief Enact a move made by the player.
 * 
 * @param col moved piece's column in format A-H
 * @param row moved piece's row in format 1-8
 * @param move move to be applied to piece
 * @return true if successful
 * @return false if attempted move is illegal
 */
bool player_move_black(char col, char row, Move move){
    char pc = col, pr = row;
    
    // To number
    if(pc >= 'a') pc -= 32;
    pc -= 'A';

    pr -= '1';

    if(pc < 0 || pc > X_LIM
    || pr < 0 || pr > Y_LIM) return false;


    int b = find_black(pc, pr);
    if(b == -1){
        cout << "No Black piece at " << col << row << "\n";
        return false;
    }

    return move_black(b, move);
}

/**
 * @brief Starts board with initial positions.
 */
void init_board(){
    board = vector<vector<Piece>>(Y_LIM, vector<Piece>(X_LIM, NONE));

    board[1] = vector<Piece>(X_LIM, White);
    board[X_LIM-2] = vector<Piece>(X_LIM, Black);

    whites = vector<schar>(X_LIM);
    blacks = vector<schar>(X_LIM);
    for(int i = 0; i < X_LIM; i++){
        // col row
        whites[i] = pair_to_pos(i, 1);
        blacks[i] = pair_to_pos(i, Y_LIM-2);
    }

    #ifdef DEBUG_INIT_BOARD
    cout << "Whites";
    for(schar pos: whites){
        cout << " " << pos_to_str(pos);
    }
    cout << "\nBlacks";
    for(schar pos: blacks){
        cout << " " << pos_to_str(pos);
    }
    cout << "\n";
    #endif

    w_points = 0;
    b_points = 0;

    srand(time(NULL));
}

// int main(){
//     init_board();

//     Piece p;
//     char column, row;
//     string move;

//     while(true){
//         cin >> column >> row >> move;

//         if(column == '0' || row == '0' || move == "0") break;

//         cout << move_piece(column, row, str_to_move(move)) << '\n';

//         print_board();
//     }

//     return 0;
// }