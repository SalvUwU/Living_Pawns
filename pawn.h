#ifndef PAWN
#define PAWN
#define X_LIM 8
#define Y_LIM 8
#define schar signed char

#include <string>
#include <vector>


using namespace std;

enum Piece {Black = -1, NONE, White, X_Piece};

enum Move {NW, North, NE, SE, South, SW, X_Move};


/**
 * @brief Transforms given Move to corresponding string.
 * 
 * @param move move to transform to string
 * @return string corresponding to given Move
 */
string move_to_str(Move move);

/**
 * @brief Transforms given string to a valid move.
 * 
 * @param str move as string
 * @return Move corresponding to given string
 */
Move str_to_move(string str);

/**
 * @brief Prints Board in console.
 */
void print_board();

/**
 * @brief Get the whole board.
 * 
 * @return vector<vector<Piece>> with all pieces
 */
vector<vector<Piece>> get_board();

/**
 * @brief Get the list of White piece's positions.
 * 
 * @return vector<schar> - White piece's positions.
 */
vector<schar> get_whites();

/**
 * @brief Get the list of Black piece's positions.
 * 
 * @return vector<schar> - Black piece's positions.
 */
vector<schar> get_blacks();

/**
 * @brief Get the number of points scored by White pieces.
 * 
 * @return int - White points.
 */
int get_w_points();

/**
 * @brief Get the number of points scored by Black pieces.
 * 
 * @return int - Black points.
 */
int get_b_points();

/**
 * @brief Checks wether any Piece can move.
 * 
 * @return int - 0 if none can move,
 *               1 if White can move,
 *               2 if Black can move,
 *               3 if Both can move.
 */
int end_game();

/**
 * @brief Moves white piece in i index.
 * 
 * @param i white piece's index in whites vector.
 * @param move move to make.
 * @return true if successful.
 * @return false if failed to move.
 */
bool move_white(int i, Move move);

/**
 * @brief Moves Black piece in i index.
 * 
 * @param i Black piece's index in blacks vector.
 * @param move move to make.
 * @return true if successful.
 * @return false if failed to move.
 */
bool move_black(int i, Move move);

/**
 * @brief Move a random White piece.
 * 
 * @return true if successful.
 * @return false if no White can be moved.
 */
bool move_any_white();

/**
 * @brief Move a random Black piece.
 * 
 * @return true if successful.
 * @return false if no Black can be moved.
 */
bool move_any_black();

/**
 * @brief Enact a move made by the player.
 * 
 * @param col moved piece's column in format A-H
 * @param row moved piece's row in format 1-8
 * @param move move to be applied to piece
 * @return true if successful
 * @return false if attempted move is illegal
 */
bool player_move_black(char col, char row, Move move);

/**
 * @brief Starts board with initial positions.
 */
void init_board();

#endif