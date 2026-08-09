#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>              
#include "animal.h"
#include "cli.h"

ListeAnimaux* creerListe(void) {
    ListeAnimaux *liste = (ListeAnimaux*) malloc(sizeof(ListeAnimaux));
    if (liste == NULL) {
        perror("Erreur allocation liste");
        return NULL;
    }
    liste->tete = NULL;
    liste->compteur_id = 1;   /* Les IDs commencent a 1 */
    liste->taille = 0;
    return liste;
}
void libererListe(ListeAnimaux *liste) {
    if (liste == NULL) return;

    Animal *courant = liste->tete;
    while (courant != NULL) {
        Animal *tmp = courant;
        courant = courant->suivant;
        free(tmp);
    }
    free(liste);
}


static int trouverProchainIdLibre(const ListeAnimaux *liste) {
    int id = 1;
    while (rechercherAnimalParId(liste, id) != NULL) {
        id++;
    }
    return id;
}


int ajouterAnimal(ListeAnimaux *liste,
                  const char *nom, const char *espece, const char *race,
                  int age, const char *proprietaire) {
    if (liste == NULL) return 0;

    Animal *nouveau = (Animal*) malloc(sizeof(Animal));
    if (nouveau == NULL) {
        perror("Erreur allocation animal");
        return 0;
    }

   
    int id_attribue = trouverProchainIdLibre(liste);
    nouveau->id = id_attribue;

    
    if (id_attribue >= liste->compteur_id)
        liste->compteur_id = id_attribue + 1;

    strncpy(nouveau->nom,          nom,          MAX_NOM - 1);
    strncpy(nouveau->espece,       espece,       MAX_ESPECE - 1);
    strncpy(nouveau->race,         race,         MAX_RACE - 1);
    strncpy(nouveau->proprietaire, proprietaire, MAX_PROPRIETAIRE - 1);
    nouveau->nom[MAX_NOM - 1]          = '\0';
    nouveau->espece[MAX_ESPECE - 1]    = '\0';
    nouveau->race[MAX_RACE - 1]        = '\0';
    nouveau->proprietaire[MAX_PROPRIETAIRE - 1] = '\0';
    nouveau->age = age;

    
    nouveau->suivant = liste->tete;
    liste->tete = nouveau;
    liste->taille++;

    return nouveau->id;
}

int supprimerAnimal(ListeAnimaux *liste, int id) {
    if (liste == NULL || liste->tete == NULL) return 0;

    Animal *courant = liste->tete;
    Animal *precedent = NULL;

    while (courant != NULL) {
        if (courant->id == id) {
            if (precedent == NULL)
                liste->tete = courant->suivant;   /* 1er element */
            else
                precedent->suivant = courant->suivant;
            free(courant);
            liste->taille--;
            return 1;
        }
        precedent = courant;
        courant = courant->suivant;
    }
    return 0;  /* Non trouve */
}


Animal* rechercherAnimalParId(const ListeAnimaux *liste, int id) {
    if (liste == NULL) return NULL;

    Animal *courant = liste->tete;
    while (courant != NULL) {
        if (courant->id == id) return courant;
        courant = courant->suivant;
    }
    return NULL;
}


void afficherAnimal(const Animal *a) {
    if (a == NULL) {
        afficherErreur("Animal inexistant.");
        return;
    }
    printf("%s%-4d%s  %-15s %-10s %-15s %3d an(s)  %s\n",
           CLR_BOLD, a->id, CLR_RESET,
           a->nom, a->espece, a->race, a->age, a->proprietaire);
}


void afficherTousAnimaux(const ListeAnimaux *liste) {
    if (liste == NULL || liste->tete == NULL) {
        afficherInfo("Aucun animal enregistre pour le moment.");
        return;
    }
    afficherTitre("LISTE DES ANIMAUX ENREGISTRES");
    printf("%s%-4s  %-15s %-10s %-15s %-8s  %s%s\n",
           CLR_GRIS, "ID", "NOM", "ESPECE", "RACE", "AGE", "PROPRIETAIRE", CLR_RESET);
    afficherSeparateur('-', 80);

    Animal *courant = liste->tete;
    while (courant != NULL) {
        afficherAnimal(courant);
        courant = courant->suivant;
    }
    printf("\n%sTotal : %d animal(x)%s\n", CLR_GRIS, liste->taille, CLR_RESET);
}


void rechercherMulticritere(const ListeAnimaux *liste,
                            const char *espece, int age_max) {
    if (liste == NULL) return;

    int trouve = 0;
    char filtre_espece = (espece != NULL && espece[0] != '\0');
    char filtre_age     = (age_max > 0);

    afficherTitre("RESULTAT RECHERCHE MULTICRITERE");
    if (filtre_espece) printf("%sEspece : %s%s  ", CLR_GRIS, espece, CLR_RESET);
    if (filtre_age)    printf("%sAge max : %d ans%s", CLR_GRIS, age_max, CLR_RESET);
    if (!filtre_espece && !filtre_age)
        printf("%sAucun filtre actif - affichage de tous les animaux.%s",
               CLR_GRIS, CLR_RESET);
    printf("\n");
    afficherSeparateur('-', 80);

    Animal *courant = liste->tete;
    while (courant != NULL) {
        int ok = 1;
        if (filtre_espece && strcasecmp(courant->espece, espece) != 0)
            ok = 0;
        if (filtre_age && courant->age > age_max)
            ok = 0;
        if (ok) {
            afficherAnimal(courant);
            trouve++;
        }
        courant = courant->suivant;
    }

    if (trouve == 0)
        afficherErreur("Aucun animal ne correspond a ces criteres.");
    else
        printf("\n%s%d animal(s) trouve(s).%s\n",
               CLR_VERT, trouve, CLR_RESET);
}


Animal** trierParProprietaire(const ListeAnimaux *liste) {
    if (liste == NULL || liste->taille == 0) return NULL;

    /* 1. Allocation d'un tableau de pointeurs */
    Animal **tab = (Animal**) malloc(liste->taille * sizeof(Animal*));
    if (tab == NULL) return NULL;

    /* 2. Remplissage du tableau */
    Animal *courant = liste->tete;
    int i = 0;
    while (courant != NULL) {
        tab[i++] = courant;
        courant = courant->suivant;
    }

    /* 3. Tri bulle par nom de proprietaire (case-insensitive) */
    for (i = 0; i < liste->taille - 1; i++) {
        for (int j = 0; j < liste->taille - 1 - i; j++) {
            if (strcasecmp(tab[j]->proprietaire, tab[j+1]->proprietaire) > 0) {
                Animal *tmp = tab[j];
                tab[j] = tab[j+1];
                tab[j+1] = tmp;
            }
        }
    }
    return tab;
}


void afficherAnimauxTriees(Animal **tableau, int taille) {
    if (tableau == NULL || taille == 0) {
        afficherInfo("Aucun animal a trier.");
        return;
    }
    afficherTitre("ANIMAUX TRIES PAR PROPRIETAIRE (A -> Z)");
    printf("%s%-4s  %-15s %-10s %-15s %-8s  %s%s\n",
           CLR_GRIS, "ID", "NOM", "ESPECE", "RACE", "AGE", "PROPRIETAIRE", CLR_RESET);
    afficherSeparateur('-', 80);

    for (int i = 0; i < taille; i++) {
        afficherAnimal(tableau[i]);
    }
    printf("\n%sTotal : %d animal(x)%s\n", CLR_GRIS, taille, CLR_RESET);
}
