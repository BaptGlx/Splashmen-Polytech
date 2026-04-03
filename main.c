/**
 * Splashmem IA - Main Game Engine
 * Moteur de jeu avec rendu SDL2 et chargement dynamique des joueurs
 */

#include "actions.h"
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================
 * Configuration du rendu
 * ============================================ */
#define CELL_SIZE 7
#define WINDOW_WIDTH (GRID_WIDTH * CELL_SIZE)
#define WINDOW_HEIGHT (GRID_HEIGHT * CELL_SIZE)
#define GAME_SPEED_MS 5 /* Délai entre chaque tour en ms */

/* ============================================
 * Couleurs des joueurs (RGBA)
 * ============================================ */
static const SDL_Color PLAYER_COLORS[MAX_PLAYERS] = {
    {255, 50, 50, 255},  /* Rouge vif */
    {50, 150, 255, 255}, /* Bleu clair */
    {50, 255, 100, 255}, /* Vert lime */
    {255, 200, 50, 255}  /* Jaune doré */
};

static const SDL_Color EMPTY_COLOR = {30, 30, 40, 255}; /* Gris foncé */
static const SDL_Color PLAYER_MARKER = {
    255, 255, 255, 255}; /* Blanc pour marquer les joueurs */

/* ============================================
 * Structure d'un joueur
 * ============================================ */
typedef struct {
  void *handle;             /* Handle de la bibliothèque .so */
  enum action (*get_action)(void); /* Pointeur vers la fonction get_action */
  int x, y;                 /* Position actuelle */
  int credits;              /* Crédits restants */
  int score;                /* Nombre de cases colorées */
  SDL_Color color;          /* Couleur du joueur */
  char name[256];           /* Nom du fichier .so */

  /* Champs pour le parseur .txt */
  int is_txt;
  enum action *txt_actions;
  int num_txt_actions;
  int txt_action_idx;

  /* Timers et effets état */
  int mute_timer;
  int swap_timer;
  int swap_beneficiary;
  
  int fork_timer;
  int fork_active_timer;
  int fork_x, fork_y;
  
  int bomb_timer;
  int bomb_x, bomb_y;
} Player;

/* ============================================
 * État du jeu
 * ============================================ */
typedef struct {
  int grid[GRID_HEIGHT][GRID_WIDTH]; /* -1 = vide, 0-3 = joueur */
  Player players[MAX_PLAYERS];
  int num_players;
  int game_over;
  SDL_Window *window;
  SDL_Renderer *renderer;
} GameState;

/* ============================================
 * Fonctions utilitaires
 * ============================================ */

/* Wrap-around torique pour les coordonnées */
static int wrap(int coord, int max) {
  coord = coord % max;
  if (coord < 0)
    coord += max;
  return coord;
}

/* Calcule le coût d'une action */
static int get_action_cost(enum action action) {
  switch (action) {
  case ACTION_MOVE_U:
  case ACTION_MOVE_D:
  case ACTION_MOVE_L:
  case ACTION_MOVE_R:
    return COST_MOVE;

  case ACTION_DASH_U:
  case ACTION_DASH_D:
  case ACTION_DASH_L:
  case ACTION_DASH_R:
    return COST_DASH;

  case ACTION_TELEPORT_U:
  case ACTION_TELEPORT_D:
  case ACTION_TELEPORT_L:
  case ACTION_TELEPORT_R:
    return COST_TELEPORT;

  case ACTION_BOMB: return COST_BOMB;
  case ACTION_CLEAN: return COST_CLEAN;
  case ACTION_MUTE: return COST_MUTE;
  case ACTION_SWAP: return COST_SWAP;
  case ACTION_FORK: return 0; /* Le coût doubleur de FORK est traité à la volée */

  case ACTION_STILL:
  default:
    return COST_STILL;
  }
}

