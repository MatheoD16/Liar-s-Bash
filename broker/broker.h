//
// Created by esteban on 22/02/2026.
//


#ifndef BROKER_H
#define BROKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "common.h"

// --- config broker ---
#define LIAR_DECK_SIZE 20   // 6 rois, 6 dames, 6 as, 2 jokers
#define TABLE_DECK_SIZE 3   // 1 roi, 1 dame, 1 as

// index pour le tableau de semaphores
#define SEM_VIDE 0
#define SEM_PLEIN 1
#define SEM_MUTEX 2

// --- structures privees ---

// paquet du croupire (pas dans la shm pour eviter la triche)
typedef struct {
    CardValue cards[LIAR_DECK_SIZE];
    int top_index;          // index de la prochaine carte a piocher
} Deck;

// variables globales pour le broker.c
extern GameState *game_state;
extern int shm_fd;
extern mqd_t bl_id;         // id de la bl
extern sem_t *sem_id;
extern Deck main_deck;
extern ZoneBL *zone_bl;       // ptr vers la 2eme shm (bl)
// type extern: pour ne pas les créer dans le .h mais dans les .c (solution: IA)


// --- ipc & system ---

// init les ipc (shm, bl et semaphore)
// a appeler une seule fois au demarrage du main
void init_ipc();

// nettoie les ipc a la fin ou sur un ctrlcc (unlink et close)
void cleanup();

// intercepte le ctrl+c
void handle_sigint(int sig);

// envoie SIGUSR1 a tt les client pour refresh l'ui
void broadcast_update();


// --- game logic ---

// reset la table pour une nouvelle manche
// vide le pot, tire une nouvelle carte au hasard et donnnes les cartes
// passe le jeu en PHASE_PLAYING
void lancer_nouvelle_manche();

// remplit le main_deck avec les 20 cartes
void init_deck();

// melange le deck avec rand()
void shuffle_deck();

// tire la premiere carte du paquet
CardValue piocher_carte();

// donne 5 cartes (ou moins) a chaque joueur en vie
void distribuer_cartes();


// --- action handlers ---

// cherche un slot libre et inscrit le pid et pseudo
void gerer_login(ClientMessage *msg);

// prend les cartes de la main et les met dans le pot
void gerer_play_cards(ClientMessage *msg);

// verifie si le joueur precdeent a menti en retournant ses cartes
// passe la partie en PHASE_ROULETTE
void gerer_call_liar(ClientMessage *msg);

// tire a la roulette russe (1 chance sur 6 de mourir)
void gerer_shoot(ClientMessage *msg);

// supprime le joueur de la table (remet son pid a 0)
void gerer_quit(ClientMessage *msg);


// --- utils ---

// retrouve l'index (0 a 3) du jouer avec son pid
int trouver_joueur_par_pid(pid_t pid);

// psase au joueur vivant suivant (saute les morts et les slots vides)
void passer_au_joueur_suivant();

// check combien de joueurs sont en vie
// si un seul survivant, passe en PHASE_GAME_OVER
void verifier_fin_de_partie();

#endif // BROKER_H