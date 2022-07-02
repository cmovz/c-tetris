gcc -o find_weights -std=c99 -flto -O3 -Wall -Wno-switch \
  main.c mt19937-64.c ../ai.c ../grid.c ../pieces.c