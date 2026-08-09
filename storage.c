/* ==========================================================================
 *  storage.c  -  Persistance des donnees (fichiers CSV)
 *  Projet 11 : Clinique Veterinaire
 *
 *  Couche 4 : Sauvegarde / Chargement
 *
 *  Format CSV simple (separateur ';') :
 *     animaux.csv     : id;nom;espece;race;age;proprietaire
 *     file_attente.csv: id_animal;niveau_urgence;numero_ticket
 *     historique.csv  : id_animal;description;horodatage
 *
 *  NOTE : La pile est sauvegardee dans l'ordre inverse de la file
 *  (LIFO -> on empile en relisant dans l'ordre naturel du fichier,
 *  donc le fichier contient du plus ancien au plus recent).
 * ==========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "storage.h"
#include "cli.h"

/* Cree le dossier data/ s'il n'existe pas */
static void assurerDossierData(void) {
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0755);
#endif
}

/* Supprime les sauts de ligne finaux d'une ligne lue par fgets */
static void tronquerFinLigne(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';
    }
}

/* -----------------------------------------------------------------------
 * SAUVEGARDE
 * ----------------------------------------------------------------------- */

int sauvegarderAnimaux(const ListeAnimaux *liste) {
    if (liste == NULL) return 0;
    assurerDossierData();
    FILE *f = fopen(FICHIER_ANIMAUX, "w");
    if (f == NULL) {
        perror("Ouverture animaux.csv");
        return 0;
    }
    fprintf(f, "# id;nom;espece;race;age;proprietaire\n");
    fprintf(f, "# compteur_id=%d\n", liste->compteur_id);

    Animal *courant = liste->tete;
    while (courant != NULL) {
        fprintf(f, "%d;%s;%s;%s;%d;%s\n",
                courant->id, courant->nom, courant->espece,
                courant->race, courant->age, courant->proprietaire);
        courant = courant->suivant;
    }
    fclose(f);
    return 1;
}

int sauvegarderFile(const FileAttente *f) {
    if (f == NULL) return 0;
    assurerDossierData();
    FILE *file = fopen(FICHIER_FILE, "w");
    if (file == NULL) {
        perror("Ouverture file_attente.csv");
        return 0;
    }
    fprintf(file, "# id_animal;niveau_urgence;numero_ticket\n");
    fprintf(file, "# compteur_tickets=%d\n", f->compteur_tickets);

    NoeudFile *courant = f->tete;
    /* On sauvegarde dans l'ordre naturel (tete -> queue) qui est l'ordre
     * de priorite. Au chargement on lira dans le meme ordre. */
    while (courant != NULL) {
        fprintf(file, "%d;%d;%d\n",
                courant->id_animal, courant->niveau_urgence,
                courant->numero_ticket);
        courant = courant->suivant;
    }
    fclose(file);
    return 1;
}

int sauvegarderHistorique(const PileHistorique *p) {
    if (p == NULL) return 0;
    assurerDossierData();
    FILE *f = fopen(FICHIER_HISTORIQUE, "w");
    if (f == NULL) {
        perror("Ouverture historique.csv");
        return 0;
    }
    fprintf(f, "# id_animal;description;horodatage\n");

    /* Pour conserver l'ordre LIFO au chargement, on doit ecrire du
     * plus ancien au plus recent. On va donc d'abord compter, puis
     * utiliser un tableau temporaire pour inverser. */
    int n = p->taille;
    if (n > 0) {
        NoeudPile **tab = (NoeudPile**) malloc(n * sizeof(NoeudPile*));
        if (tab != NULL) {
            NoeudPile *courant = p->sommet;
            int i = n - 1;   /* on remplit depuis la fin */
            while (courant != NULL && i >= 0) {
                tab[i--] = courant;
                courant = courant->suivant;
            }
            for (i = 0; i < n; i++) {
                fprintf(f, "%d;%s;%s\n",
                        tab[i]->id_animal, tab[i]->description,
                        tab[i]->horodatage);
            }
            free(tab);
        }
    }
    fclose(f);
    return 1;
}

int sauvegarderTout(const ListeAnimaux *liste,
                    const FileAttente   *file,
                    const PileHistorique *pile) {
    spinner("Sauvegarde des animaux", 400);
    int ok1 = sauvegarderAnimaux(liste);
    spinner("Sauvegarde de la file d'attente", 400);
    int ok2 = sauvegarderFile(file);
    spinner("Sauvegarde de l'historique", 400);
    int ok3 = sauvegarderHistorique(pile);
    return ok1 && ok2 && ok3;
}

/* -----------------------------------------------------------------------
 * CHARGEMENT
 * ----------------------------------------------------------------------- */

