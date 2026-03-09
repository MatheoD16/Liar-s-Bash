#include "player.h"

int nb_player = 4;

const char *TYPE_SYMBOLS[] = {"♥","♦","♣","♠"};

int main(){

    init_graphic();
    display_game_state();

    return 0;
}

void init_graphic(){
    
    setlocale(LC_ALL, ""); 
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    
    if(has_colors()){
        start_color();
        init_pair(1, COLOR_RED, COLOR_WHITE);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(4, COLOR_BLACK, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_WHITE);
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

void draw_card(int y, int x, int type, int color)
{
    int h = 7;
    int w = 10;

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

    mvprintw(y+1, x+1, "%s", TYPE_SYMBOLS[type]);          // Haut gauche
    mvprintw(y+1, x+w-2, "%s", TYPE_SYMBOLS[type]);     // Haut droite
    mvprintw(y+h-2, x+1, "%s", TYPE_SYMBOLS[type]);        // Bas gauche
    mvprintw(y+h-2, x+w-2, "%s", TYPE_SYMBOLS[type]);     // Bas droite


    // ======================
    // LB centré
    // ======================

    int center_y = y + h/2;
    int center_x = x + w/2;

    mvprintw(center_y, center_x, "1");

    attroff(A_BOLD);
    attroff(COLOR_PAIR(color));
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

void draw_pot(int height, int width)
{
    int real_pot_count = 12;      // vrai nombre (exemple)

    int center_y = height / 2 - 3;
    int center_x = width / 2 - 5;

    draw_card_back(center_y, center_x);

    // Texte en dessous
    attron(COLOR_PAIR(3));
    mvprintw(center_y + 8, center_x - 2, "Pot : %d cartes", real_pot_count);
    attroff(COLOR_PAIR(3));
}

void draw_opponent_card_hand(int top, int left, int bottom, int right)
{
    int card_h = 7;
    int card_w = 11;
    int spacing = 2;
    int total_width = 5 * card_w + 4 * spacing;

    int box_width = right - left;
    int start_x = left + (box_width - total_width) / 2;
    int start_y = top + (bottom - top - card_h) / 2 + 1;

    for(int i = 0; i < 5; i++)
    {
        draw_card_back(start_y, start_x + i * (card_w + spacing));
    }
}

void draw_player_card_hand(int top, int left, int bottom, int right)
{
    int card_h = 7;
    int card_w = 10;
    int spacing = 3;

    int nb_cards = 5;

    int total_width = nb_cards * card_w + (nb_cards - 1) * spacing;

    int box_width = right - left;
    int start_x = left + (box_width - total_width) / 2;
    int start_y = top + (bottom - top - card_h) / 2;

    for(int i = 0; i < nb_cards; i++)
    {
        int type = i % 4;
        int color = (type == 0 || type == 1) ? 1 : 2;

        draw_card(start_y,
                  start_x + i * (card_w + spacing),
                  type,
                  color);
    }
}

// ===============================
// ADVERSAIRES
// ===============================
void draw_opponents(int height, int width)
{
    int box_height = 10;
    int box_width  = width / 3 + 3;
    int top_y = 2;

    if (nb_player == 2)
    {
        int left = width/2 - box_width/2;
        int bottom = top_y + box_height;
        int right = left + box_width;

        attron(COLOR_PAIR(3));
        draw_box(top_y, left, bottom, right+3);
        mvprintw(top_y-1, left+2, "Joueur 2");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left+4, bottom, right);
    }
    else if (nb_player == 3)
    {
        int left1 = width/8;
        int bottom = top_y + box_height;
        int right1 = left1 + box_width;

        attron(COLOR_PAIR(3));
        draw_box(top_y, left1-3, bottom, right1);
        mvprintw(top_y-1, left1+2, "Joueur 2");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left1, bottom, right1);

        int left2 = width - width/8 - box_width;
        int right2 = left2 + box_width;

        attron(COLOR_PAIR(3));
        draw_box(top_y, left2, bottom, right2+3);
        mvprintw(top_y-1, left2+2, "Joueur 3");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left2+4, bottom, right2);
    }
    else if (nb_player == 4)
    {
        // Haut centre
        int left_top = width/2 - box_width/2;
        int bottom_top = top_y + box_height;
        int right_top = left_top + box_width + 1;

        attron(COLOR_PAIR(3));
        draw_box(top_y, left_top, bottom_top, right_top);
        mvprintw(top_y-1, left_top+2, "Joueur 2");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(top_y, left_top + 2, bottom_top, right_top);

        // Gauche
        int mid_y = height/2 - box_height/2;
        int left_g = 2;
        int right_g = left_g + box_width + 1;
        int bottom_g = mid_y + box_height;

        attron(COLOR_PAIR(3));
        draw_box(mid_y, left_g, bottom_g, right_g);
        mvprintw(mid_y-1, left_g+2, "Joueur 3");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(mid_y, left_g + 2, bottom_g, right_g);

        // Droite
        int right_x = width - box_width - 2;
        int right_d = right_x + box_width;
        int bottom_d = mid_y + box_height;

        attron(COLOR_PAIR(3));
        draw_box(mid_y, right_x - 1, bottom_d, right_d);
        mvprintw(mid_y-1, right_x+2, "Joueur 4");
        attroff(COLOR_PAIR(3));
        draw_opponent_card_hand(mid_y, right_x + 1, bottom_d, right_d);
    }
}

// ===============================
// DISPLAY
// ===============================
void display_game_state(){

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

    draw_player_card_hand(top+2, left, bottom, right);

    // ===============================
    // Adversaires
    // ===============================
    draw_opponents(height, width);

    draw_pot(height, width);

    refresh();
    getch();
    endwin();
}