/* Calcule le déplacement (dx, dy) pour une action */
static void get_movement(enum action action, int *dx, int *dy) {
  *dx = 0;
  *dy = 0;

  int distance = 1;

  /* Détermine la distance */
  if ((action >= ACTION_DASH_L && action <= ACTION_DASH_D) ||
      (action >= ACTION_TELEPORT_L && action <= ACTION_TELEPORT_D)) {
    distance = DASH_DISTANCE;
  }

  /* Détermine la direction */
  switch (action) {
  case ACTION_MOVE_U:
  case ACTION_DASH_U:
  case ACTION_TELEPORT_U:
    *dy = -distance;
    break;
  case ACTION_MOVE_D:
  case ACTION_DASH_D:
  case ACTION_TELEPORT_D:
    *dy = distance;
    break;
  case ACTION_MOVE_L:
  case ACTION_DASH_L:
  case ACTION_TELEPORT_L:
    *dx = -distance;
    break;
  case ACTION_MOVE_R:
  case ACTION_DASH_R:
  case ACTION_TELEPORT_R:
    *dx = distance;
    break;
  default:
    break; /* ACTION_STILL, ou nouvelle action non-mouvement */
  }
}

/* Parsing d'une chaîne d'action en enum action */
static enum action parse_action_string(char *str) {
    /* Nettoyage espaces / retours chariot en fin */
    char *end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
    /* Saut espaces en début */
    while(*str == ' ') str++;

    if (strcmp(str, "ACTION_MOVE_L") == 0) return ACTION_MOVE_L;
    if (strcmp(str, "ACTION_MOVE_R") == 0) return ACTION_MOVE_R;
    if (strcmp(str, "ACTION_MOVE_U") == 0) return ACTION_MOVE_U;
    if (strcmp(str, "ACTION_MOVE_D") == 0) return ACTION_MOVE_D;
    if (strcmp(str, "ACTION_DASH_L") == 0) return ACTION_DASH_L;
    if (strcmp(str, "ACTION_DASH_R") == 0) return ACTION_DASH_R;
    if (strcmp(str, "ACTION_DASH_U") == 0) return ACTION_DASH_U;
    if (strcmp(str, "ACTION_DASH_D") == 0) return ACTION_DASH_D;
    if (strcmp(str, "ACTION_TELEPORT_L") == 0) return ACTION_TELEPORT_L;
    if (strcmp(str, "ACTION_TELEPORT_R") == 0) return ACTION_TELEPORT_R;
    if (strcmp(str, "ACTION_TELEPORT_U") == 0) return ACTION_TELEPORT_U;
    if (strcmp(str, "ACTION_TELEPORT_D") == 0) return ACTION_TELEPORT_D;
    if (strcmp(str, "ACTION_BOMB") == 0) return ACTION_BOMB;
    if (strcmp(str, "ACTION_FORK") == 0) return ACTION_FORK;
    if (strcmp(str, "ACTION_CLEAN") == 0) return ACTION_CLEAN;
    if (strcmp(str, "ACTION_MUTE") == 0) return ACTION_MUTE;
    if (strcmp(str, "ACTION_SWAP") == 0) return ACTION_SWAP;
    return ACTION_STILL;
}

/* Distance de Manhattan avec wrap-around torique */
static int manhattan_toric(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    if (dx > GRID_WIDTH / 2) dx = GRID_WIDTH - dx;
    int dy = abs(y1 - y2);
    if (dy > GRID_HEIGHT / 2) dy = GRID_HEIGHT - dy;
    return dx + dy;
}

/* ============================================
 * Chargement des joueurs
 * ============================================ */
