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

!! Warn: Currently only works on POSIX compliant platforms, since the underlaying inter-process communication library doesn't support windows.
TODO: add CMake stuff for easier build instructions.
for now, there isn't much linking or other build-fu techniques.
If you know what's what you should be able to compile it no problem (it's just a bunch of source files). (C++20)

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



