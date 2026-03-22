
#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <semaphore.h>
#include <locale.h>
#include <ncurses.h>

#include "../common.h"


#define DEAD_SYMB "☠"
// --- GLOBAL STATE ---

// pointeur vers la SHM (lecture seule)
extern GameState *game_state_shm;

// id shm
extern int shm_fd;

// boite aux lettres vers le broker
extern mqd_t bl_id;

// semaphore pour proteger la shm
extern sem_t *sem_id;

// pid du player
extern pid_t my_pid;

// index du joueur dans game_state->players[]
extern int my_player_index;

// state du player
extern PlayerState my_state;

// flag modifie par le handler de signal
extern int update_flag;


// --- IPC ---

// ouvre la shm, la mq et le semaphore
void init_ipc();

// ferme proprement les descripteurs locaux
void cleanup();

// envoie un message generique au broker
void send_message(ClientMessage *msg);


// --- SIGNAL HANDLING ---

// handler pour SIG_UPDATE
void handle_update(int sig);

// handler pour CTRL+C
void handle_sigint(int sig);

// installe les handlers
void setup_signal_handlers();


// --- GAME LOGIC ---

void game_loop();

// envoie un call liar
void send_call_liar();

// envoie une action shoot (roulette)
void send_shoot();

// envoie un quit propre
void send_quit();

void player_login();

void select_card();

int is_card_played(int id);

void add_card_to_play(int id);

void remove_card_to_play(int id);

void play_turn(GameState *game_state);

void play_russian_roulette(GameState * game_state);

void end_game_loop();

// --- SHM ACCESS ---

// lit l'etat du jeu (avec semaphore)
void lock_shm();
void unlock_shm();

// met a jour my_player_index en fonction du pid
void update_my_index();


// --- UI ---

// affiche tout l'etat du jeu
void display_game_state(GameState * game_state);

// affiche uniquement la main du joueur
void display_hand();

// affiche les infos globales (tour, phase, requirement)
void display_table_info();

// demande input utilisateur si c'est son tour
void handle_user_input();

// convertit une carte en string
const char* card_to_string(CardValue value);

// efface l'ecran (terminal)
void clear_screen();

// initialiser les variables poru l'affichage graphique (couleurs d'écriture)
void init_graphic();

// dessine un rectangle blanc sur fond vert
void draw_box(int top, int left, int bottom, int right);

// dessine la main des adversaires (position change en fonction du nombre de joueur)
void draw_opponents(int height, int width, GameState *game_state);

// dessine une carte
void draw_card(int y, int x, CardValue card, int is_selected, int is_played);

// dessine le pot au milieu de la table
void draw_pot(int height, int width, GameState *game_state);

// dessine une carte face cachée
void draw_card_back(int y, int x);

// dessine les cartes dans la main des adversaires
void draw_opponent_card_hand(int top, int left, int bottom, int right, int nb_card);

// dessine les cartes dans la main du joueur
void draw_player_card_hand(int top, int left, int bottom, int right, int selected_card);

void display_waiting_lobby(GameState *game_state);

void display_white_screen();

void display_end_screen(GameState *game_state);


#endif