static int load_player(GameState *game, const char *path, int player_id) {
  Player *p = &game->players[player_id];
  memset(p, 0, sizeof(Player));

  strncpy(p->name, path, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  p->credits = STARTING_CREDITS;
  p->score = 0;
  p->color = PLAYER_COLORS[player_id];

  size_t path_len = strlen(path);
  if (path_len >= 4 && strcmp(path + path_len - 4, ".txt") == 0) {
    p->is_txt = 1;
    FILE *f = fopen(path, "r");
    if (!f) {
      fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier texte '%s'\n", path);
      return -1;
    }
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), f)) {
      int count = 1;
      for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ',') count++;
      }
      p->txt_actions = malloc(sizeof(enum action) * count);
      p->num_txt_actions = 0;
      char *token = strtok(buffer, ",");
      while (token) {
        p->txt_actions[p->num_txt_actions++] = parse_action_string(token);
        token = strtok(NULL, ",");
      }
    }
    fclose(f);
    if (p->num_txt_actions == 0) {
      fprintf(stderr, "Erreur: Fichier txt '%s' sans actions valides\n", path);
      return -1;
    }
  } else {
    p->is_txt = 0;
    char safe_path[512];
    /* Si le chemin ne contient pas de '/', on ajoute './' pour forcer la recherche locale (fix pour Linux) */
    if (strchr(path, '/') == NULL) {
      snprintf(safe_path, sizeof(safe_path), "./%s", path);
    } else {
      strncpy(safe_path, path, sizeof(safe_path) - 1);
      safe_path[sizeof(safe_path) - 1] = '\0';
    }

    /* Ouvrir la bibliothèque dynamique */
    p->handle = dlopen(safe_path, RTLD_NOW);
    if (!p->handle) {
      fprintf(stderr, "Erreur: Impossible de charger '%s': %s\n", safe_path,
              dlerror());
      return -1;
    }

    /* Récupérer la fonction get_action */
    dlerror(); /* Clear any existing error */
    p->get_action = (enum action (*)(void))dlsym(p->handle, "get_action");
    char *error = dlerror();
    if (error != NULL) {
      fprintf(stderr, "Erreur: 'get_action' non trouvée dans '%s': %s\n", safe_path,
              error);
      dlclose(p->handle);
      return -1;
    }
  }

  /* Position de départ avec espacement égal */
  switch (player_id) {
  case 0:
    p->x = GRID_WIDTH / 4;
    p->y = GRID_HEIGHT / 4;
    break;
  case 1:
    p->x = 3 * GRID_WIDTH / 4;
    p->y = GRID_HEIGHT / 4;
    break;
  case 2:
    p->x = GRID_WIDTH / 4;
    p->y = 3 * GRID_HEIGHT / 4;
    break;
  case 3:
    p->x = 3 * GRID_WIDTH / 4;
    p->y = 3 * GRID_HEIGHT / 4;
    break;
  }

  /* Colorier la case de départ */
  game->grid[p->y][p->x] = player_id;
  p->score = 1;

  printf("Joueur %d chargé: %s (position: %d,%d)\n", player_id + 1, path,
         p->x, p->y);
  return 0;
}

/* ============================================
 * Initialisation du jeu
 * ============================================ */
