#include "player.h"

#define CHECK(sts,msg) if ((sts) == -1) { perror(msg); exit(-1); }

int nb_player = 4;
int my_player_index;
char *player_name;

int other_players_id[MAX_PLAYERS-1];
int nb_other_player = 0;

int selected_card = -1;

int card_id_to_play[MAX_CARD_PLAYABLE];
int nb_card_played = 0;


int update_flag = 0;
int shm_fd = -1;
mqd_t bl_id = -1;
sem_t *sem_id = SEM_FAILED;

GameState *game_state_shm = NULL;
pid_t my_pid;
PlayerState my_state;

const char *TYPE_SYMBOLS[] = {"♥","♦","♣","♠"};

int main(int argc, char **argv){

    if (argc < 2){
        perror("Il faut ajouter votre nom");
        exit(-1);
     }

    player_name = argv[1];
    my_pid = getpid();

    init_ipc();
    init_graphic();

    player_login();

    game_loop();

    return 0;
}

/////////////////////////
// Partie communication 
/////////////////////////
void init_ipc(){

    //MP jeu
    CHECK(shm_fd = shm_open(SHM_NAME, O_RDWR, 0666), "--Error shm_open--");
    game_state_shm = mmap(NULL, sizeof(GameState), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (game_state_shm == MAP_FAILED) { perror("--Error mmap --"); exit(-1);}

    //bl
    CHECK(bl_id = mq_open(BL_NAME, O_WRONLY), "--Error mq_open--");

    //semaphore
    sem_id = sem_open(SEM_NAME,0);
    if (sem_id == SEM_FAILED) { perror("--Error sem_open--"); exit(-1);}
}

void player_login(){
    ClientMessage msg;
    msg.client_pid = my_pid;
    strncpy(msg.pseudo, player_name ,31);
    msg.action = ACT_LOGIN;

    send_message(&msg);
}

void game_loop(){

    while (1){

        signal(SIGINT, handle_sigint);
        signal(SIGTERM, handle_sigint);
        signal(SIG_UPDATE, handle_update);

        //Attente des signaux pour lire la mp et afficher/gérer le tour
        if (update_flag){
            sem_wait(sem_id);
            GameState copy_game_state = *game_state_shm;
            sem_post(sem_id);
            
            my_player_index = -1;
            nb_other_player = 0;
            for (int i = 0; i < MAX_PLAYERS; i++){
                if (copy_game_state.players[i].pid == my_pid){
                    my_state = copy_game_state.players[i];
                    my_player_index = i;
                }else{
                    other_players_id[nb_other_player] = i;
                    nb_other_player++;
                }
            }

            nb_player = copy_game_state.connected_players;

            //Afficher l'état du jeu au début du tour
            display_game_state(&copy_game_state);

            switch (copy_game_state.current_phase)
            {
            case PHASE_LOBBY:
            display_waiting_lobby(&copy_game_state);
            break;
            case PHASE_PLAYING:
                play_turn(&copy_game_state);
                break;
            case PHASE_ROULETTE:
                //Gestion roulette
                break;
            case PHASE_GAME_OVER:
                //Affichage fin de partie
                cleanup();
                break;
            default:
                break;
            }
        }
        update_flag = 0;
    }
}

void select_card(GameState *game_state){

    int ch;
    selected_card = 0; // première carte par défaut
    display_game_state(game_state);

    while (1) {
        ch = getch();
        if (ch != ERR){
        if (ch == KEY_LEFT) {
            if (selected_card > 0) selected_card--;
        } else if (ch == KEY_RIGHT) {
            if (selected_card < my_state.cards_left - 1) selected_card++;
        } else if (ch == ' '){
            if (is_card_played(selected_card)) remove_card_to_play(selected_card);
            else add_card_to_play(selected_card);
        } else if (ch == '\n') {
            break;
        }
        display_game_state(game_state);
    }
    }
    selected_card = -1;

}

int is_card_played(int id){
    for (int i = 0; i < nb_card_played; i++){
        if(card_id_to_play[i] == id) return 1;
    }
    return 0;
}

void add_card_to_play(int id) {
    if (nb_card_played < MAX_CARD_PLAYABLE){
        card_id_to_play[nb_card_played] = id;
        nb_card_played++;
    }
}

void remove_card_to_play(int id) {
    for (int i = 0; i < nb_card_played; i++) {
        if (card_id_to_play[i] == id) {
            for (int j = i; j < nb_card_played - 1; j++) {
                card_id_to_play[j] = card_id_to_play[j+1];
            }
            nb_card_played--;
            break;
        }
    }
}


void send_message(ClientMessage *msg){
    CHECK(mq_send(bl_id, (char*)msg, sizeof(ClientMessage), 0), "--Error mq_send--");
}

void send_played_action(){
    ClientMessage msg;
    msg.action = ACT_PLAY_CARDS;
    memcpy(msg.card_indices, card_id_to_play, MAX_CARD_PLAYABLE * sizeof(int));
    msg.client_pid = my_pid;
    strncpy(msg.pseudo, player_name ,31);
    msg.num_cards_played = nb_card_played;
    send_message(&msg); 
}

void play_turn(GameState *game_state){
    if (game_state->players[game_state->current_player_idx].pid == my_pid){
        select_card(game_state);
        send_played_action();

        memset(card_id_to_play, 0, 3 * sizeof(int));
        nb_card_played = 0;

        //TODO Gérer bullet
    }
}

void handle_update(int sig){
    update_flag = 1;
}


void handle_sigint(int sig){
    cleanup();
}


void cleanup(){
    endwin();

    if (game_state_shm != NULL){
        munmap(game_state_shm, sizeof(GameState));
        game_state_shm = NULL;
    }

    if (shm_fd != -1){
        close(shm_fd);
        shm_fd = -1;
    }

    if (bl_id != -1){
        mq_close(bl_id);
        bl_id = -1;
    }

    if (sem_id != SEM_FAILED){
        sem_close(sem_id);
        sem_id = SEM_FAILED;
    }

    printf("Player terminé avec succès !");
    exit(0);
}


//////////////////////////
//Partie graphique
//////////////////////////
void init_graphic(){
    
    setlocale(LC_ALL, ""); 
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    
    if(has_colors()){
        start_color();
        init_pair(1, COLOR_RED, COLOR_WHITE);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_BLACK, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_WHITE);
        init_pair(6, COLOR_BLUE, COLOR_WHITE);
        init_pair(7, COLOR_YELLOW, COLOR_WHITE);
        init_pair(8, COLOR_MAGENTA, COLOR_WHITE);
        init_pair(9, COLOR_YELLOW, COLOR_YELLOW);
        init_pair(10, COLOR_CYAN, COLOR_CYAN);
        if (can_change_color()){
            init_color(10, 0, 350, 100);
            init_pair(3, COLOR_WHITE, 10);
        }else{
        init_pair(3, COLOR_WHITE, COLOR_GREEN);
        }
    }
}


