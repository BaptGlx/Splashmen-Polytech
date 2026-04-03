/**
 * Splashmem IA - Actions Header
 * Définition des codes d'action pour les joueurs
 */

#ifndef ACTIONS_H
#define ACTIONS_H

enum action {
    ACTION_STILL, // 0
    ACTION_MOVE_L, // 1
    ACTION_MOVE_R, // 2
    ACTION_MOVE_U, // 3
    ACTION_MOVE_D,
    ACTION_DASH_L,
    ACTION_DASH_R,
    ACTION_DASH_U,
    ACTION_DASH_D,
    ACTION_TELEPORT_L,
    ACTION_TELEPORT_R,
    ACTION_TELEPORT_U,
    ACTION_TELEPORT_D,
    ACTION_BOMB, // 13
    ACTION_FORK, // 14
    ACTION_CLEAN, // 15
    ACTION_MUTE, // 16
    ACTION_SWAP, // 17
    ACTION_NUMBER
};

/* ============================================
 * Constantes de jeu
 * ============================================ */
#define GRID_WIDTH      100
#define GRID_HEIGHT     100
#define STARTING_CREDITS 9000
#define MAX_PLAYERS     4

/* Coûts des actions */
#define COST_MOVE       1
#define COST_DASH       10
#define COST_TELEPORT   2
#define COST_BOMB       9
#define COST_CLEAN      40
#define COST_MUTE       30
#define COST_SWAP       35
#define COST_STILL      1

/* Distance de déplacement */
#define DASH_DISTANCE       8
#define TELEPORT_DISTANCE   8

#endif /* ACTIONS_H */
