//
// Created by esteban on 17/02/2026.
//

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <stdbool.h>
#include <signal.h>

// --- IPC CONFIG ---
#define SHM_NAME "/liars_bash_shm"
#define BL_NAME  "/liars_bash_bl" // boite aux lettres
#define SEM_NAME "/liars_bash_sem"

// --- GAME CONFIG ---
#define MAX_CARD_PLAYABLE 3
#define MAX_PLAYERS 4
#define HAND_SIZE 5
#define REVOLVER_CAPACITY 6
#define SIG_UPDATE SIGUSR1 // signal pour refresh UI client

// --- ENUMS ---
typedef enum {
    CARD_KING = 0,
    CARD_QUEEN,
    CARD_ACE,
    CARD_JOKER, // joker = innocent
    CARD_NONE   // carte deja posee
} CardValue;

//state machine
typedef enum {
    PHASE_LOBBY,        //attente joueur avant lancement partie
    PHASE_PLAYING,
    PHASE_ROULETTE,
    PHASE_GAME_OVER     //jeu terminé avec un seul joueur
} GamePhase;

typedef enum {
    ACT_LOGIN,          //envoyé au lancement pour se connecter au jeu
    ACT_PLAY_CARDS,
    ACT_CALL_LIAR,
    ACT_SHOOT,
    ACT_QUIT            // a utiliser si le joueur quitte le jeu avant la fin
} ActionType;

// --- IPC DATA STRUCTURES ---

// payload client -> broker (via BL)
typedef struct {
    pid_t client_pid;
    char pseudo[32];      // utilise si action == ACT_LOGIN
    ActionType action;

    // Args pour ACT_PLAY_CARDS
    int num_cards_played; // 1 a 3
    int card_indices[MAX_CARD_PLAYABLE];  // Index dans la main (0 a 4)
} ClientMessage; //faut tout mettre parceque les paquets s'envoient pas dans la BL s'ils font pas la meme taille
//(c'est chiant j'ai passé 1h a comprendre le man c'est bien eft)

// etat d'un joueur (via SHM)
typedef struct {
    pid_t pid;            // 0 si slot dispo
    char pseudo[32];
    bool is_alive;

    CardValue hand[HAND_SIZE];  //5 (je crois?)
    int cards_left;
    int bullets_survived;       //pour savoirla proba qu'il a de mourrir
} PlayerState;

// etat du jeu (via SHM, protege par semaphore)
typedef struct {
    GamePhase current_phase;
    int connected_players;

    // infos tour par tour
    int current_player_idx;
    int last_player_idx;                   //pour le menteur
    CardValue last_played_cards_reveal[MAX_CARD_PLAYABLE]; //pour afficher en cas de menteur

    // infos table
    CardValue table_requirement;
    int pot_total_cards;        //le nombre dde cardes déja jouées pour savoir combien en afficher
    int last_played_count;      //pour savoir combien de carte enlever

    // data joueurs
    PlayerState players[MAX_PLAYERS];   //surtout pour savoir combien de cartes affichées a chaqueposition de joueur pour le client

    // log pour affichage (ncruses )
    char log_message[128];              //pour afficher genre "jouer x a joué 3 cartes Y'
} GameState;

#endif // COMMON_H