void draw_box(int top, int left, int bottom, int right)
{
    mvhline(top, left+1, ACS_HLINE, right-left-1);
    mvhline(bottom, left+1, ACS_HLINE, right-left-1);

    mvvline(top+1, left, ACS_VLINE, bottom-top-1);
    mvvline(top+1, right, ACS_VLINE, bottom-top-1);

    mvaddch(top, left, ACS_ULCORNER);
    mvaddch(top, right, ACS_URCORNER);
    mvaddch(bottom, left, ACS_LLCORNER);
    mvaddch(bottom, right, ACS_LRCORNER);
}

void draw_card(int y, int x, CardValue card, int is_selected, int is_played)
{
    int h = 7;
    int w = 10;

    int color;
    char *value;
    switch (card)
    {
    case CARD_KING:
        value = "K";
        color = 6;
        break;
    case CARD_QUEEN:
        value = "Q";
        color = 1;
        break;
    case CARD_ACE:
        value = "A";
        color = 7;
        break;
    case CARD_JOKER:
        value = "★";
        color = 8;
        break;
    
    default:
        value = "?"; 
        color = 2;
        break;
    }

    attron(COLOR_PAIR(5));

    // ======================
    // Contour
    // ======================
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x+1, ACS_HLINE, w-2);
    mvaddch(y, x+w-1, ACS_URCORNER);

    mvaddch(y+h-1, x, ACS_LLCORNER);
    mvhline(y+h-1, x+1, ACS_HLINE, w-2);
    mvaddch(y+h-1, x+w-1, ACS_LRCORNER);

    mvvline(y+1, x, ACS_VLINE, h-2);
    mvvline(y+1, x+w-1, ACS_VLINE, h-2);

    // ======================
    // Nettoyage intérieur
    // ======================
    for(int i = 1; i < h-1; i++)
        mvhline(y+i, x+1, ' ', w-2);

    attroff(COLOR_PAIR(5));

    // ======================
    // Symboles coins
    // ======================
    attron(A_BOLD);
    
    attron(COLOR_PAIR(color));

    mvprintw(y+1, x+1, "%s", TYPE_SYMBOLS[0]);          // Haut gauche
    mvprintw(y+1, x+w-2, "%s", TYPE_SYMBOLS[1]);     // Haut droite
    mvprintw(y+h-2, x+1, "%s", TYPE_SYMBOLS[2]);        // Bas gauche
    mvprintw(y+h-2, x+w-2, "%s", TYPE_SYMBOLS[3]);     // Bas droite


    // ======================
    // Valeur de la carte
    // ======================

    int center_y = y + h/2;
    int center_x = x + w/2;

    mvprintw(center_y, center_x, "%s", value); 

    attroff(A_BOLD);
    attroff(COLOR_PAIR(color));

    // ==========================
    // Ligne jaune sous la carte si sélectionnée
    // ==========================
    if (is_selected) {
        attron(COLOR_PAIR(9));
        mvhline(y + h, x, ' ', w);
        attroff(COLOR_PAIR(9));
    }

    if (is_played){
        attron(COLOR_PAIR(10));
        mvhline(y, x, ' ', w);
        attroff(COLOR_PAIR(10));
    }
}

