//
// Created by esteban on 01/03/2026.
//

// --- imports ---

#include <broker.h>

// projet messagerie / adapté vars
void P(int index) {
    struct sembuf op = { index, -1, 0 };
    semop(semid, &op, 1);
}

// projet messagerie / adapté vars
void V(int index) {
    struct sembuf op = { index, +1, 0 };
    semop(semid, &op, 1);
}

// --- main ---
while (1) {
    // pour catch les dofférents moyen d'arreter le programme
    signal(SIGINT, handle_sigint);  // rattrape le Ctrl+C
    signal(SIGTERM, handle_sigint); // rattrape le kill normal
    signal(SIGTSTP, handle_sigint); // rattrape le Ctrl+Z
    // projet messagerie / adapté vars (attente message)
    P(SEM_PLEIN);
    P(SEM_MUTEX);

    // On lit le message dans la boite aux lettres
    ClientMessage msg = zone_bl->buffer[zone_bl->lecture];
    zone_bl->lecture = (zone_bl->lecture + 1) % MAX_MSG_BL;

    V(SEM_MUTEX);
    V(SEM_VIDE);

    //TODO: SWITCH CASE
}

// --- fonctions du .h

void init_ipc() {
    // projet messagerie / adapté vars (main)

    // Création/acces memoire partagee (BL)
    key_t cle_bl = ftok(CLE_PATH, ID_SHM_BL);
    if (cle_bl == -1) { perror("ftok bl"); exit(1); }
    shmid_bl = shmget(cle_bl, sizeof(ZoneBL), 0666 | IPC_CREAT);
    zone_bl = (ZoneBL*) shmat(shmid_bl, NULL, 0);
    if (zone_bl == (void*) -1) { perror("shmat bl"); exit(1); }

    // Création/acces memoire partagee (jeu)
    key_t cle_jeu = ftok(CLE_PATH, ID_SHM_JEU);
    shmid_jeu = shmget(cle_jeu, sizeof(GameState), 0666 | IPC_CREAT);
    game_state = (GameState*) shmat(shmid_jeu, NULL, 0);
    if (game_state == (void*) -1) { perror("shmat jeu"); exit(1); }

    // Creation/semaphores
    key_t cle_sem = ftok(CLE_PATH, ID_SEM);
    semid = semget(cle_sem, 3, 0666 | IPC_CREAT); // 3 au lieu de 4 ici

    // Initialisation semaphores
    union semun { int val; } arg;
    arg.val = MAX_MSG_BL;  // positions vides initiales
    semctl(semid, SEM_VIDE, SETVAL, arg);
    arg.val = 0;           // positions pleines initiales = 0
    semctl(semid, SEM_PLEIN, SETVAL, arg);
    arg.val = 1;           // mutex a 1
    semctl(semid, SEM_MUTEX, SETVAL, arg);

    // Initialiser buffer
    zone_bl->ecriture = zone_bl->lecture = 0;
}

void cleanup() {
    // pprojet messagerie / adapté vars)
    shmdt(zone_bl);
    shmdt(game_state);
    shmctl(shmid_bl, IPC_RMID, NULL);
    shmctl(shmid_jeu, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, 0);
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