# Splashmem IA - Polytech

Splashmem IA est un jeu de simulation de bataille conçu en C dans lequel jusqu'à 4 bots s'affrontent sur une grille de 100x100. Le but est de terminer la partie en ayant colorié le maximum de cases à la couleur de votre bot.

## 💻 Technologies Utilisées

- **Langage C** : Cœur du moteur de jeu et logique des bots. Utilisation de la bibliothèque `dlfcn.h` pour le chargement dynamique des joueurs (fichiers `.so`) à l'exécution.
- **SDL2 (Simple DirectMedia Layer)** : Gestion de l'interface graphique, du rendu 2D de la grille de jeu, de la boucle de rendu et des événements utilisateur (clavier, fermeture de fenêtre).
- **Make / Makefile** : Système de build permettant l'automatisation de la compilation de l'exécutable principal et la génération des bibliothèques partagées pour les bots.
- **pkg-config** : Utilitaire intégré au processus de build pour résoudre et injecter automatiquement les bons flags de compilation (`CFLAGS`) et d'édition de liens (`LDFLAGS`) requis par SDL2, garantissant la portabilité entre différents systèmes.

## 🗂 Architecture et Fichiers
- `main.c` : Le moteur de jeu principal. Il s'occupe de l'affichage graphique, du chargement dynamique des bibliothèques des bots (`.so`), et de la mécanique de jeu.
- `actions.h` : L'entête définissant les constantes, l'API des déplacements (`ACTION_MOVE_U`, `ACTION_DASH_L` etc.) et les coûts en crédit de chaque action.
- `player_random.c` : Exemple de bot très basique qui exécute des coups de manière purement aléatoire.
- `player_aggressive.c` : Exemple de bot un peu plus compétitif privilégiant souvent des actions "DASH" afin de peindre et de voler de grandes lignes d'un coup.
- `Makefile` : Fichier permettant la compilation automatique du jeu et des bots.

## 🛠 Dépendances et Compatibilité
Le code et le système de compilation sont entièrement conçus pour être **naturellement portables et compatibles sous Linux ou macOS**, les chemins d'inclusions étant récupérés dynamiquement au lieu d'être définis en dur.

**Prérequis selon votre système :**
- **Linux** (Debian/Ubuntu/PopOS etc) : `sudo apt install libsdl2-dev build-essential pkg-config`
- **macOS** (avec Homebrew) : `brew install sdl2 pkg-config`

## 🚀 Compiler et Lancer une Partie

### Compilation rapide
Un simple `make` à la racine compilera l'exécutable du jeu (`splash`) ainsi que tous vos bots en fichiers `.so` !
```bash
make clean
make all
```

### Lancer le jeu
Vous pouvez démarrer une partie manuellement en choisissant les bots (1 à 4) qui vont participer au combat :
```bash
./splash player_random.so player_aggressive.so
```

**Alternative Express de Test :**
Le `Makefile` dispose d'une commande toute prête pour lancer un affrontement de test direct afin de valider vos IA avec 4 bots en arène :
```bash
make test
```

## 🎓 Crédits

* **Contexte** : Projet éducatif réalisé dans le cadre de Polytech Tours.

* **Auteurs** : Baptiste Guilleux et Hugo Braux.

* **Assistance** : Code assisté avec le modèle IA Gemini 3.1 Pro.
