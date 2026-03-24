# 🎲 Liar's Bash - Jeu en Terminal (C / Ncurses)

## 📌 Description

**Liar's Bash** est un jeu multijoueur en terminal basé sur le bluff et le hasard.
Le projet est développé en langage C et utilise **Ncurses** pour l'affichage.

L'architecture repose sur :

* Un **broker (serveur de jeu)** qui gère toute la logique
* Jusqu'à **4 joueurs (clients)** qui rejoignent la partie

La communication se fait via :

* **Mémoire partagée POSIX** (état du jeu)
* **Sémaphore nommé** (synchronisation)
* **Boîte aux lettres POSIX (message queue)** (actions des joueurs)
* **Signaux** (notification de mise à jour)

---

## 🎮 Règles du jeu

### 🃏 Objectif

Être le **dernier joueur en vie** ou le **premier à ne plus avoir de cartes**.

---

### 🔄 Déroulement d’un tour

1. Le serveur demande une carte parmi :

   * Dame
   * Roi
   * As

2. Chaque joueur joue à son tour :

   * Il doit poser la carte demandée s’il l’a
   * Sinon, il peut **bluffer** en posant une autre carte

3. Le joueur suivant peut :

   * **Accepter** le coup
   * Ou **accuser le joueur précédent de mentir**

---

### ⚖️ Résolution d’un mensonge

* Si le joueur accusé **a menti** → il devient **Liar**
* Si le joueur accusé **n’a pas menti** → l’accusateur devient **Liar**

---

### 🔫 Roulette russe

Le joueur désigné comme **Liar** doit jouer à la roulette russe :

* 🎯 1 chance sur 6 d’être **éliminé**
* Sinon, il reste en jeu

---

### 🏆 Fin de partie

La partie se termine lorsque :

* Il ne reste **qu’un seul joueur vivant**
* OU un joueur **n’a plus de cartes**

---

## 🛠️ Installation & Compilation

### 📦 Prérequis

Assurez-vous d’avoir :

* `gcc`
* `make`
* Bibliothèques :

  * `ncurses`
  * `pthread`
  * `rt`

Sur Debian/Ubuntu :

```bash
sudo apt install build-essential libncurses-dev
```

---

### ⚙️ Compilation

Dans le dossier du projet :

```bash
make
```

Cela génère :

* `executables/broker`
* `executables/player`

---

### 🧹 Nettoyage

```bash
make clean
```

---

## 🚀 Lancement

1. Démarrer le serveur :

```bash
./executables/broker
```

2. Lancer les joueurs (dans plusieurs terminaux) :

```bash
./executables/player
```

---

## 🧠 Architecture technique

### 🔁 Communication

| Composant        | Rôle                                |
| ---------------- | ----------------------------------- |
| Mémoire partagée | Stocke l’état du jeu                |
| Sémaphore nommé  | Protège l’accès concurrent          |
| Message Queue    | Envoi des actions joueurs → serveur |
| Signaux          | Notification de mise à jour         |

---

### 🖥️ Interface

* Interface en terminal via **Ncurses**
* Mise à jour dynamique selon les signaux du broker

---

## 📌 Remarques

* Maximum : **4 joueurs**
* Jeu entièrement en terminal
* Gestion concurrente des processus

---

## 👨‍💻 Auteur
Dupuis Mathéo
Soudani Esteban

---
