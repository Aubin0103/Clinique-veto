/* ==========================================================================
 *  cli.c  -  Interface utilisateur : couleurs, ASCII art, validation
 *  Projet 11 : Clinique Veterinaire
 *
 *  Couche 3 : Interface et Experience (Polish)
 * ==========================================================================
 */
#define _DEFAULT_SOURCE   /* Pour strcasecmp() et usleep() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <strings.h>             
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "cli.h"
#include "animal.h"

/* -----------------------------------------------------------------------
 * barreAscii : genere une chaine de 10 caracteres representant une
 * proportion (utilisee pour le dashboard statistique).
 *   Exemple : 5/10 -> "*****     "
 * ----------------------------------------------------------------------- */
static char buffer_barre[11];

static const char* barreAscii(int valeur, int max) {
    if (max <= 0) max = 1;
    int nb_pleins = (valeur * 10) / max;
    if (nb_pleins > 10) nb_pleins = 10;
    for (int i = 0; i < 10; i++) {
        buffer_barre[i] = (i < nb_pleins) ? '*' : ' ';
    }
    buffer_barre[10] = '\0';
    return buffer_barre;
}

/* -----------------------------------------------------------------------
 * clearScreen : efface l'ecran (multi-plateforme)
 * ----------------------------------------------------------------------- */
void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* -----------------------------------------------------------------------
 * afficherLogo : ASCII art "CLINIQUE VETO"
 * ----------------------------------------------------------------------- */
void afficherLogo(void) {
    
    printf("\n");
    printf("  ____                                 _   ___  \n");
    printf(" / ___|_ __ ___  _   _ _ __   ___     / | / _ \\ \n");
    printf("| |   | '__/ _ \\| | | | '_ \\ / _ \\    | || | | |\n");
    printf("| |___| | | (_) | |_| | |_) |  __/    | || |_| |\n");
    printf(" \\____|_|  \\___/ \\__,_| .__/ \\___|    |_(_)___/ \n");
    printf("                      |_|                       \n");
    printf("\n");
}

/* -----------------------------------------------------------------------
 * afficherSeparateur : ligne de 'n' caracteres 'c'
 * ----------------------------------------------------------------------- */
