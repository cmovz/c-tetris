#include "../ai.h"
#include "../grid.h"
#include "mt19937-64.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/random.h>

#define POPULATION_SIZE 100
#define ELITE_SIZE 10
#define MUTANT_SIZE 20
#define ELITE_CHANCES 0.70f
#define GAME_COUNT 50

struct individual {
  int fitness;
  float weights[7];
};

int cmp_individual_desc(const void *aptr, const void *bptr)
{
  const struct individual *a = aptr, *b = bptr;
  if (a->fitness < b->fitness)
    return 1;
  if (a->fitness > b->fitness)
    return -1;
  return 0;
}

int cmp_int(const void *aptr, const void *bptr)
{
  const int *a = aptr, *b = bptr;
  if (*a < *b)
    return -1;
  if (*a > *b)
    return 1;
  return 0;
}

int main(int argc, char *argv[])
{
  // init population with random weights
  unsigned long long mt_seed;
  if (getrandom(&mt_seed, sizeof mt_seed, 0) != sizeof mt_seed) {
    perror("getrandom()");
    return EXIT_FAILURE;
  }
  init_genrand64(mt_seed);

  struct individual population[POPULATION_SIZE];
  for (size_t i = 0; i < POPULATION_SIZE; ++i) {
    for (size_t j = 0; j < 7; ++j) {
      population[i].weights[j] = genrand64_real1();
    }
  }
  // seed an individual with the best weights from previous run
  population[0].weights[0] = 0.11146253876595202f;
  population[0].weights[1] = 0.12392743901965475f;
  population[0].weights[2] = 0.029653844701920895f;
  population[0].weights[3] = 0.9624658982587024f;
  population[0].weights[4] = 0.32341412515044743f;
  population[0].weights[5] = 0.0f;
  population[0].weights[6] = 0.0f;

  size_t generation = 0;
  while (1) {
    ++generation;

    // evolve population
    if (getrandom(&mt_seed, sizeof mt_seed, 0) != sizeof mt_seed) {
      perror("getrandom()");
      return EXIT_FAILURE;
    }
    init_genrand64(mt_seed);

    struct individual temp_pool[POPULATION_SIZE - ELITE_SIZE - MUTANT_SIZE];
    size_t mating_count = POPULATION_SIZE - ELITE_SIZE - MUTANT_SIZE;
    while (mating_count--) {
      struct individual *elite_individual = 
        population + (genrand64_int64() % ELITE_SIZE);
      struct individual *random_individual = population 
        + ELITE_SIZE
        + (genrand64_int64() % (POPULATION_SIZE - ELITE_SIZE));
      struct individual *new_individual = temp_pool + mating_count;

      for (int i = 0; i < 7; ++i) {
        if (genrand64_real1() < ELITE_CHANCES)
          new_individual->weights[i] = elite_individual->weights[i];
        else
          new_individual->weights[i] = random_individual->weights[i];
      }
    }

    // replace old population with new individuals
    memcpy(population + ELITE_SIZE, temp_pool, sizeof temp_pool);

    // fill new mutants
    for (size_t i = 0; i < MUTANT_SIZE; ++i) {
      for (size_t j = 0; j < 7; ++j) {
        population[POPULATION_SIZE - i - 1].weights[j] = genrand64_real1();
      }
    }

    // compute new fitnesses
    if (getrandom(&mt_seed, sizeof mt_seed, 0) != sizeof mt_seed) {
      perror("getrandom()");
      return EXIT_FAILURE;
    }

    for (size_t i = 0; i < POPULATION_SIZE; ++i) {
      init_genrand64(mt_seed);

      int scores[GAME_COUNT];
      struct individual *individual = population + i;
      struct simple_ai sai;
      simple_ai_init(
        &sai,
        individual->weights[0],
        individual->weights[1],
        individual->weights[2],
        individual->weights[3],
        individual->weights[4],
        individual->weights[5],
        individual->weights[6]
      );

      for (size_t j = 0; j < GAME_COUNT; ++j) {
        struct dense_grid dg;
        int score = 0;
        dense_grid_init(&dg);

        dense_grid_add_piece(&dg, pieces[genrand64_int64() % 7], 6, 1);
        while (1) {
          simple_ai_run(&sai, &dg);
          simple_ai_adjust_position_virtual(&sai, &dg);
          while (dense_grid_move_piece(&dg, 0, 1)){
          }
          dense_grid_move_piece_no_check(&dg, 0, -1);
          score += dense_grid_integrate_piece_fast(&dg);
          if (!dense_grid_add_piece(&dg, pieces[genrand64_int64() % 7], 6, 1))
            break;
        }

        scores[j] = score;
      }

      qsort(scores, GAME_COUNT, sizeof(int), cmp_int);
      size_t median_idx0 = (GAME_COUNT - 1) / 2;
      size_t median_idx1 = GAME_COUNT / 2;
      individual->fitness = (scores[median_idx0] + scores[median_idx1]) / 2;
    }

    qsort(
      population, POPULATION_SIZE, sizeof population[0], cmp_individual_desc
    );

    printf(
      "-------------------------------------------\n"
      "Generation %zu\n"
      "Median fitness: %d\n",
      generation,
      population[0].fitness
    );
    for (int i = 0; i < 7; ++i) {
      printf("#define %c %ff\n", 'A' + i, population[0].weights[i]);
    }
    putchar('\n');
    fflush(stdout);
  }

  return 0;
}