static int init_game(GameState *game, int argc, char **argv) {
  memset(game, 0, sizeof(GameState));

  /* Initialiser la grille à vide (-1) */
  for (int y = 0; y < GRID_HEIGHT; y++) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      game->grid[y][x] = -1;
    }
  }

  /* Vérifier le nombre d'arguments */
  if (argc < 2) {
    fprintf(stderr,
            "Usage: %s player1.so [player2.so] [player3.so] [player4.so]\n",
            argv[0]);
    return -1;
  }

  game->num_players = (argc - 1 > MAX_PLAYERS) ? MAX_PLAYERS : argc - 1;

  /* Charger les joueurs */
  for (int i = 0; i < game->num_players; i++) {
    if (load_player(game, argv[i + 1], i) < 0) {
      return -1;
    }
  }

  /* Initialiser SDL2 */
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Erreur SDL_Init: %s\n", SDL_GetError());
    return -1;
  }

  game->window = SDL_CreateWindow("Splashmem IA", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                                  WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

  if (!game->window) {
    fprintf(stderr, "Erreur SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  game->renderer =
      SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
  if (!game->renderer) {
    fprintf(stderr, "Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(game->window);
    SDL_Quit();
    return -1;
  }

  return 0;
}

/* ============================================
 * Nettoyage
 * ============================================ */
static void cleanup_game(GameState *game) {
  /* Décharger les bibliothèques et clean txt array */
  for (int i = 0; i < game->num_players; i++) {
    if (game->players[i].handle) {
      dlclose(game->players[i].handle);
    }
    if (game->players[i].txt_actions) {
      free(game->players[i].txt_actions);
    }
  }

  /* Nettoyer SDL */
  if (game->renderer)
    SDL_DestroyRenderer(game->renderer);
  if (game->window)
    SDL_DestroyWindow(game->window);
  SDL_Quit();
}

/* ============================================
 * Mise à jour d'un joueur
 * ============================================ */
static void update_player(GameState *game, int player_id) {
  Player *p = &game->players[player_id];

  if (p->credits <= 0) return;

  /* Gestion des timers au début du tour */
  if (p->mute_timer > 0) p->mute_timer--;
  if (p->swap_timer > 0) p->swap_timer--;
  
  if (p->fork_timer > 0) {
      p->fork_timer--;
      if (p->fork_timer == 0) {
          p->fork_active_timer = 20;
      }
  } else if (p->fork_active_timer > 0) {
      p->fork_active_timer--;
  }

  if (p->bomb_timer > 0) {
      p->bomb_timer--;
      if (p->bomb_timer == 0) {
          /* Explosion 3x3 indépendante des malus actuels */
          for (int dy = -1; dy <= 1; dy++) {
              for (int dx = -1; dx <= 1; dx++) {
                  int ex = wrap(p->bomb_x + dx, GRID_WIDTH);
                  int ey = wrap(p->bomb_y + dy, GRID_HEIGHT);
                  if (game->grid[ey][ex] != player_id) {
                      int old_owner = game->grid[ey][ex];
                      if (old_owner >= 0 && old_owner < game->num_players) {
                          game->players[old_owner].score--;
                      }
                      game->grid[ey][ex] = player_id;
                      p->score++;
                  }
              }
          }
      }
  }

  /* Demander l'action au joueur */
  enum action action = ACTION_STILL;
  if (p->is_txt) {
      action = p->txt_actions[p->txt_action_idx];
      p->txt_action_idx = (p->txt_action_idx + 1) % p->num_txt_actions;
  } else {
      action = p->get_action();
  }

  int cost = get_action_cost(action);

  /* Majoration du coût par FORK */
  if (p->fork_active_timer > 0) {
      cost *= 2;
  }

  if (cost > p->credits) {
      action = ACTION_STILL;
      cost = COST_STILL;
      if (p->fork_active_timer > 0) cost *= 2; /* Même STILL coûte double si on l'applique ? "tous les coûts". Oui. */
      /* Si même le still majoré est trop cher, on limite */
      if (cost > p->credits) {
        cost = p->credits; /* Consomme le reste mais pas de négatif */
      }
  }

  p->credits -= cost;

  /* Exécution des actions uniques/spéciales pour le maître */
  if (action == ACTION_BOMB) {
      p->bomb_timer = 5;
      p->bomb_x = p->x;
      p->bomb_y = p->y;
  } else if (action == ACTION_FORK) {
      p->fork_timer = 5;
      p->fork_active_timer = 0; /* destruction de l'ancien */
      p->fork_x = p->x;
      p->fork_y = p->y;
  } else if (action == ACTION_CLEAN) {
      for (int dy = -3; dy <= 3; dy++) {
          for (int dx = -3; dx <= 3; dx++) {
              int cx = wrap(p->x + dx, GRID_WIDTH);
              int cy = wrap(p->y + dy, GRID_HEIGHT);
              int old_owner = game->grid[cy][cx];
              if (old_owner >= 0 && old_owner < game->num_players) {
                  game->players[old_owner].score--;
              }
              game->grid[cy][cx] = -1;
          }
      }
  } else if (action == ACTION_MUTE || action == ACTION_SWAP) {
      int closest_id = -1;
      int min_dist = 999999;
      for (int i = 0; i < game->num_players; i++) {
          if (i == player_id || game->players[i].credits <= 0) continue;
          int dist = manhattan_toric(p->x, p->y, game->players[i].x, game->players[i].y);
          if (dist < min_dist) {
              min_dist = dist;
              closest_id = i;
          }
      }
      if (closest_id != -1) {
          if (action == ACTION_MUTE) {
              game->players[closest_id].mute_timer = 10;
              game->players[closest_id].swap_timer = 0;
          } else {
              game->players[closest_id].swap_timer = 5;
              game->players[closest_id].mute_timer = 0;
              game->players[closest_id].swap_beneficiary = player_id;
          }
      }
  }

  /* Exécution pour le Clone des actions spéciales applicables (Clean, Mute, Swap) */
  if (p->fork_active_timer > 0) {
      if (action == ACTION_CLEAN) {
          for (int dy = -3; dy <= 3; dy++) {
              for (int dx = -3; dx <= 3; dx++) {
                  int cx = wrap(p->fork_x + dx, GRID_WIDTH);
                  int cy = wrap(p->fork_y + dy, GRID_HEIGHT);
                  int old_owner = game->grid[cy][cx];
                  if (old_owner >= 0 && old_owner < game->num_players) {
                      game->players[old_owner].score--;
                  }
                  game->grid[cy][cx] = -1;
              }
          }
      } else if (action == ACTION_MUTE || action == ACTION_SWAP) {
          int closest_id = -1;
          int min_dist = 999999;
          for (int i = 0; i < game->num_players; i++) {
              if (i == player_id || game->players[i].credits <= 0) continue;
              int dist = manhattan_toric(p->fork_x, p->fork_y, game->players[i].x, game->players[i].y);
              if (dist < min_dist) {
                  min_dist = dist;
                  closest_id = i;
              }
          }
          if (closest_id != -1) {
              if (action == ACTION_MUTE) {
                  game->players[closest_id].mute_timer = 10;
                  game->players[closest_id].swap_timer = 0;
              } else {
                  game->players[closest_id].swap_timer = 5;
                  game->players[closest_id].mute_timer = 0;
                  game->players[closest_id].swap_beneficiary = player_id; /* bénéficie toujours au joueur d'origine */
              }
          }
      }
  }

  /* Couleurs de déplacement */
  int color_id = player_id;
  if (p->mute_timer > 0) color_id = -1;
  else if (p->swap_timer > 0) color_id = p->swap_beneficiary;

  int dx, dy;
  get_movement(action, &dx, &dy);

  int is_dash = (action >= ACTION_DASH_L && action <= ACTION_DASH_D);
  int is_move = (action >= ACTION_MOVE_L && action <= ACTION_MOVE_D);

  /* Appliquer le déplacement pour le maître */
  if (is_dash || is_move) {
      int step_x = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
      int step_y = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
      int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
      for (int s = 0; s < steps; s++) {
          p->x = wrap(p->x + step_x, GRID_WIDTH);
          p->y = wrap(p->y + step_y, GRID_HEIGHT);
          if (game->grid[p->y][p->x] != color_id) {
              int old_owner = game->grid[p->y][p->x];
              if (old_owner >= 0 && old_owner < game->num_players) {
                  game->players[old_owner].score--;
              }
              game->grid[p->y][p->x] = color_id;
              if (color_id >= 0 && color_id < game->num_players) {
                  game->players[color_id].score++;
              }
          }
      }
  } else if ((action >= ACTION_TELEPORT_L && action <= ACTION_TELEPORT_D) || action == ACTION_STILL) {
      p->x = wrap(p->x + dx, GRID_WIDTH);
      p->y = wrap(p->y + dy, GRID_HEIGHT);
      if (game->grid[p->y][p->x] != color_id) {
          int old_owner = game->grid[p->y][p->x];
          if (old_owner >= 0 && old_owner < game->num_players) {
              game->players[old_owner].score--;
          }
          game->grid[p->y][p->x] = color_id;
          if (color_id >= 0 && color_id < game->num_players) {
              game->players[color_id].score++;
          }
      }
  }

  /* Appliquer le déplacement pour le Clone */
  if (p->fork_active_timer > 0) {
      if (is_dash || is_move) {
          int step_x = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
          int step_y = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
          int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
          for (int s = 0; s < steps; s++) {
              p->fork_x = wrap(p->fork_x + step_x, GRID_WIDTH);
              p->fork_y = wrap(p->fork_y + step_y, GRID_HEIGHT);
              if (game->grid[p->fork_y][p->fork_x] != color_id) {
                  int old_owner = game->grid[p->fork_y][p->fork_x];
                  if (old_owner >= 0 && old_owner < game->num_players) {
                      game->players[old_owner].score--;
                  }
                  game->grid[p->fork_y][p->fork_x] = color_id;
                  if (color_id >= 0 && color_id < game->num_players) {
                      game->players[color_id].score++;
                  }
              }
          }
      } else if ((action >= ACTION_TELEPORT_L && action <= ACTION_TELEPORT_D) || action == ACTION_STILL) {
          p->fork_x = wrap(p->fork_x + dx, GRID_WIDTH);
          p->fork_y = wrap(p->fork_y + dy, GRID_HEIGHT);
          if (game->grid[p->fork_y][p->fork_x] != color_id) {
              int old_owner = game->grid[p->fork_y][p->fork_x];
              if (old_owner >= 0 && old_owner < game->num_players) {
                  game->players[old_owner].score--;
              }
              game->grid[p->fork_y][p->fork_x] = color_id;
              if (color_id >= 0 && color_id < game->num_players) {
                  game->players[color_id].score++;
              }
          }
      }
  }
}

/* ============================================
 * Rendu graphique
 * ============================================ */
static void render_game(GameState *game) {
  /* Effacer l'écran */
  SDL_SetRenderDrawColor(game->renderer, EMPTY_COLOR.r, EMPTY_COLOR.g,
                         EMPTY_COLOR.b, EMPTY_COLOR.a);
  SDL_RenderClear(game->renderer);

  /* Dessiner les cases */
  for (int y = 0; y < GRID_HEIGHT; y++) {
    for (int x = 0; x < GRID_WIDTH; x++) {
      SDL_Rect rect = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE - 1,
                       CELL_SIZE - 1};

      int owner = game->grid[y][x];
      if (owner >= 0) {
        SDL_Color c = game->players[owner].color;
        SDL_SetRenderDrawColor(game->renderer, c.r, c.g, c.b, c.a);
      } else {
        SDL_SetRenderDrawColor(game->renderer, EMPTY_COLOR.r, EMPTY_COLOR.g,
                               EMPTY_COLOR.b, EMPTY_COLOR.a);
      }
      SDL_RenderFillRect(game->renderer, &rect);
    }
  }

  /* Dessiner les marqueurs des joueurs (petit carré blanc au centre) */
  for (int i = 0; i < game->num_players; i++) {
    Player *p = &game->players[i];
    if (p->credits > 0) {
      SDL_Rect marker = {p->x * CELL_SIZE + CELL_SIZE / 4,
                         p->y * CELL_SIZE + CELL_SIZE / 4, CELL_SIZE / 2,
                         CELL_SIZE / 2};
      SDL_SetRenderDrawColor(game->renderer, PLAYER_MARKER.r, PLAYER_MARKER.g,
                             PLAYER_MARKER.b, PLAYER_MARKER.a);
      SDL_RenderFillRect(game->renderer, &marker);
    }
  }

  SDL_RenderPresent(game->renderer);
}

/* ============================================
 * Vérification de fin de partie
 * ============================================ */
static int check_game_over(GameState *game) {
  for (int i = 0; i < game->num_players; i++) {
    if (game->players[i].credits > 0) {
      return 0; /* Au moins un joueur a encore des crédits */
    }
  }
  return 1;
}

/* ============================================
 * Affichage des résultats finaux
 * ============================================ */
static void print_results(GameState *game) {
  printf("\n========================================\n");
  printf("         RÉSULTATS FINAUX\n");
  printf("========================================\n");

  /* Trouver le gagnant */
  int winner = 0;
  int max_score = game->players[0].score;

  for (int i = 0; i < game->num_players; i++) {
    Player *p = &game->players[i];
    printf("Joueur %d (%s): %d cases\n", i + 1, p->name, p->score);

    if (p->score > max_score) {
      max_score = p->score;
      winner = i;
    }
  }

  printf("----------------------------------------\n");
  printf("🏆 GAGNANT: Joueur %d (%s) avec %d cases!\n", winner + 1,
         game->players[winner].name, max_score);
  printf("========================================\n");
}

/* ============================================
 * Boucle principale
 * ============================================ */
static void game_loop(GameState *game) {
  int running = 1;
  SDL_Event event;

  while (running && !game->game_over) {
    /* Gestion des événements SDL */
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = 0;
        }
      }
    }

    /* Mettre à jour chaque joueur */
    for (int i = 0; i < game->num_players; i++) {
      update_player(game, i);
    }

    /* Vérifier la fin de partie */
    game->game_over = check_game_over(game);

    /* Rendu */
    render_game(game);

    /* Délai pour contrôler la vitesse */
    SDL_Delay(GAME_SPEED_MS);
  }

  /* Afficher les résultats et attendre que l'utilisateur ferme */
  print_results(game);

  printf("\nAppuyez sur Échap ou fermez la fenêtre pour quitter...\n");
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT ||
          (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        running = 0;
      }
    }
    SDL_Delay(100);
  }
}

/* ============================================
 * Point d'entrée
 * ============================================ */
int main(int argc, char **argv) {
  GameState game;

  printf("╔════════════════════════════════════╗\n");
  printf("║         SPLASHMEM IA               ║\n");
  printf("║   Bataille de robots colorés       ║\n");
  printf("╚════════════════════════════════════╝\n\n");

  /* Initialiser le générateur aléatoire (pour les bots) */
  srand((unsigned int)time(NULL));

  /* Initialiser le jeu */
  if (init_game(&game, argc, argv) < 0) {
    return 1;
  }

  printf("\n--- Démarrage du jeu ---\n\n");

  /* Lancer la boucle de jeu */
  game_loop(&game);

  /* Nettoyer */
  cleanup_game(&game);

  return 0;
}