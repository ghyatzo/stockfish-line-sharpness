# Line Sharpness
Have you ever got annoyed that in a certain position an engine smuggly says "yeah, this is completely fine" but every move in the position EXCEPT one are losing?
This tool tries to give a metric to measure how sharp a position (or line) is, by comparing the number of bad moves/blunder to best/ok moves there are.

## How it works
This tool launches an engine underneath, and then communicates with it, asking to evaluate positions and extracting the information.
There is no logic for evaluating a position within this source code.
This is why you have to pass an engine executable to the tool. For example, if I had stockfish in my path i could run
```
line_sharpness -e $(which stockfish)
```
to calculate the sharpness of the starting position. It needs an absolute path.

!! Warn: Currently only works on POSIX compliant platforms, since the underlaying inter-process communication library doesn't support windows (yet, i'm working on it).
TODO: add CMake stuff for easier build instructions.
for now, there isn't much linking or other build-fu techniques.
If you know what's what you should be able to compile it no problem (it's just a bunch of source files). (C++20)

## Example
Exploring the sharpness of the bayonette attack in the Caro-Kan defense, advance variation:
```
$ ./line_sharpness -d 17 -e $(which stockfish) -a e4 c6 d4 d5 e5 Bf5 g4 Bg6 h4 h5
Starting process with PID: 17336

 +---+---+---+---+---+---+---+---+
 | r | n |   | q | k | b | n | r | 8
 +---+---+---+---+---+---+---+---+
 | p | p |   |   | p | p | p |   | 7
 +---+---+---+---+---+---+---+---+
 |   |   | p |   |   |   | b |   | 6
 +---+---+---+---+---+---+---+---+
 |   |   |   | p | P |   |   | p | 5
 +---+---+---+---+---+---+---+---+
 |   |   |   | P |   |   | P | P | 4
 +---+---+---+---+---+---+---+---+
 |   |   |   |   |   |   |   |   | 3
 +---+---+---+---+---+---+---+---+
 | P | P | P |   |   | P |   |   | 2
 +---+---+---+---+---+---+---+---+
 | R | N | B | Q | K | B | N | R | 1
 +---+---+---+---+---+---+---+---+
   a   b   c   d   e   f   g   h

Fen: rn1qkbnr/pp2ppp1/2p3b1/3pP2p/3P2PP/8/PPP2P2/RNBQKBNR w KQkq - 0 6
Eval: 0 (depth: 17)
In This position there are 37 possible moves. White to move
[ooooooooooooooooooooooooooooooooooooo]       
Bad moves: 24 (loss >= 1.2)
Ok moves: 2 (loss < 0.5)
not optimal, but not terrible: 11
Sharpness ratio: 24/(2+24) = 0.923077
Good Moves: 
[-0]	 e6 (e5e6)
[-0.33]	 gxh5 (g4h5)
```
This is a very sharp line that white can take against the Caro-Kann. This position is reached after 
`1.e4 c6 2.d4 d5 3.e5 Bf5 4.g4?! Bg6?! 5.h4?! h5!`.
A move ago, after white played `h4` black has a similar scenario, where `h5` is the only move that keep black in the game.
After `..h5` by black, now it's white turn. As all sharp lines, it is a double-edged sword.
To stay in the game white has to play the critical move `e6` or the slightly worse `gxh5`.

## Usage
```
Usage is: line_sharpness -e <engine path> [-d <depth>] [-f '<FEN string>'] [-l] [-a] [<moves>...] 
	 -h prints this message
	 -e <path> the path must be absolute
	 -d <int> default = 15
	 -f '<FEN string>' default = starpos (use quotes)
	 -l eval line flag
	 -a short algebraic flag
	 <moves>... set of moves relative to the position ( long algebraic notation )
The -a flag tells the program to interpret the moves with the short algebraic notation (Nf3, Bg5, ...),
otherwise it will interpret the moves to be in long algebraic notation (e2e4, g6g8, ...)
If the -l flag is not set, only the end position given by <FEN> + <moves> is analysed.
If the -l flag is passed, the whole line is analysed. There must be at least 1 move.
If not the -l flag does nothing. (I suggest you evaluate lines on a low depth, lest you like to watch paint dry)
```

Given a position, it explores all possible legal moves in that position and evaluates the resulting position, 
it then checks which moves incurred in a high enough evaluation drop and marks them as bad.
Otherwise, moves that keep the evaluation stable are marked as good.
It then gives a ratio of how many bad moves there are compared to the good ones.

When computing whole lines, a relatively low depth is suggested (15-17). For each move in the line the engine has to evaluate 30ish positions.
At around depth 17 the evaluation time is about 1 second, and it grows considerably at higher depths.



