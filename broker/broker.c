//
// Created by esteban on 01/03/2026.
//

// --- imports ---

#include "broker.h"
#include <ncurses.h>

// --- init des var globales du .h----
GameState *game_state = NULL;
int shm_fd = -1;
mqd_t bl_id = -1;
sem_t *sem_id = SEM_FAILED;
Deck main_deck;

// --- main ---
int main() {
    srand(time(NULL)); //init rand
    init_ipc();

    //ajout ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    char dernier_log_important[128] = "La partie vient de commencer !";

    while (1) {
        // pour catch les dofférents moyen d'arreter le programme
        signal(SIGINT, handle_sigint);  // rattrape le Ctrl+C
        signal(SIGTERM, handle_sigint); // rattrape le kill normal
        signal(SIGTSTP, handle_sigint); // rattrape le Ctrl+Z

        //ajout affichage ncruses
        clear();
        if (game_state->current_phase == PHASE_LOBBY) {
            mvprintw (2,4, "====== LIAR's BAR - Lobby =======");
            mvprintw (2,4, "bienvenue au liar's bar !");
            mvprintw(2,4, "%d joueurs connectés sur %d", game_state->connected_players, MAX_PLAYERS);
            if (game_state->connected_players >= 2) {
                attron(A_REVERSE);
                mvprintw(6, 4, "[ FORCER LE DEMARRAGE : appyer sur entrée ]");
                attroff(A_REVERSE);
            }
        }
        else {
            mvprintw(2, 4, "====== LIAR's BAR - partie en cours =======");
            int en_vie = 0;
            //comptage nombre de joueur en vie
            for(int i = 0; i < MAX_PLAYERS; i++) {
                if(game_state->players[i].pid != 0 && game_state->players[i].is_alive) en_vie++;
            }
            mvprintw(4, 4, "Nombre de joueurs en vie : %d", en_vie);

            mvprintw(6, 4, ">> Dernier evenement majeur :");
            mvprintw(7, 7, "%s", dernier_log_important);
        }
        refresh();
        int choice = getch();
        if (choice == '\n' && game_state->current_phase == PHASE_LOBBY && game_state->connected_players >=2 ) {
            //démarrage du jeu
            game_state->current_player_idx=0;
            lancer_nouvelle_manche();
            broadcast_update();
        }

        ClientMessage msg;
        // projet messagerie / adapté vars (attente message)
        if (mq_receive(bl_id, (char*)&msg, sizeof(ClientMessage), NULL) >= 0) {
            //attente evenet
           sem_wait(sem_id);

            //apel de la bonne fonction en fonction de l'action recue
            switch (msg.action) {
                case ACT_LOGIN:      gerer_login(&msg); break;
                case ACT_PLAY_CARDS: gerer_play_cards(&msg); break;
                case ACT_CALL_LIAR:
                    gerer_call_liar(&msg);
                    strncpy(dernier_log_important, game_state->log_message, 127);
                    break;
                case ACT_SHOOT:
                    gerer_shoot(&msg);
                    strncpy(dernier_log_important, game_state->log_message, 127);
                    break;
                case ACT_QUIT:       gerer_quit(&msg); break;
            }
            // update écrans joueurs apres action
            broadcast_update();
        }
        usleep(50000);

    }
    return 0;
}

// --- fonctions du .h

void init_ipc() {

    //mémorie partagée
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(GameState));
    game_state = mmap(NULL, sizeof(GameState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // init le jeu
    game_state->current_phase = PHASE_LOBBY;
    game_state->connected_players = 0;

   //bloquer quand quelq'un joue
    sem_id = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    // BL
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(ClientMessage);
    attr.mq_curmsgs = 0;
    bl_id = mq_open(BL_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0666, &attr);
}

void cleanup() {
    endwin();

    // clean memoire partagee
    munmap(game_state, sizeof(GameState)),
    close(shm_fd);
    shm_unlink(SHM_NAME);

    // clean semaphore
    sem_close(sem_id);
    sem_unlink(SEM_NAME);

    //clean BL
    mq_close(bl_id);
    mq_unlink(BL_NAME);

    printf("Broker termine.\n");
    exit(0);
}

void handle_sigint(int sig) {
    cleanup();
    //TODO: compléter (?)
}

// projet messagerie / adapté vars (ancienne boucle kill)
void broadcast_update() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game_state->players[i].pid != 0) {
            // si joueur connecté a cette place, update de l'affichage
            kill(game_state->players[i].pid, SIG_UPDATE);
        }
    }
}

// ---logique de jeu ---

// reset la table pour une nouvelle manche
void lancer_nouvelle_manche() {
    game_state->pot_total_cards = 0;
    game_state->last_played_count = 0;
    game_state->last_player_idx = -1;

    // on efface les cartes posées
    for (int i = 0; i < 3; i++) {
        game_state->last_played_cards_reveal[i] = CARD_NONE;
    }

    // choisit aléatoire roi0, dame1 ou as2
    game_state->table_requirement = rand() % 3;

    init_deck();
    shuffle_deck();
    distribuer_cartes();

    game_state->current_phase = PHASE_PLAYING;
}

// remplit le main_deck avec les 20 cartes
void init_deck() {
    int pos = 0;
    for(int i = 0; i < 6; i++) {
        main_deck.cards[pos] = CARD_KING;
        pos+=1;
    }
    for(int i = 0; i < 6; i++) {
        main_deck.cards[pos] = CARD_QUEEN;
        pos+=1;
    }
    for(int i = 0; i < 6; i++) {
        main_deck.cards[pos] = CARD_ACE;
        pos+=1;
    }
    for(int i = 0; i < 2; i++) {
        main_deck.cards[pos] = CARD_JOKER;
        pos+=1;
    }
    main_deck.top_index = 0;
}

