/* ==========================================================================

 *
 *  Architecture en couches :
 *     Couche 1 : Liste chainee (animal.c)
 *     Couche 1 : File de priorite avec tickets (queue.c)
 *     Couche 1 : Pile d'historique (stack.c)
 *     Couche 2 : Pont File -> Pile (consultation -> historique)
 *     Couche 3 : Interface CLI (cli.c)
 *     Couche 4 : Persistance (storage.c)
 *

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "animal.h"
#include "queue.h"
#include "stack.h"
#include "cli.h"
#include "storage.h"

/* -----------------------------------------------------------------------
 * Prototypes des fonctions du menu
 * ----------------------------------------------------------------------- */
static void actionAjouterAnimal(ListeAnimaux *liste);
static void actionSupprimerAnimal(ListeAnimaux *liste, FileAttente *file);
static void actionRechercherAnimal(ListeAnimaux *liste);
static void actionRechercherMulti(ListeAnimaux *liste);
static void actionTrierParProprietaire(ListeAnimaux *liste);
static void actionEnqueue(ListeAnimaux *liste, FileAttente *file);
static void actionDequeue(FileAttente *file, PileHistorique *pile,
                          ListeAnimaux *liste);
static void actionPopPile(PileHistorique *pile);

/* -----------------------------------------------------------------------
 * Programme principal
 * ----------------------------------------------------------------------- */
int main(void) {
    /* 1. Initialisation des 3 structures */
    ListeAnimaux   *liste = creerListe();
    FileAttente    *file  = creerFile();
    PileHistorique *pile  = creerPile();

    if (liste == NULL || file == NULL || pile == NULL) {
        fprintf(stderr, "Erreur fatale : impossible d'initialiser.\n");
        return EXIT_FAILURE;
    }

    /* 2. Chargement automatique au demarrage (couche 4) */
    spinner("Chargement des donnees", 500);
    if (chargerTout(liste, file, pile)) {
        afficherSucces("Donnees chargees depuis les fichiers.");
    } else {
        afficherInfo("Premier lancement - aucun fichier trouve.");
    }
    pauseApp();

    int choix;
    do {
        clearScreen();
        afficherLogo();
        afficherMenuPrincipal();
        choix = lireEntier("Votre choix", 0, 14);

        clearScreen();
        switch (choix) {
            /* --- Gestion des animaux --- */
            case 1: actionAjouterAnimal(liste);                       break;
            case 2: actionSupprimerAnimal(liste, file);               break;
            case 3: actionRechercherAnimal(liste);                    break;
            case 4: actionRechercherMulti(liste);                     break;
            case 5: afficherTousAnimaux(liste);                       break;
            case 6: actionTrierParProprietaire(liste);                break;

            /* --- File d'attente --- */
            case 7: actionEnqueue(liste, file);                       break;
            case 8: actionDequeue(file, pile, liste);                 break;
            case 9: afficherFile(file, liste);                        break;

            /* --- Historique --- */
            case 10: afficherPile(pile, liste);                       break;
            case 11: actionPopPile(pile);                             break;

            /* --- Systeme --- */
            case 12: afficherDashboard(liste, file, pile);            break;
            case 13:
                if (sauvegarderTout(liste, file, pile))
                    afficherSucces("Sauvegarde complete terminee.");
                else
                    afficherErreur("Echec de la sauvegarde.");
                break;
            case 14:
                /* Avant de recharger, on vide tout */
                viderPile(pile);
                while (!estVideFile(file)) dequeue(file);
                while (liste->tete != NULL) {
                    Animal *tmp = liste->tete;
                    liste->tete = tmp->suivant;
                    free(tmp);
                }
                liste->taille = 0;
                liste->compteur_id = 1;
                if (chargerTout(liste, file, pile))
                    afficherSucces("Donnees rechargees.");
                else
                    afficherErreur("Echec du chargement.");
                break;
            case 0:
                afficherInfo("Sauvegarde automatique avant fermeture...");
                sauvegarderTout(liste, file, pile);
                afficherSucces("Au revoir !");
                break;
            default:
                afficherErreur("Choix invalide.");
        }

        if (choix != 0) pauseApp();
    } while (choix != 0);

    /* Liberation memoire */
    libererListe(liste);
    libererFile(file);
    libererPile(pile);
    return EXIT_SUCCESS;
}

