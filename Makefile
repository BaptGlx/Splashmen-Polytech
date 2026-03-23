# Splashmem IA - Makefile
# Build system pour le jeu et les bots

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC $(shell pkg-config --cflags sdl2)
LDFLAGS = -ldl $(shell pkg-config --libs sdl2)

# Fichier principal
TARGET = splash
MAIN_SRC = main.c

# Bots
BOTS = player_random.so player_random2.so player_aggressive.so player_aggressive2.so

# Règle par défaut
all: $(TARGET) $(BOTS)

# Compilation de l'exécutable principal
$(TARGET): $(MAIN_SRC) actions.h
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(LDFLAGS)

# Compilation des bots (bibliothèques dynamiques)
player_random.so: player_random.c actions.h
	$(CC) $(CFLAGS) -shared -o $@ player_random.c

# Création d'une copie pour pouvoir jouer contre soi-même
player_random2.so: player_random.c actions.h
	$(CC) $(CFLAGS) -shared -o $@ player_random.c

player_aggressive.so: player_aggressive.c actions.h
	$(CC) $(CFLAGS) -shared -o $@ player_aggressive.c

# Création d'une copie pour pouvoir jouer contre soi-même
player_aggressive2.so: player_aggressive.c actions.h
	$(CC) $(CFLAGS) -shared -o $@ player_aggressive.c

# Compilation d'un bot personnalisé
%.so: %.c actions.h
	$(CC) $(CFLAGS) -shared -o $@ $<

# Nettoyage
clean:
	rm -f $(TARGET) *.so

# Rebuild complet
rebuild: clean all

# Test rapide avec 4 joueurs
test: all
	./$(TARGET) player_random.so player_random2.so player_aggressive.so player_aggressive2.so

# Aide
help:
	@echo "Splashmem IA - Makefile"
	@echo "========================"
	@echo "Cibles disponibles:"
	@echo "  all      - Compile tout (défaut)"
	@echo "  clean    - Supprime les fichiers compilés"
	@echo "  rebuild  - Clean + All"
	@echo "  test     - Lance le jeu avec 4 bots"
	@echo "  help     - Affiche cette aide"
	@echo ""
	@echo "Usage: ./splash player1.so [player2.so] [player3.so] [player4.so]"

.PHONY: all clean rebuild test help