void afficherSeparateur(char c, int n) {
    for (int i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

/* -----------------------------------------------------------------------
 * afficherTitre : titre encadre (avec barres au-dessus et au-dessous)
 * ----------------------------------------------------------------------- */
void afficherTitre(const char *titre) {
    int len = strlen(titre);
    printf("\n%s", CLR_TITRE);
    afficherSeparateur('=', len + 4);
    printf("| %s |\n", titre);
    afficherSeparateur('=', len + 4);
    printf("%s\n", CLR_RESET);
}

/* -----------------------------------------------------------------------
 * pauseApp : attend que l'utilisateur appuie sur Entree
 *   En mode non-interactif (stdin redirige depuis un fichier), ne fait
 *   rien pour eviter de consommer l'entree suivante du menu.
 * ----------------------------------------------------------------------- */
void pauseApp(void) {
    /* Si on n'est pas sur un terminal (ex: tests automatises via fichier),
     * on ne bloque pas. */
    if (!isatty(STDIN_FILENO)) {
        return;
    }
    printf("%s\n[Appuyez sur Entree pour continuer...]%s",
           CLR_GRIS, CLR_RESET);
    fflush(stdout);
    int c;
    /* Consomme tout jusqu'au prochain '\n' (ou EOF) */
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* -----------------------------------------------------------------------
 * spinner : simule un chargement (effet visuel)
 *   Le caractere tourne | / - \ pendant 'ms' millisecondes.
 * ----------------------------------------------------------------------- */
void spinner(const char *message, int ms) {
    const char chars[] = { '|', '/', '-', '\\' };
    int nb_etapes = 10;
    int delai_par_etape = ms / nb_etapes;
    if (delai_par_etape < 20) delai_par_etape = 20;

    /* En mode non-interactif, on skip l'animation pour ne pas ralentir les tests */
    if (!isatty(STDIN_FILENO)) {
        printf("%s%s%s %sOK%s\n", CLR_JAUNE, message, CLR_RESET, CLR_VERT, CLR_RESET);
        return;
    }

    printf("%s%s%s ", CLR_JAUNE, message, CLR_RESET);
    fflush(stdout);
    for (int i = 0; i < nb_etapes; i++) {
        printf("%c\b", chars[i % 4]);
        fflush(stdout);
#ifdef _WIN32
        Sleep(delai_par_etape);
#else
        usleep(delai_par_etape * 1000);
#endif
    }
    printf("%sOK%s\n", CLR_VERT, CLR_RESET);
}

/* -----------------------------------------------------------------------
 * lireEntier : saisie SECURISEE d'un entier
 *   - Utilise fgets() + sscanf() pour eviter les boucles infinies
 *     lorsque l'utilisateur tape du texte.
 *   - Verifie l'intervalle [min, max].
 *   - Repose la question tant que la saisie est invalide.
 *   - Affiche l'erreur en rouge.
 * ----------------------------------------------------------------------- */
int lireEntier(const char *invite, int min, int max) {
    char buffer[100];
    int valeur;
    int ok = 0;

    do {
        printf("%s%s%s ", CLR_BLEU, invite, CLR_RESET);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            /* EOF (Ctrl+D) - on renvoie une valeur invalide */
            return min - 1;
        }
        /* Si la ligne ne tient pas dans le buffer, on vide le reste */
        if (strchr(buffer, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
        }

        /* sscanf verifie qu'on a bien un entier */
        char reste[10];
        if (sscanf(buffer, "%d%9s", &valeur, reste) == 1) {
            if (valeur >= min && valeur <= max) {
                ok = 1;
            } else {
                printf("%sErreur : le nombre doit etre entre %d et %d.%s\n",
                       CLR_ERR, min, max, CLR_RESET);
            }
        } else {
            printf("%sErreur : veuillez entrer un nombre valide.%s\n",
                   CLR_ERR, CLR_RESET);
        }
    } while (!ok);

    return valeur;
}

/* -----------------------------------------------------------------------
 * lireChaine : saisie SECURISEE d'une chaine non vide
 * ----------------------------------------------------------------------- */
void lireChaine(const char *invite, char *buffer, int max_len) {
    do {
        printf("%s%s%s ", CLR_BLEU, invite, CLR_RESET);
        if (fgets(buffer, max_len, stdin) == NULL) {
            buffer[0] = '\0';
            return;
        }
        /* Retirer le '\n' final */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        /* Si la ligne ne tient pas dans le buffer, on vide le reste */
        if (len == (size_t)(max_len - 1) && buffer[len - 1] != '\n') {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
        }
        if (buffer[0] == '\0') {
            printf("%sErreur : la valeur ne peut pas etre vide.%s\n",
                   CLR_ERR, CLR_RESET);
        }
    } while (buffer[0] == '\0');
}

/* -----------------------------------------------------------------------
 * lireUrgence : saisie d'un niveau d'urgence
 * ----------------------------------------------------------------------- */
int lireUrgence(void) {
    printf("%sNiveau d'urgence :%s\n", CLR_BLEU, CLR_RESET);
    printf("  %s1.%s CRITIQUE  (vie en jeu, passage immediat)\n", CLR_ROUGE, CLR_RESET);
    printf("  %s2.%s URGENT    (douleur, besoin rapide)\n",      CLR_JAUNE, CLR_RESET);
    printf("  %s3.%s NORMAL    (consultation de routine)\n",     CLR_VERT,  CLR_RESET);
    return lireEntier("Votre choix", 1, 3);
}

/* -----------------------------------------------------------------------
 * afficherDashboard : statistiques visuelles ASCII avec barres
 * ----------------------------------------------------------------------- */
void afficherDashboard(const ListeAnimaux *liste,
                       const FileAttente  *file,
                       const PileHistorique *pile) {
    afficherTitre("DASHBOARD - STATISTIQUES");

    if (liste == NULL || liste->taille == 0) {
        afficherInfo("Aucune donnee a afficher.");
        return;
    }

    /* Compter les especes (chien, chat, oiseau, autre) */
    int chiens = 0, chats = 0, oiseaux = 0, autres = 0;
    Animal *courant = liste->tete;
    while (courant != NULL) {
        if (strcasecmp(courant->espece, "chien") == 0)   chiens++;
        else if (strcasecmp(courant->espece, "chat") == 0)  chats++;
        else if (strcasecmp(courant->espece, "oiseau") == 0) oiseaux++;
        else autres++;
        courant = courant->suivant;
    }
    int total = liste->taille;

    /* Calcul du max pour echelle des barres (10 caracteres max) */
    int max = chiens;
    if (chats   > max) max = chats;
    if (oiseaux > max) max = oiseaux;
    if (autres  > max) max = autres;
    if (max == 0) max = 1;

    /* Affichage avec barres ASCII */
    printf("\n%sRepartition par espece :%s\n\n", CLR_SOUS_TITRE, CLR_RESET);
    printf("  Chiens   : [%-10s] %s%d%s\n",
           barreAscii(chiens, max),  CLR_VERT,  chiens,  CLR_RESET);
    printf("  Chats    : [%-10s] %s%d%s\n",
           barreAscii(chats, max),   CLR_JAUNE, chats,   CLR_RESET);
    printf("  Oiseaux  : [%-10s] %s%d%s\n",
           barreAscii(oiseaux, max), CLR_CYAN,  oiseaux, CLR_RESET);
    printf("  Autres   : [%-10s] %s%d%s\n",
           barreAscii(autres, max),  CLR_GRIS,  autres,  CLR_RESET);

    printf("\n%sTotal animaux enregistres :%s %d\n",
           CLR_SOUS_TITRE, CLR_RESET, total);

    printf("%sTaille de la file d'attente :%s %s%d%s (tickets distribues : %d)\n",
           CLR_SOUS_TITRE, CLR_RESET,
           CLR_ROUGE, file ? file->taille : 0, CLR_RESET,
           file ? file->compteur_tickets : 0);

    printf("%sSoins effectues (pile) :%s %s%d%s\n\n",
           CLR_SOUS_TITRE, CLR_RESET,
           CLR_MAGENTA, pile ? pile->taille : 0, CLR_RESET);
}

void afficherMenuPrincipal(void) {
    afficherTitre("MENU PRINCIPAL - CLINIQUE VETERINAIRE");
    printf("%s--- Gestion des animaux  ---%s\n", CLR_SOUS_TITRE, CLR_RESET);
    printf("  %s1.%s  Ajouter un animal\n", CLR_MENU, CLR_RESET);
    printf("  %s2.%s  Supprimer un animal\n", CLR_MENU, CLR_RESET);
    printf("  %s3.%s  Rechercher un animal par ID\n", CLR_MENU, CLR_RESET);
    printf("  %s4.%s  Rechercher multicritere (espece + age)\n", CLR_MENU, CLR_RESET);
    printf("  %s5.%s  Afficher tous les animaux\n", CLR_MENU, CLR_RESET);
    printf("  %s6.%s  Trier par proprietaire (A -> Z)\n", CLR_MENU, CLR_RESET);
    printf("\n%s--- File d'attente  ---%s\n", CLR_SOUS_TITRE, CLR_RESET);
    printf("  %s7.%s  Ajouter un patient en file \n", CLR_MENU, CLR_RESET);
    printf("  %s8.%s  Faire passer le prochain patient \n", CLR_MENU, CLR_RESET);
    printf("  %s9.%s  Afficher la file d'attente\n", CLR_MENU, CLR_RESET);
    printf("\n%s--- Historique  ---%s\n", CLR_SOUS_TITRE, CLR_RESET);
    printf(" %s10.%s  Afficher l'historique des soins\n", CLR_MENU, CLR_RESET);
    printf(" %s11.%s  Annuler le dernier soin \n", CLR_MENU, CLR_RESET);
    printf("\n%s--- Systeme ---%s\n", CLR_SOUS_TITRE, CLR_RESET);
    printf(" %s12.%s  Afficher le dashboard statistique\n", CLR_MENU, CLR_RESET);
    printf(" %s13.%s  Sauvegarder dans les fichiers\n", CLR_MENU, CLR_RESET);
    printf(" %s14.%s  Recharger les donnees depuis les fichiers\n", CLR_MENU, CLR_RESET);
    printf("  %s0.%s  %sQuitter%s (sauvegarde auto)\n",
           CLR_MENU, CLR_RESET, CLR_ROUGE, CLR_RESET);
    afficherSeparateur('-', 50);
}

void afficherErreur(const char *msg) {
    printf("%s[ERREUR] %s%s\n", CLR_ERR, msg, CLR_RESET);
}

void afficherErreuranimal(const char *msg) {
    /* Alias - garde pour compat */
    afficherErreur(msg);
}

void afficherSucces(const char *msg) {
    printf("%s[OK] %s%s\n", CLR_OK, msg, CLR_RESET);
}

void afficherInfo(const char *msg) {
    printf("%s[INFO] %s%s\n", CLR_BLEU, msg, CLR_RESET);
}
