/* ==========================================================================
 *  stack.c  -  Pile d'historique des soins effectues
 *  Projet 11 : Clinique Veterinaire
 *
 *  Couche 1 : Pile (ajoutee par le groupe)
 *  Couche 2 : Pont automatique File -> Pile (consultation -> historique)
 *
 *  La pile est une simple liste chainee ou on insere et retire
 *  toujours en TETE (LIFO).
 * ==========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "stack.h"
#include "cli.h"

/* -----------------------------------------------------------------------
 * creerPile : alloue et initialise une pile vide
 * ----------------------------------------------------------------------- */
PileHistorique* creerPile(void) {
    PileHistorique *p = (PileHistorique*) malloc(sizeof(PileHistorique));
    if (p == NULL) {
        perror("Erreur allocation pile");
        return NULL;
    }
    p->sommet = NULL;
    p->taille = 0;
    return p;
}

/* -----------------------------------------------------------------------
 * libererPile : libere toute la memoire
 * ----------------------------------------------------------------------- */
void libererPile(PileHistorique *p) {
    if (p == NULL) return;
    viderPile(p);
    free(p);
}

/* -----------------------------------------------------------------------
 * viderPile : vide la pile sans liberer la structure
 * ----------------------------------------------------------------------- */
void viderPile(PileHistorique *p) {
    if (p == NULL) return;
    NoeudPile *courant = p->sommet;
    while (courant != NULL) {
        NoeudPile *tmp = courant;
        courant = courant->suivant;
        free(tmp);
    }
    p->sommet = NULL;
    p->taille = 0;
}

/* -----------------------------------------------------------------------
 * push : empile un soin (LIFO - insertion en tete) O(1)
 *   Genere automatiquement un horodatage "JJ/MM HH:MM".
 * ----------------------------------------------------------------------- */
int push(PileHistorique *p, int id_animal, const char *description) {
    if (p == NULL || description == NULL) return 0;

    NoeudPile *nouveau = (NoeudPile*) malloc(sizeof(NoeudPile));
    if (nouveau == NULL) {
        perror("Erreur allocation pile");
        return 0;
    }
    nouveau->id_animal = id_animal;
    strncpy(nouveau->description, description, MAX_DESC_SOIN - 1);
    nouveau->description[MAX_DESC_SOIN - 1] = '\0';

    /* Horodatage automatique */
    time_t maintenant = time(NULL);
    struct tm *t = localtime(&maintenant);
    strftime(nouveau->horodatage, sizeof(nouveau->horodatage),
             "%d/%m %H:%M", t);

    nouveau->suivant = p->sommet;
    p->sommet = nouveau;
    p->taille++;
    return 1;
}

/* -----------------------------------------------------------------------
 * pop : depile le dernier soin (LIFO - retrait en tete) O(1)
 *   Retourne 1 si OK (remplit id_animal et description),
 *   0 si la pile etait vide.
 * ----------------------------------------------------------------------- */
int pop(PileHistorique *p, int *id_animal, char *description) {
    if (p == NULL || p->sommet == NULL) return 0;

    NoeudPile *a_supprimer = p->sommet;
    if (id_animal != NULL)   *id_animal = a_supprimer->id_animal;
    if (description != NULL) strcpy(description, a_supprimer->description);

    p->sommet = a_supprimer->suivant;
    free(a_supprimer);
    p->taille--;
    return 1;
}

/* -----------------------------------------------------------------------
 * estVidePile : accesseur
 * ----------------------------------------------------------------------- */
int estVidePile(const PileHistorique *p) {
    return (p == NULL || p->sommet == NULL);
}

/* -----------------------------------------------------------------------
 * afficherPile : affiche l'historique du plus recent au plus ancien
 * ----------------------------------------------------------------------- */
void afficherPile(const PileHistorique *p, const ListeAnimaux *liste) {
    if (p == NULL) return;

    afficherTitre("HISTORIQUE DES SOINS (PILE - LIFO)");
    if (p->sommet == NULL) {
        afficherInfo("Aucun soin enregistre pour le moment.");
        return;
    }

    printf("%s%-10s %-15s %-20s %s%s\n",
           CLR_GRIS, "HORODATAGE", "ANIMAL", "SOIN EFFECTUE", "PROPRIETAIRE", CLR_RESET);
    afficherSeparateur('-', 75);

    NoeudPile *courant = p->sommet;
    while (courant != NULL) {
        Animal *a = rechercherAnimalParId(liste, courant->id_animal);
        const char *nom = (a != NULL) ? a->nom : "<inconnu>";
        const char *prop = (a != NULL) ? a->proprietaire : "<inconnu>";

        printf("%s%-10s%s %-15s %-20s %s\n",
               CLR_MAGENTA, courant->horodatage, CLR_RESET,
               nom, courant->description, prop);

        courant = courant->suivant;
    }
    printf("\n%sTotal : %d soin(s) enregistre(s)%s\n",
           CLR_GRIS, p->taille, CLR_RESET);
}
