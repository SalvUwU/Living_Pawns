# Living_Pawns

This project contains code for a game based on chess containing only pawns.
This game was made for "The 6 Heroes Festival", a DnD (inspired) Campaign created by this project's author.

## How to play

### Opponents

Inside the folder AI, there should be various files named player1.ai, player2.ai, etc.
These files contain opponents to choose from.

### Start a game

To start a game, run the command:
    ./star play "execution"

The execution is the number of the opponent (for player1.ai, execution would be 1).

### During a game

The opponent will always take the first turn.
After this, you will play turns in order Player->Opponent->Player->Opponent->...
(This rule can be broken if either the player or the opponent cannot move any pieces during their turn)

### Taking a turn

Before your turn, a representation of the board will be shown.
To take a turn, you must specify which pawn you wish to move through its position and the move you want it to make.
Example: A7 N
(This would make piece in position A7 move North)

### End of the game

The game will end when neither the player nor the opponent can take a turn.
When this happens, the winner will be shown.

You can give up at any given time, to do so, insert 0 as one of the fields of your action.

### Scoring

To score a point, you must "walk" a pawn out of the board.