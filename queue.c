/* ==========================================================================
 *  queue.c  -  File d'attente a PRIORITE avec tickets FIFO
 *  Projet 11 : Clinique Veterinaire
 *
 *  INNOVATION PRINCIPALE DU PROJET
 *
 *  Regle de triage :
 *    1. Urgence la plus petite (1 = critique) sort en premier
 *    2. Si meme urgence, le ticket le plus petit (arrive en premier) sort
 *       en premier -> respect strict du FIFO
 *
 *  L'insertion (enqueue) parcourt la file pour trouver la bonne place :
 *  O(n) pour maintenir la liste triee -> Dequeue en O(1).
 * ==========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "cli.h"

/* -----------------------------------------------------------------------
 * creerFile : alloue et initialise une file vide
 * ----------------------------------------------------------------------- */
FileAttente* creerFile(void) {
    FileAttente *f = (FileAttente*) malloc(sizeof(FileAttente));
    if (f == NULL) {
        perror("Erreur allocation file");
        return NULL;
    }
    f->tete = NULL;
    f->queue = NULL;
    f->compteur_tickets = 0;
    f->taille = 0;
    return f;
}

/* -----------------------------------------------------------------------
 * libererFile : libere toute la memoire
 * ----------------------------------------------------------------------- */
void libererFile(FileAttente *f) {
    if (f == NULL) return;
    NoeudFile *courant = f->tete;
    while (courant != NULL) {
        NoeudFile *tmp = courant;
        courant = courant->suivant;
        free(tmp);
    }
    free(f);
}

/* -----------------------------------------------------------------------
 * estPrioritaireSur : compare 2 elements
 *   Retourne 1 si 'a' doit sortir AVANT 'b'.
 *   Regle : urgence plus petite gagne. Si egalite, ticket plus petit gagne.
 * ----------------------------------------------------------------------- */
static int estPrioritaireSur(const NoeudFile *a, const NoeudFile *b) {
    if (a->niveau_urgence != b->niveau_urgence)
        return a->niveau_urgence < b->niveau_urgence;
    return a->numero_ticket < b->numero_ticket;
}

/* -----------------------------------------------------------------------
 * enqueue : insertion triee O(n) dans la file
 *   L'element est insere AVANT le premier element qui lui est "inferieur"
 *   ou egal en priorite. Maintient ainsi l'ordre (urgence, ticket).
 *
 *   CAS 1 : file vide        -> devient tete ET queue
 *   CAS 2 : prioritaire sur tete -> nouvelle tete
 *   CAS 3 : cas general      -> parcours jusqu'a trouver sa place
 * ----------------------------------------------------------------------- */
int enqueue(FileAttente *f, int id_animal, int niveau_urgence) {
    if (f == NULL) return 0;
    if (niveau_urgence < 1 || niveau_urgence > 3) {
        afficherErreur("Niveau d'urgence invalide (1, 2 ou 3 uniquement).");
        return 0;
    }

    /* 1. Creation du nouveau patient */
    NoeudFile *nouveau = (NoeudFile*) malloc(sizeof(NoeudFile));
    if (nouveau == NULL) {
        perror("Erreur allocation file");
        return 0;
    }
    nouveau->id_animal     = id_animal;
    nouveau->niveau_urgence = niveau_urgence;
    nouveau->suivant       = NULL;

    /* 2. Attribution du ticket d'arrivee (FIFO) */
    f->compteur_tickets++;
    nouveau->numero_ticket = f->compteur_tickets;

    /* 3. Cas file vide */
    if (f->tete == NULL) {
        f->tete = nouveau;
        f->queue = nouveau;
        f->taille++;
        return 1;
    }

    /* 4. Cas ou le nouveau doit aller EN TETE */
    if (estPrioritaireSur(nouveau, f->tete)) {
        nouveau->suivant = f->tete;
        f->tete = nouveau;
        f->taille++;
        return 1;
    }

    /* 5. Cas general : parcours pour trouver la bonne place */
    NoeudFile *actuel = f->tete;
    while (actuel->suivant != NULL &&
           estPrioritaireSur(actuel->suivant, nouveau)) {
        actuel = actuel->suivant;
    }
    /* Insertion entre actuel et actuel->suivant */
    nouveau->suivant = actuel->suivant;
    actuel->suivant = nouveau;

    /* Si on l'a insere en fin de file, on met a jour le pointeur queue */
    if (nouveau->suivant == NULL)
        f->queue = nouveau;

    f->taille++;
    return 1;
}

