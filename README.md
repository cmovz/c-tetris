# c-tetris
Tetris clone written in C with simple AI that can play the game.

# running
1. Download the code
2. Compile by running `bash compile.sh`
3. Run `./tetris` or `./tetris --ai`

# how it works
- The bot checks all possible movements, then, for each movement, it checks what the next states will be if each of the 7 pieces fall, it repeats this 3 levels deep and assigns a quality score for each state - using the median value as the final score. Then it chooses the highest rated position.
- The bot doesn't know in advance what the next piece will be, that's why it checks every possible combination.
- For best performance, the grid state is stored as an array of bits. The first bit corresponding to the x=0, y=0 position. This allows multiple cells to be manipulated with fewer instructions.