// melange le deck
void shuffle_deck() {
    for (int i = 0; i < LIAR_DECK_SIZE; i++) {
        int hasard = rand() % LIAR_DECK_SIZE;
        CardValue temp = main_deck.cards[i];
        main_deck.cards[i] = main_deck.cards[hasard];
        main_deck.cards[hasard] = temp;
    }
}

// tirer la premiere carte du paquet
CardValue piocher_carte() {
    if (main_deck.top_index >= LIAR_DECK_SIZE) {
        return CARD_NONE;
    }
    CardValue carte = main_deck.cards[main_deck.top_index];
    main_deck.top_index+=1;
    return carte;
}

// donne max 5 cartes a chaque joueur en vie
void distribuer_cartes() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // verifiue si ya un joueur et s'il est vivant
        if (game_state->players[i].pid != 0 && game_state->players[i].is_alive == true) {
            game_state->players[i].cards_left = 0;
            for (int j = 0; j < HAND_SIZE; j++) {
                game_state->players[i].hand[j] = piocher_carte();
                if (game_state->players[i].hand[j] != CARD_NONE) {
                    game_state->players[i].cards_left++;
                }
            }
        }
    }
}


// --- action handlers ---

// cherche un slot libre et inscrit le pid et pseudo
void gerer_login(ClientMessage *msg) {
    if (game_state->current_phase != PHASE_LOBBY) return; //  check jeu commencé

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game_state->players[i].pid == 0) {
            game_state->players[i].pid = msg->client_pid;
            strncpy(game_state->players[i].pseudo, msg->pseudo, 31);
            game_state->players[i].is_alive = true;
            game_state->connected_players++;
            break;
        }
    }

    // autostart a 4 joueurs
    if (game_state->connected_players == MAX_PLAYERS) {
        game_state->current_player_idx = 0;
        lancer_nouvelle_manche();
    }
}

// prend les cartes de la main et les met dans le pot
void gerer_play_cards(ClientMessage *msg) {
    int index = trouver_joueur_par_pid(msg->client_pid);
    if (index == -1 || index != game_state->current_player_idx) return;

    game_state->last_played_count = msg->num_cards_played;
    game_state->last_player_idx = index;

    for (int i = 0; i < msg->num_cards_played; i++) {
        int idx_carte = msg->card_indices[i];
        game_state->last_played_cards_reveal[i] = game_state->players[index].hand[idx_carte];
        game_state->players[index].hand[idx_carte] = CARD_NONE;
        game_state->players[index].cards_left--;
    }

    game_state->pot_total_cards = game_state->pot_total_cards + msg->num_cards_played;
    passer_au_joueur_suivant();
}

// verifie si le joueur precdeent a menti en retournant ses cartesS
void gerer_call_liar(ClientMessage *msg) {
    int accusateur = trouver_joueur_par_pid(msg->client_pid);
    int accuse = game_state->last_player_idx;

    int a_menti = 0; // 0 = non, 1 = oui
    for (int i = 0; i < game_state->last_played_count; i++) {
        CardValue c = game_state->last_played_cards_reveal[i];
        if (c != game_state->table_requirement && c != CARD_JOKER) {
            a_menti = 1;
        }
    }

    if (a_menti == 1) {
        game_state->current_player_idx = accuse; // cramé il prend le gun
    } else {
        game_state->current_player_idx = accusateur; // il c'est trompé il prend le gun
    }

    game_state->current_phase = PHASE_ROULETTE;
}

// tire a la roulette russe
void gerer_shoot(ClientMessage *msg) {
    int index = trouver_joueur_par_pid(msg->client_pid);
    if (index == -1 || index != game_state->current_player_idx) return;

    int balle = rand() % 6; //une chance sur 6 de mourrir
    if (balle == 0) {
        game_state->players[index].is_alive = false; // rip
    }

    verifier_fin_de_partie();   //vérficication au cas ou ils étaient que 2 et il meurt
                                //faisable dans le balle==0 ???? (to do vérifier)

    if (game_state->current_phase != PHASE_GAME_OVER) {
        lancer_nouvelle_manche();
    }
}

// supprime le joueur de la table (remet son pid a 0) s'il quitte avant la fin
void gerer_quit(ClientMessage *msg) {
    int index = trouver_joueur_par_pid(msg->client_pid);
    if (index != -1) {
        game_state->players[index].pid = 0;
        game_state->players[index].is_alive = false;
        game_state->connected_players--;
        verifier_fin_de_partie();
    }
}


// retrouve l'index du jouer avec son pid / (0 a  3)
int trouver_joueur_par_pid(pid_t pid) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game_state->players[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

// psase au joueur vivant suivant
void passer_au_joueur_suivant() {
    int trouve = 0;
    while (trouve == 0) {
        game_state->current_player_idx++;

        // revient a 0 si on depasse le nombre de joueur max
        //remaque: pas le plus adapté mais acceptable vu le nombre d'itération max
        if (game_state->current_player_idx >= MAX_PLAYERS) {
            game_state->current_player_idx = 0;
        }

        int i = game_state->current_player_idx;
        if (game_state->players[i].pid != 0 && game_state->players[i].is_alive == true) {
            trouve = 1;
        }
    }
}

// check combien de joueurs sont en vie et change la phase de jeu
void verifier_fin_de_partie() {
    int en_vie = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game_state->players[i].pid != 0 && game_state->players[i].is_alive == true) {
            en_vie++;
        }
    }
    if (en_vie <= 1) {
        game_state->current_phase = PHASE_GAME_OVER;
    }
}