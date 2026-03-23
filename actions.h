/**
 * Splashmem IA - Actions Header
 * Définition des codes d'action pour les joueurs
 */

#ifndef ACTIONS_H
#define ACTIONS_H

/* ============================================
 * Actions de déplacement simple (1 case)
 * Coût: 1 crédit
 * ============================================ */
#define ACTION_MOVE_U     0x01
#define ACTION_MOVE_D     0x02
#define ACTION_MOVE_L     0x03
#define ACTION_MOVE_R     0x04

/* ============================================
 * Actions DASH (8 cases)
 * Coût: 10 crédits
 * Colore toutes les cases du chemin et la case d'arrivée
 * ============================================ */
#define ACTION_DASH_U     0x11
#define ACTION_DASH_D     0x12
#define ACTION_DASH_L     0x13
#define ACTION_DASH_R     0x14

/* ============================================
 * Actions TELEPORT (8 cases)
 * Coût: 2 crédits
 * Seule la case d'arrivée est colorée
 * ============================================ */
#define ACTION_TELEPORT_U     0x21
#define ACTION_TELEPORT_D     0x22
#define ACTION_TELEPORT_L     0x23
#define ACTION_TELEPORT_R     0x24

/* ============================================
 * Action STILL (reste sur place)
 * Coût: 1 crédit
 * ============================================ */
#define ACTION_STILL   0x00

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
#define COST_STILL      1

/* Distance de déplacement */
#define DASH_DISTANCE       8
#define TELEPORT_DISTANCE   8

#endif /* ACTIONS_H */
