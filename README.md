# Clinique-veto
Clinique Véto est un logiciel complet de gestion destiné aux cliniques et cabinets vétérinaires. Il permet de centraliser le suivi des animaux et de leurs propriétaires, de gérer les dossiers médicaux (consultations, vaccinations, traitements, antécédents), ainsi que la planification des rendez-vous et des rappels. 

# Clinique Vétérinaire — Projet 11

**Burkina Rescue Challenge 2026**
Prototype C — Structures de données et Algorithmique Dynamique

---

## Objectif du projet

Application en langage C illustrant les **3 structures** vues en cours :

| Structure | Rôle dans la clinique |
|-----------|----------------------|
| **Liste chaînée** | Fiches des animaux (ID, nom, espèce, race, âge, propriétaire) |
| **File d'attente** | File de **priorité** avec tickets FIFO pour les consultations |
| **Pile** | Historique des soins effectués (ajoutée par le groupe) |

---

## Architecture en couches

```
┌─────────────────────────────────────────────────────────┐
│ Couche 4 : Persistance (storage.c)                      │
│   Sauvegarde/chargement automatique en CSV              │
├─────────────────────────────────────────────────────────┤
│ Couche 3 : Interface CLI (cli.c)                        │
│   Couleurs ANSI, ASCII art, validation, dashboard       │
├─────────────────────────────────────────────────────────┤
│ Couche 2 : Innovations                                  │
│   • File de priorité (urgence + ticket FIFO)            │
│   • Pont automatique File → Pile (consultation→soin)    │
│   • Recherche multicritère + Tri                        │
├─────────────────────────────────────────────────────────┤
│ Couche 1 : Structures de données de base                │
│   animal.c (Liste)  queue.c (File)  stack.c (Pile)      │
└─────────────────────────────────────────────────────────┘
```

---

## Innovations implémentées (vs cahier des charges minimal)

1. **File de priorité avec tickets FIFO**
   - Règle 1 : urgence la plus petite sort en premier (1 = critique)
   - Règle 2 : si même urgence, le ticket d'arrivée le plus petit sort en premier
   - `enqueue()` en O(n), `dequeue()` en O(1)
2. **Pont automatique File → Pile**
   - Quand on appelle `dequeue()`, le programme demande le soin effectué et empile automatiquement l'entrée dans la pile d'historique
3. **Recherche multicritère** : par espèce + âge maximum
4. **Tri par propriétaire** (A → Z) sans modifier la liste en mémoire (tri sur tableau de pointeurs)
5. **Dashboard ASCII** avec barres de progression par espèce
6. **Couleurs ANSI** : vert (succès), rouge (erreur), cyan (menus), jaune (urgences)
7. **Validation blindée des saisies** : `lireEntier()` et `lireChaine()` basés sur `fgets()` + `sscanf()` — impossible de planter le programme avec une lettre
8. **Logo ASCII** au démarrage
9. **Spinner de chargement** lors des sauvegardes
10. **Persistance CSV** automatique au démarrage et à la fermeture

---

## Compilation

```bash
# Option 1 : Makefile
make
./clinique

# Option 2 : commande directe
gcc src/*.c -o clinique -Wall -Wextra -I src
./clinique
```

> Testé sous Linux (gcc 9+) et macOS (clang). Compatible Windows
> (MinGW) — les couleurs ANSI fonctionnent sur Windows 10+.

---

## Organisation du code

```
clinique_veto/
├── Makefile
├── README.md
├── src/
│   ├── animal.h / animal.c    → Liste chaînée + recherche + tri
│   ├── queue.h  / queue.c     → File de priorité avec tickets
│   ├── stack.h  / stack.c     → Pile d'historique
│   ├── cli.h    / cli.c       → Interface utilisateur
│   ├── storage.h / storage.c  → Sauvegarde/chargement CSV
│   └── main.c                 → Menu principal
├── build/                     → Fichiers objets (générés)
└── data/                      → Fichiers CSV (générés au runtime)
    ├── animaux.csv
    ├── file_attente.csv
    └── historique.csv
```

---



---

## Scénario de démonstration type (5 min)

1. Lancer le programme → logo ASCII + chargement des données
2. Menu 1 → ajouter **Rex** (chien, urgence normale)
3. Menu 1 → ajouter **Minou** (chat, urgence normale)
4. Menu 1 → ajouter **Buddy** (chien, urgence critique)
5. Menu 7 → mettre Rex en file (urgence 3)
6. Menu 7 → mettre Minou en file (urgence 3)
7. Menu 7 → mettre Buddy en file (urgence 1) — **il passe devant !**
8. Menu 9 → afficher la file : Buddy #1, Rex #2, Minou #3 (FIFO respecté pour urgence égale)
9. Menu 8 → faire passer Buddy : saisir "Visite d'urgence" → automatiquement empilé dans l'historique
10. Menu 10 → afficher l'historique
11. Menu 12 → dashboard ASCII
12. Menu 0 → sauvegarde auto + quitter

---

## Pitch jury (suggestion)

> « Le cahier des charges demandait une Liste, une File et une Pile.
> Nous avons implémenté une **file de priorité** avec gestion des
> égalités par **timestamp logique** (numéro de ticket) garantissant
> le FIFO en cas de même urgence — comme dans un vrai service d'urgences.
> L'insertion triée se fait en O(n) pour permettre un `dequeue` en O(1).
> De plus, nos structures communiquent : chaque consultation déclenche
> automatiquement l'empilement du soin dans l'historique (pont File→Pile).
> Enfin, l'interface propose un dashboard ASCII, des couleurs ANSI et
> une validation des saisies indestructible. »

---

## Améliorations futures possibles

- Passage à SQLite pour la persistance (architecture cache mémoire + flush)
- Tri par autres critères (âge, espèce)
- Gestion des doublons et fusion d'animaux
- Export PDF du dashboard
- Mode multi-utilisateurs (vétérinaires)