void draw_card_back(int y, int x)
{
    int h = 7;
    int w = 10;

    attron(COLOR_PAIR(4));

    // ======================
    // Contour
    // ======================
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x+1, ACS_HLINE, w-2);
    mvaddch(y, x+w-1, ACS_URCORNER);

    mvaddch(y+h-1, x, ACS_LLCORNER);
    mvhline(y+h-1, x+1, ACS_HLINE, w-2);
    mvaddch(y+h-1, x+w-1, ACS_LRCORNER);

    mvvline(y+1, x, ACS_VLINE, h-2);
    mvvline(y+1, x+w-1, ACS_VLINE, h-2);

    attroff(COLOR_PAIR(4));

    // ======================
    // Nettoyage intérieur
    // ======================
    for(int i = 1; i < h-1; i++)
        mvhline(y+i, x+1, ' ', w-2);

    // ======================
    // Symboles coins
    // ======================
    attron(A_BOLD);
    
    mvaddch(y+1, x+1, ACS_DIAMOND);          // Haut gauche
    mvaddch(y+1, x+w-2, ACS_DIAMOND);     // Haut droite
    mvaddch(y+h-2, x+1, ACS_DIAMOND);        // Bas gauche
    mvaddch(y+h-2, x+w-2, ACS_DIAMOND);     // Bas droite


    // ======================
    // LB centré
    // ======================

    int center_y = y + h/2;
    int center_x = x + w/2 - 1;

    mvprintw(center_y, center_x, "LB");

    attroff(A_BOLD);
}

