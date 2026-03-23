/**
 * Splashmem IA - Random Player Bot
 * Bot qui joue aléatoirement parmi toutes les actions possibles
 */

#include "actions.h"
#include <stdlib.h>

/* Liste de toutes les actions possibles */
static const char ACTIONS[] = {
    ACTION_MOVE_U,       ACTION_MOVE_D,      ACTION_MOVE_L,  ACTION_MOVE_R,  ACTION_DASH_U,
    ACTION_DASH_D,     ACTION_DASH_L,      ACTION_DASH_R, ACTION_TELEPORT_U, ACTION_TELEPORT_D,
    ACTION_TELEPORT_L, ACTION_TELEPORT_R, ACTION_STILL};

#define NUM_ACTIONS (sizeof(ACTIONS) / sizeof(ACTIONS[0]))

/**
 * Fonction exportée - Retourne une action aléatoire
 */
char get_action(void) { return ACTIONS[rand() % NUM_ACTIONS]; }
