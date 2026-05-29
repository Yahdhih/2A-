# Architecture OS

Notes de cours et travaux pratiques sur l'architecture des systèmes d'exploitation.

## Contenu

| Répertoire / Fichier | Description |
|---|---|
| [tps/tp_mémoire/](tps/tp_mémoire/) | TP sur la gestion de la mémoire — allocateur de tas, benchmarks |
| [kernel/sched/](kernel/sched/) | Étude du code du noyau Linux — ordonnancement (`completion.c`) |
| [kernel/sécurité informatique/](kernel/sécurité%20informatique/) | Notes sur la sécurité système |
| [notes.md](notes.md) | Notes de cours du module |

## Thèmes abordés

- Gestion de la mémoire virtuelle et physique
- Allocateurs de tas (implémentation de `malloc`/`free`)
- Ordonnancement dans le noyau Linux
- Primitives de synchronisation (`completion`, `mutex`, `semaphore`)
- Sécurité au niveau du système d'exploitation

## TP Mémoire

Le TP porte sur l'implémentation d'un allocateur de tas haute performance (`hp_allocator`).
Voir [tps/tp_mémoire/README.md](tps/tp_mémoire/README.md) pour les détails.