void draw_pot(int height, int width, GameState * game_state)
{
    int real_pot_count = game_state->pot_total_cards;

    int center_y = height / 2 - 3;
    int center_x = width / 2 - 5;

    draw_card_back(center_y, center_x);

    // Texte en dessous
    attron(COLOR_PAIR(3));

    if (game_state->last_player_idx != -1){
        char *last_player_name = game_state->players[game_state->last_player_idx].pseudo;
        mvprintw(center_y + 8, center_x -10, "%s a joué %d cartes !", last_player_name, game_state->last_played_count);
    }

    mvprintw(center_y + 9, center_x - 2, "Pot : %d cartes", real_pot_count);

    char card_to_play[25];
    switch (game_state->table_requirement)
    {
    case CARD_KING:
        strcpy(card_to_play, "un roi");
        break;
    case CARD_QUEEN:
        strcpy(card_to_play, "une reine");
        break;
    case CARD_ACE:
        strcpy(card_to_play, "un as");
        break;
    default:
        strcpy(card_to_play, "rien");
        break;
    }

    mvprintw(center_y + 10, center_x - 10, "Le maitre du jeu demande %s", card_to_play);
    attroff(COLOR_PAIR(3));
}

void draw_opponent_card_hand(int top, int left, int bottom, int right, int nb_card)
{
    int card_h = 7;
    int card_w = 11;
    int spacing = 2;
    int total_width = 5 * card_w + 4 * spacing;

    int box_width = right - left;
    int start_x = left + (box_width - total_width) / 2;
    int start_y = top + (bottom - top - card_h) / 2 + 1;

    for(int i = 0; i < nb_card; i++)
    {
        draw_card_back(start_y, start_x + i * (card_w + spacing));
    }
}

void draw_player_card_hand(int top, int left, int bottom, int right, int selected_card)
{
    int card_h = 7;
    int card_w = 10;
    int spacing = 3;

    int nb_cards = my_state.cards_left;

    int total_width = nb_cards * card_w + (nb_cards - 1) * spacing;

    int box_width = right - left;
    int start_x = left + (box_width - total_width) / 2;
    int start_y = top + (bottom - top - card_h) / 2;

    int is_selected = 0;
    for(int i = 0; i < HAND_SIZE; i++)
    {
        if (selected_card == i) is_selected = 1;
        CardValue card = my_state.hand[i];
        if (card != CARD_NONE){
            draw_card(start_y,
                start_x + i * (card_w + spacing),
                card, is_selected, is_card_played(i));
            is_selected = 0;
        }//TODO REGLER LE BUG CARTE ? (FAIRE UNE COPIE DE LA MAIN DU JOUEUR EN SUPPRIMANT LES CARTES INUTILES + GERER LES BON ID peut etre une struct basic)
    }
}