int chargerAnimaux(ListeAnimaux *liste) {
    if (liste == NULL) return 0;
    FILE *f = fopen(FICHIER_ANIMAUX, "r");
    if (f == NULL) return 0;  /* Fichier inexistant : OK */

    char ligne[256];
    int compteur_id_lu = 0;
    while (fgets(ligne, sizeof(ligne), f) != NULL) {
        tronquerFinLigne(ligne);
        if (ligne[0] == '#' || ligne[0] == '\0') {
            /* Ligne speciale : on cherche compteur_id */
            if (strncmp(ligne, "# compteur_id=", 14) == 0) {
                sscanf(ligne + 14, "%d", &compteur_id_lu);
            }
            continue;
        }
        int id, age;
        char nom[MAX_NOM], espece[MAX_ESPECE], race[MAX_RACE], prop[MAX_PROPRIETAIRE];
        if (sscanf(ligne, "%d;%49[^;];%29[^;];%29[^;];%d;%49[^\n]",
                   &id, nom, espece, race, &age, prop) == 6) {
            /* Insertion directe sans reutiliser ajouterAnimal (car on
             * veut preserver l'ID original). */
            Animal *nouveau = (Animal*) malloc(sizeof(Animal));
            if (nouveau == NULL) continue;
            nouveau->id = id;
            strncpy(nouveau->nom, nom, MAX_NOM - 1); nouveau->nom[MAX_NOM-1] = '\0';
            strncpy(nouveau->espece, espece, MAX_ESPECE - 1); nouveau->espece[MAX_ESPECE-1] = '\0';
            strncpy(nouveau->race, race, MAX_RACE - 1); nouveau->race[MAX_RACE-1] = '\0';
            strncpy(nouveau->proprietaire, prop, MAX_PROPRIETAIRE - 1);
            nouveau->proprietaire[MAX_PROPRIETAIRE-1] = '\0';
            nouveau->age = age;
            nouveau->suivant = liste->tete;
            liste->tete = nouveau;
            liste->taille++;
            if (id >= liste->compteur_id) liste->compteur_id = id + 1;
        }
    }
    if (compteur_id_lu > liste->compteur_id) liste->compteur_id = compteur_id_lu;
    fclose(f);
    return 1;
}

int chargerFile(FileAttente *f) {
    if (f == NULL) return 0;
    FILE *file = fopen(FICHIER_FILE, "r");
    if (file == NULL) return 0;

    char ligne[256];
    int compteur_tickets_lu = 0;
    while (fgets(ligne, sizeof(ligne), file) != NULL) {
        tronquerFinLigne(ligne);
        if (ligne[0] == '#' || ligne[0] == '\0') {
            if (strncmp(ligne, "# compteur_tickets=", 19) == 0) {
                sscanf(ligne + 19, "%d", &compteur_tickets_lu);
            }
            continue;
        }
        int id_animal, urgence, ticket;
        if (sscanf(ligne, "%d;%d;%d", &id_animal, &urgence, &ticket) == 3) {
            /* On reconstruit chaque maillon manuellement (et non via
             * enqueue) pour preserver le numero_ticket exact. On
             * l'insere en queue car le fichier est deja trie. */
            NoeudFile *nouveau = (NoeudFile*) malloc(sizeof(NoeudFile));
            if (nouveau == NULL) continue;
            nouveau->id_animal = id_animal;
            nouveau->niveau_urgence = urgence;
            nouveau->numero_ticket = ticket;
            nouveau->suivant = NULL;
            if (f->tete == NULL) {
                f->tete = nouveau;
                f->queue = nouveau;
            } else {
                f->queue->suivant = nouveau;
                f->queue = nouveau;
            }
            f->taille++;
            if (ticket > f->compteur_tickets) f->compteur_tickets = ticket;
        }
    }
    if (compteur_tickets_lu > f->compteur_tickets)
        f->compteur_tickets = compteur_tickets_lu;
    fclose(file);
    return 1;
}

int chargerHistorique(PileHistorique *p) {
    if (p == NULL) return 0;
    FILE *f = fopen(FICHIER_HISTORIQUE, "r");
    if (f == NULL) return 0;

    char ligne[256];
    /* Comme le fichier est ecrit du plus ancien au plus recent, on
     * peut appeler push() dans l'ordre naturel : le plus ancien se
     * retrouve en bas de pile, le plus recent en sommet. */
    while (fgets(ligne, sizeof(ligne), f) != NULL) {
        tronquerFinLigne(ligne);
        if (ligne[0] == '#' || ligne[0] == '\0') continue;
        int id_animal;
        char description[MAX_DESC_SOIN], horodatage[20];
        if (sscanf(ligne, "%d;%99[^;];%19[^\n]",
                   &id_animal, description, horodatage) == 3) {
            /* On utilise push() mais on ecrase l'horodatage avec
             * celui du fichier. */
            push(p, id_animal, description);
            /* Recopie manuelle de l'horodatage */
            if (p->sommet != NULL) {
                strncpy(p->sommet->horodatage, horodatage,
                        sizeof(p->sommet->horodatage) - 1);
                p->sommet->horodatage[sizeof(p->sommet->horodatage) - 1] = '\0';
            }
        }
    }
    fclose(f);
    return 1;
}

int chargerTout(ListeAnimaux *liste,
                FileAttente  *file,
                PileHistorique *pile) {
    int ok1 = chargerAnimaux(liste);
    int ok2 = chargerFile(file);
    int ok3 = chargerHistorique(pile);
    /* Si aucun fichier n'existait, c'est juste un 1er lancement */
    return ok1 || ok2 || ok3;
}