/* -----------------------------------------------------------------------
 * dequeue : retrait en tete O(1)
 *   Comme la file est deja triee, le premier element est TOUJOURS
 *   le plus prioritaire (et le plus ancien a priorite egale).
 *   Retourne l'ID de l'animal sortant, ou -1 si file vide.
 * ----------------------------------------------------------------------- */
int dequeue(FileAttente *f) {
    if (f == NULL || f->tete == NULL) {
        return -1;
    }

    NoeudFile *a_supprimer = f->tete;
    int id_animal_sortant = a_supprimer->id_animal;

    f->tete = f->tete->suivant;
    if (f->tete == NULL)
        f->queue = NULL;   /* La file est maintenant vide */

    free(a_supprimer);
    f->taille--;
    return id_animal_sortant;
}

/* -----------------------------------------------------------------------
 * retirerDeFile : retire un animal precis de la file (annulation)
 *   Retourne 1 si OK, 0 si non trouve.
 * ----------------------------------------------------------------------- */
int retirerDeFile(FileAttente *f, int id_animal) {
    if (f == NULL || f->tete == NULL) return 0;

    NoeudFile *courant = f->tete;
    NoeudFile *precedent = NULL;

    while (courant != NULL) {
        if (courant->id_animal == id_animal) {
            if (precedent == NULL)
                f->tete = courant->suivant;
            else
                precedent->suivant = courant->suivant;
            if (courant == f->queue)
                f->queue = precedent;
            free(courant);
            f->taille--;
            return 1;
        }
        precedent = courant;
        courant = courant->suivant;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * afficherFile : affiche la file avec couleurs selon l'urgence
 * ----------------------------------------------------------------------- */
void afficherFile(const FileAttente *f, const ListeAnimaux *liste) {
    if (f == NULL) return;

    afficherTitre("FILE D'ATTENTE (PRIORITE)");
    if (f->tete == NULL) {
        afficherInfo("File d'attente vide.");
        return;
    }

    printf("%s%-4s  %-10s %-15s %-10s  %s%s\n",
           CLR_GRIS, "RANG", "URGENCE", "NOM", "TICKET", "PROPRIETAIRE", CLR_RESET);
    afficherSeparateur('-', 70);

    NoeudFile *courant = f->tete;
    int rang = 1;
    while (courant != NULL) {
        const char *couleur;
        const char *label;
        switch (courant->niveau_urgence) {
            case URGENCE_CRITIQUE: couleur = CLR_ROUGE;   label = "CRITIQUE"; break;
            case URGENCE_URGENT:   couleur = CLR_JAUNE;   label = "URGENT";   break;
            default:               couleur = CLR_VERT;    label = "NORMAL";   break;
        }
        /* Recuperer le nom de l'animal via la liste chainee */
        Animal *a = rechercherAnimalParId(liste, courant->id_animal);
        const char *nom = (a != NULL) ? a->nom : "<inconnu>";
        const char *prop = (a != NULL) ? a->proprietaire : "<inconnu>";

        printf("%s#%-3d%s  %s%-10s%s %-15s #%04d       %s\n",
               CLR_BOLD, rang, CLR_RESET,
               couleur, label, CLR_RESET,
               nom, courant->numero_ticket, prop);

        courant = courant->suivant;
        rang++;
    }
    printf("\n%sPatients en attente : %d%s\n",
           CLR_GRIS, f->taille, CLR_RESET);
}

/* -----------------------------------------------------------------------
 * estVideFile / teteFile : accesseurs
 * ----------------------------------------------------------------------- */
int estVideFile(const FileAttente *f) {
    return (f == NULL || f->tete == NULL);
}

NoeudFile* teteFile(const FileAttente *f) {
    return (f != NULL) ? f->tete : NULL;
}