/* =====================================================================
 *  ACTIONS DU MENU
 * ===================================================================== */

/* ----- 1. Ajouter un animal ----- */
static void actionAjouterAnimal(ListeAnimaux *liste) {
    afficherTitre("AJOUTER UN ANIMAL");
    char nom[MAX_NOM], espece[MAX_ESPECE], race[MAX_RACE], prop[MAX_PROPRIETAIRE];
    int  age;

    lireChaine("Nom de l'animal", nom, MAX_NOM);
    lireChaine("Espece (Chien/Chat/Oiseau/...)", espece, MAX_ESPECE);
    lireChaine("Race", race, MAX_RACE);
    age = lireEntier("Age (en annees)", 0, 50);
    lireChaine("Nom du proprietaire", prop, MAX_PROPRIETAIRE);

    int id = ajouterAnimal(liste, nom, espece, race, age, prop);
    if (id > 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Animal ajoute avec l'ID %d.", id);
        afficherSucces(msg);
    } else {
        afficherErreur("Echec de l'ajout.");
    }
}

/* ----- 2. Supprimer un animal ----- */
static void actionSupprimerAnimal(ListeAnimaux *liste, FileAttente *file) {
    afficherTitre("SUPPRIMER UN ANIMAL");
    int id = lireEntier("ID de l'animal a supprimer", 1, 99999);

    Animal *a = rechercherAnimalParId(liste, id);
    if (a == NULL) {
        afficherErreur("Aucun animal avec cet ID.");
        return;
    }
    printf("Animal trouve : ");
    afficherAnimal(a);

    /* On le retire aussi de la file d'attente s'il y etait */
    if (retirerDeFile(file, id))
        afficherInfo("Animal retire de la file d'attente.");

    if (supprimerAnimal(liste, id))
        afficherSucces("Animal supprime de la liste.");
    else
        afficherErreur("Echec de la suppression.");
}

/* ----- 3. Rechercher par ID ----- */
static void actionRechercherAnimal(ListeAnimaux *liste) {
    afficherTitre("RECHERCHER UN ANIMAL PAR ID");
    int id = lireEntier("ID de l'animal", 1, 99999);
    Animal *a = rechercherAnimalParId(liste, id);
    if (a == NULL) {
        afficherErreur("Aucun animal avec cet ID.");
        return;
    }
    printf("%s%-4s  %-15s %-10s %-15s %-8s  %s%s\n",
           CLR_GRIS, "ID", "NOM", "ESPECE", "RACE", "AGE", "PROPRIETAIRE", CLR_RESET);
    afficherSeparateur('-', 80);
    afficherAnimal(a);
}

/* ----- 4. Recherche multicritere ----- */
static void actionRechercherMulti(ListeAnimaux *liste) {
    afficherTitre("RECHERCHE MULTICRITERE");
    char espece[MAX_ESPECE];
    printf("%s(Laissez vide pour ignorer le critere)%s\n", CLR_GRIS, CLR_RESET);

    printf("%sEspece%s (vide = tous) : ", CLR_BLEU, CLR_RESET);
    if (fgets(espece, MAX_ESPECE, stdin) == NULL) espece[0] = '\0';
    size_t len = strlen(espece);
    if (len > 0 && espece[len-1] == '\n') espece[len-1] = '\0';

    int age_max = lireEntier("Age max (0 = ignorer)", 0, 100);

    rechercherMulticritere(liste, espece, age_max);
}

/* ----- 6. Trier par proprietaire ----- */
static void actionTrierParProprietaire(ListeAnimaux *liste) {
    if (liste->taille == 0) {
        afficherInfo("Aucun animal a trier.");
        return;
    }
    Animal **tab = trierParProprietaire(liste);
    afficherAnimauxTriees(tab, liste->taille);
    free(tab);
}

