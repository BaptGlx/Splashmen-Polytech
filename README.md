# Splashmem IA - Polytech

Splashmem IA est un jeu de simulation de bataille conçu en C dans lequel jusqu'à 4 bots s'affrontent sur une grille de 100x100. Le but est de terminer la partie en ayant colorié le maximum de cases à la couleur de votre bot.

## 💻 Technologies Utilisées

- **Langage C** : Cœur du moteur de jeu et logique des bots. Utilisation de la bibliothèque `dlfcn.h` pour le chargement dynamique des joueurs (fichiers `.so`) à l'exécution.
- **SDL2 (Simple DirectMedia Layer)** : Gestion de l'interface graphique, du rendu 2D de la grille de jeu, de la boucle de rendu et des événements utilisateur (clavier, fermeture de fenêtre).
- **Make / Makefile** : Système de build permettant l'automatisation de la compilation de l'exécutable principal et la génération des bibliothèques partagées pour les bots.
- **pkg-config** : Utilitaire intégré au processus de build pour résoudre et injecter automatiquement les bons flags de compilation (`CFLAGS`) et d'édition de liens (`LDFLAGS`) requis par SDL2, garantissant la portabilité entre différents systèmes.

## 🗂 Architecture et Fichiers
- `main.c` : Le moteur de jeu principal. Il s'occupe de l'affichage graphique, du chargement dynamique des bibliothèques des bots (`.so`) ou de scripts (`.txt`), et de la mécanique de jeu.
- `actions.h` : L'entête définissant l'énumération `enum action`, les constantes de jeu et les coûts en crédit de chaque action.
- `player_random.c` : Exemple de bot `.so` qui exécute des coups de manière aléatoire.
- `test_player.txt` : Exemple de script de jeu séquentiel lisible par le moteur.
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
Vous pouvez charger jusqu'à 4 joueurs en mélangeant bibliothèques dynamiques (`.so`) et scripts séquentiels (`.txt`) :
```bash
# Mélange de bots compilés et de scripts
./splash player_random.so test_player.txt player_aggressive.so
```

### 📄 Format des fichiers .txt
Les fichiers scripts permettent de programmer une suite d'actions fixe. Le moteur lit le fichier, exécute les actions une par une, et recommence au début une fois la fin du fichier atteinte.
- Format : actions séparées par des virgules.
- Exemple : `ACTION_MOVE_R, ACTION_MOVE_D, ACTION_BOMB, ACTION_CLEAN`

### ⚡ Nouvelles Actions & Capacités
Le moteur Splashmem supporte désormais des actions avancées :
- **ACTION_BOMB** : Pose une bombe qui explose en 3x3 après 5 tours.
- **ACTION_FORK** : Crée un clone après 5 tours. Le clone dure 20 tours et imite vos mouvements. Attention : vos coûts d'actions sont doublés pendant cette période.
- **ACTION_CLEAN** : Nettoie une zone de 7x7 autour de vous (les cases deviennent vides).
- **ACTION_MUTE** : Rend les cases coloriées par l'ennemi le plus proche "noires" (neutres) pendant 10 tours.
- **ACTION_SWAP** : L'ennemi le plus proche colorie les cases à **votre** couleur pendant 5 tours.

---

**Alternative Express de Test :**
Le `Makefile` dispose d'une commande pour lancer un affrontement de test direct :
```bash
make test
```

## 🎓 Crédits

* **Contexte** : Projet éducatif réalisé dans le cadre de Polytech Tours.

* **Auteurs** : Baptiste Guilleux et Hugo Braux.

* **Assistance** : Code assisté avec le modèle IA Gemini 3.1 Pro.