// ===============================
// ADVERSAIRES
// ===============================
void draw_opponents(int height, int width, GameState * game_state)
{
    int box_height = 10;
    int box_width  = width / 3 + 3;
    int top_y = 2;

    int opponent_id;
    if (nb_player == 2)
    {
        int left = width/2 - box_width/2;
        int bottom = top_y + box_height;
        int right = left + box_width;

        opponent_id = other_players_id[0];

        attron(COLOR_PAIR(3));
        draw_box(top_y, left, bottom, right+3);
        mvprintw(top_y-1, left+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left+4, bottom, right, game_state->players[opponent_id].cards_left);
    }
    else if (nb_player == 3)
    {
        int left1 = width/8;
        int bottom = top_y + box_height;
        int right1 = left1 + box_width;

        opponent_id = other_players_id[0];

        attron(COLOR_PAIR(3));
        draw_box(top_y, left1-3, bottom, right1);
        mvprintw(top_y-1, left1+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left1, bottom, right1, game_state->players[opponent_id].cards_left);

        int left2 = width - width/8 - box_width;
        int right2 = left2 + box_width;

        opponent_id = other_players_id[1];

        attron(COLOR_PAIR(3));
        draw_box(top_y, left2, bottom, right2+3);
        mvprintw(top_y-1, left2+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left2+4, bottom, right2, game_state->players[opponent_id].cards_left);
    }
    else if (nb_player == 4)
    {
        // Haut centre
        int left_top = width/2 - box_width/2;
        int bottom_top = top_y + box_height;
        int right_top = left_top + box_width + 1;

        opponent_id = other_players_id[0];

        attron(COLOR_PAIR(3));
        draw_box(top_y, left_top, bottom_top, right_top);
        mvprintw(top_y-1, left_top+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left_top + 2, bottom_top, right_top, game_state->players[opponent_id].cards_left);

        // Gauche
        int mid_y = height/2 - box_height/2;
        int left_g = 2;
        int right_g = left_g + box_width + 1;
        int bottom_g = mid_y + box_height;

        opponent_id = other_players_id[1];

        attron(COLOR_PAIR(3));
        draw_box(mid_y, left_g, bottom_g, right_g);
        mvprintw(mid_y-1, left_g+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(mid_y, left_g + 2, bottom_g, right_g, game_state->players[opponent_id].cards_left);

        // Droite
        int right_x = width - box_width - 2;
        int right_d = right_x + box_width;
        int bottom_d = mid_y + box_height;

        opponent_id = other_players_id[2];

        attron(COLOR_PAIR(3));
        draw_box(mid_y, right_x - 1, bottom_d, right_d);
        mvprintw(mid_y-1, right_x+2, "%s", game_state->players[opponent_id].pseudo);
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(mid_y, right_x + 1, bottom_d, right_d, game_state->players[opponent_id].cards_left);
    }
}

// ===============================
// DISPLAY
// ===============================
void display_game_state(GameState * game_state){

    int height, width;
    getmaxyx(stdscr, height, width);

    int deck_y = height - 12;
    int deck_x = width / 6;

    clear();

    // ===============================
    // Fond vert (table)
    // ===============================
    attron(COLOR_PAIR(3));
    for (int y = 0; y < height; y++){
        mvhline(y, 0, ' ', width);
    }
    attroff(COLOR_PAIR(3));

    // ===============================
    // Main du joueur principal
    // ===============================
    int top = deck_y;
    int bottom = height - 2;
    int left = deck_x;
    int right = width - deck_x;

    attron(COLOR_PAIR(3));
    draw_box(top, left, bottom, right);
    mvprintw(top-1, left+2, "Votre main");
    attroff(COLOR_PAIR(3));

    draw_player_card_hand(top+2, left, bottom, right, selected_card);

    // ===============================
    // Adversaires
    // ===============================
    draw_opponents(height, width, game_state);

    // ===============================
    // Pot
    // ===============================
    draw_pot(height, width, game_state);

    refresh();
}

void display_waiting_lobby(GameState *game_state) {
    // initialise ncurses si pas déjà fait
    static bool ncurses_init = false;
    if (!ncurses_init) {
        initscr();
        cbreak();
        noecho();
        curs_set(0);
        nodelay(stdscr, TRUE); // lecture non bloquante
        ncurses_init = true;
    }

    clear();

    // Titre
    attron(A_BOLD);
    mvprintw(2, 4, "====== LIAR'S BAR - Lobby ======");
    attroff(A_BOLD);

    // Message d'attente
    if (my_player_index == -1) {
        mvprintw(4, 4, "Vous n'êtes pas encore inscrit au jeu...");
    } else {
        mvprintw(4, 4, "Bonjour %s !", game_state->players[my_player_index].pseudo);
        mvprintw(6, 4, "En attente du démarrage de la partie...");
        mvprintw(7, 4, "%d joueurs connectés sur %d", 
                 game_state->connected_players, MAX_PLAYERS);
        if (game_state->connected_players >= 2) {
            attron(A_REVERSE);
            mvprintw(9, 4, "[ Attente du lancement ou appuyez sur entrée pour forcer ]");
            attroff(A_REVERSE);
        }
    }

    // rafraîchissement de l'écran
    refresh();
}