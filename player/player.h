
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

// --- IPC ---

void init_ipc();
void cleanup();
void send_message(ClientMessage *msg);


// --- SIGNAL HANDLING ---

void handle_update(int sig);
void handle_sigint(int sig);

// --- GAME LOGIC ---

void game_loop();
void send_quit();
void player_login();
void select_card();
int is_card_played(int id);
void add_card_to_play(int id);
void remove_card_to_play(int id);
void play_turn(GameState *game_state);
void play_russian_roulette(GameState * game_state);
void end_game_loop();


// --- UI ---

void display_game_state(GameState * game_state);
void init_graphic();
void draw_box(int top, int left, int bottom, int right);
void draw_opponents(int height, int width, GameState *game_state);
void draw_card(int y, int x, CardValue card, int is_selected, int is_played);
void draw_pot(int height, int width, GameState *game_state);
void draw_card_back(int y, int x);
void draw_opponent_card_hand(int top, int left, int bottom, int right, int nb_card);
void draw_player_card_hand(int top, int left, int bottom, int right, int selected_card);
void display_waiting_lobby(GameState *game_state);
void display_white_screen();
void display_end_screen(GameState *game_state);


#endif