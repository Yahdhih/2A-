# Carnet Patients — Projet Qt

Application de gestion de patients en C++/Qt avec historique des visites, recherche multi-critères et affichage dans un `QTreeView`.

## Fonctionnalités

- Créer, modifier et supprimer des **patients** (nom, prénom, date de naissance, téléphone, ID)
- Ajouter, modifier et supprimer des **visites** (date, motif, note, médecin)
- Visualiser l'historique des visites de chaque patient dans un **QTreeView** (patients → visites)
- Recherche multi-critères (nom, date, pathologie, etc.)
- Validation des champs à la saisie

## Architecture

```
Projet_LAOA_Carent_Patients/
├── planning.md         # Plan de développement sur 21 jours
├── patient.h/.cpp      # Classe Patient (à créer)
├── visit.h/.cpp        # Classe Visit (à créer)
├── patientmodel.h/.cpp # Modèle Qt pour le QTreeView (à créer)
├── mainwindow.h/.cpp   # Fenêtre principale Qt (à créer)
└── CMakeLists.txt      # (à créer)
```

## Modèle de données

```
Patient
├── id: QString
├── nom: QString
├── prenom: QString
├── dateNaissance: QDate
├── telephone: QString
└── visites: QList<Visit>

Visit
├── date: QDate
├── motif: QString
├── note: QString
└── medecin: QString
```

## Plan de développement

Le développement est planifié sur 21 jours (1h/jour) — voir [planning.md](planning.md) pour le détail :

| Semaine | Thème |
|---|---|
| Semaine 1 (jours 1–7) | Modèle & conception (UML, classes de base, tests) |
| Semaine 2 (jours 8–14) | Interface Qt (QTreeView, dialogues, recherche) |
| Semaine 3 (jours 15–21) | Persistance JSON, validation, finalisation |

## Prérequis

- Qt 5.x ou Qt 6.x (Widgets)
- CMake ≥ 3.16
- Compilateur C++11

## Compilation

```bash
mkdir build && cd build
cmake ..
make
./CarnetPatients
```
