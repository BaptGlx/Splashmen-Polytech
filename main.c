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
  char (*get_action)(void); /* Pointeur vers la fonction get_action */
  int x, y;                 /* Position actuelle */
  int credits;              /* Crédits restants */
  int score;                /* Nombre de cases colorées */
  SDL_Color color;          /* Couleur du joueur */
  char name[256];           /* Nom du fichier .so */
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
static int get_action_cost(char action) {
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

  case ACTION_STILL:
  default:
    return COST_STILL;
  }
}

/* Calcule le déplacement (dx, dy) pour une action */
static void get_movement(char action, int *dx, int *dy) {
  *dx = 0;
  *dy = 0;

  int distance = 1;

  /* Détermine la distance */
  if ((action >= ACTION_DASH_U && action <= ACTION_DASH_R) ||
      (action >= ACTION_TELEPORT_U && action <= ACTION_TELEPORT_R)) {
    distance = 8;
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
    break; /* ACTION_STILL ou action invalide */
  }
}

/* ============================================
 * Chargement des joueurs
 * ============================================ */
static int load_player(GameState *game, const char *so_path, int player_id) {
  Player *p = &game->players[player_id];

  /* Ouvrir la bibliothèque dynamique */
  p->handle = dlopen(so_path, RTLD_NOW);
  if (!p->handle) {
    fprintf(stderr, "Erreur: Impossible de charger '%s': %s\n", so_path,
            dlerror());
    return -1;
  }

  /* Récupérer la fonction get_action */
  dlerror(); /* Clear any existing error */
  p->get_action = (char (*)(void))dlsym(p->handle, "get_action");
  char *error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "Erreur: 'get_action' non trouvée dans '%s': %s\n", so_path,
            error);
    dlclose(p->handle);
    return -1;
  }

  /* Initialiser le joueur */
  strncpy(p->name, so_path, sizeof(p->name) - 1);
  p->name[sizeof(p->name) - 1] = '\0';
  p->credits = STARTING_CREDITS;
  p->score = 0;
  p->color = PLAYER_COLORS[player_id];

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

  printf("Joueur %d chargé: %s (position: %d,%d)\n", player_id + 1, so_path,
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
  /* Décharger les bibliothèques */
  for (int i = 0; i < game->num_players; i++) {
    if (game->players[i].handle) {
      dlclose(game->players[i].handle);
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

  /* Joueur sans crédits ? */
  if (p->credits <= 0)
    return;

  /* Demander l'action au joueur */
  char action = p->get_action();
  int cost = get_action_cost(action);

  /* Pas assez de crédits ? Faire ACTION_STILL à la place */
  if (cost > p->credits) {
    action = ACTION_STILL;
    cost = COST_STILL;
  }

  /* Déduire le coût */
  p->credits -= cost;

  /* Calculer le déplacement */
  int dx, dy;
  get_movement(action, &dx, &dy);

  /* Appliquer le déplacement avec wrap-around torique */
  int is_dash = (action >= ACTION_DASH_U && action <= ACTION_DASH_R);
  int is_move = (action >= ACTION_MOVE_U && action <= ACTION_MOVE_R);

  if (is_dash || is_move) {
    /* Colorier tout le long du trajet de DASH ou MOVE */
    int step_x = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int step_y = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    for (int s = 0; s < steps; s++) {
      p->x = wrap(p->x + step_x, GRID_WIDTH);
      p->y = wrap(p->y + step_y, GRID_HEIGHT);

      if (game->grid[p->y][p->x] != player_id) {
        int old_owner = game->grid[p->y][p->x];
        if (old_owner >= 0 && old_owner < game->num_players) {
          game->players[old_owner].score--;
        }
        game->grid[p->y][p->x] = player_id;
        p->score++;
      }
    }
  } else {
    /* TELEPORT ou STILL : On saute directement à la case d'arrivée */
    p->x = wrap(p->x + dx, GRID_WIDTH);
    p->y = wrap(p->y + dy, GRID_HEIGHT);

    /* Colorier la case d'arrivée seulement */
    if (game->grid[p->y][p->x] != player_id) {
      /* Si la case appartenait à un autre joueur, retirer son score */
      int old_owner = game->grid[p->y][p->x];
      if (old_owner >= 0 && old_owner < game->num_players) {
        game->players[old_owner].score--;
      }

      /* Attribuer la case au joueur actuel */
      game->grid[p->y][p->x] = player_id;
      p->score++;
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