/* ----- 7. Enqueue : ajout dans la file de priorite ----- */
static void actionEnqueue(ListeAnimaux *liste, FileAttente *file) {
    afficherTitre("AJOUTER UN PATIENT EN FILE D'ATTENTE");
    if (liste->taille == 0) {
        afficherErreur("Aucun animal enregistre. Ajoutez d'abord un animal.");
        return;
    }
    /* On affiche rapidement la liste des animaux pour faciliter le choix */
    printf("%sAnimaux disponibles :%s\n", CLR_GRIS, CLR_RESET);
    Animal *c = liste->tete;
    while (c != NULL) {
        printf("  %s#%d%s - %s (%s)\n", CLR_BOLD, c->id, CLR_RESET, c->nom, c->espece);
        c = c->suivant;
    }
    printf("\n");

    int id = lireEntier("ID de l'animal a ajouter en file", 1, 99999);
    Animal *a = rechercherAnimalParId(liste, id);
    if (a == NULL) {
        afficherErreur("Aucun animal avec cet ID.");
        return;
    }
    int urgence = lireUrgence();
    if (enqueue(file, id, urgence)) {
        char msg[120];
        snprintf(msg, sizeof(msg),
                 "Patient ajoute en file. Urgence=%d, Ticket=#%04d.",
                 urgence, file->compteur_tickets);
        afficherSucces(msg);
    } else {
        afficherErreur("Echec de l'ajout en file.");
    }
}

/* ----- 8. Dequeue : passage en consultation (PONT FILE -> PILE) ----- */
static void actionDequeue(FileAttente *file, PileHistorique *pile,
                          ListeAnimaux *liste) {
    afficherTitre("PASSAGE EN CONSULTATION (DEQUEUE)");
    if (estVideFile(file)) {
        afficherInfo("File d'attente vide - aucun patient a consulter.");
        return;
    }
    /* On montre le prochain patient */
    NoeudFile *prochain = teteFile(file);
    Animal *a = rechercherAnimalParId(liste, prochain->id_animal);
    printf("%sProchain patient :%s %s (%s) - Urgence %d, Ticket #%04d\n",
           CLR_JAUNE, CLR_RESET,
           a ? a->nom : "<inconnu>",
           a ? a->espece : "?",
           prochain->niveau_urgence,
           prochain->numero_ticket);

    /* PONT AUTOMATIQUE : on demande le soin et on empile */
    printf("\n%sQuel soin a ete effectue ?%s\n", CLR_BLEU, CLR_RESET);
    printf("  (ex: Vaccin antirabique, Visite de controle, Chirurgie...)\n");
    char soin[MAX_DESC_SOIN];
    lireChaine("Description du soin", soin, MAX_DESC_SOIN);

    int id_sortant = dequeue(file);
    if (id_sortant < 0) {
        afficherErreur("Erreur : file vide a la sortie.");
        return;
    }
    /* Empile dans la pile d'historique (couche 2 : pont) */
    if (push(pile, id_sortant, soin)) {
        char msg[150];
        snprintf(msg, sizeof(msg),
                 "Patient consulte et soin empile dans l'historique (total soins : %d).",
                 pile->taille);
        afficherSucces(msg);
    } else {
        afficherErreur("Consultation OK mais erreur d'enregistrement du soin.");
    }
}

/* ----- 11. Pop : annuler le dernier soin ----- */
static void actionPopPile(PileHistorique *pile) {
    afficherTitre("ANNULER LE DERNIER SOIN (POP)");
    if (estVidePile(pile)) {
        afficherInfo("Pile vide - rien a annuler.");
        return;
    }
    int id;
    char description[MAX_DESC_SOIN];
    if (pop(pile, &id, description)) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "Soin annule : '%s' (animal #%d). Soin(s) restant(s) : %d.",
                 description, id, pile->taille);
        afficherSucces(msg);
    } else {
        afficherErreur("Echec de l'annulation.");
    }
}
