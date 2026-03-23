/**
 * Splashmem IA - Aggressive Player Bot
 * Bot qui utilise principalement DASH pour couvrir plus de terrain
 */

#include "actions.h"
#include <stdlib.h>

/* Compteur pour alterner les directions */
static int move_counter = 0;

/* Liste des actions DASH et MOVE */
static const char DASH_ACTIONS[] = {ACTION_DASH_U, ACTION_DASH_D, ACTION_DASH_L, ACTION_DASH_R};

static const char MOVE_ACTIONS[] = {ACTION_MOVE_U, ACTION_MOVE_D, ACTION_MOVE_L, ACTION_MOVE_R};

/**
 * Fonction exportée - Stratégie agressive
 * Alterne entre DASH et MOVE pour maximiser la couverture
 */
char get_action(void) {
  move_counter++;

  /* 70% DASH, 30% MOVE pour économiser parfois les crédits */
  if (rand() % 10 < 7) {
    /* DASH dans une direction aléatoire */
    return DASH_ACTIONS[rand() % 4];
  } else {
    /* MOVE pour économiser les crédits */
    return MOVE_ACTIONS[rand() % 4];
  }
}
