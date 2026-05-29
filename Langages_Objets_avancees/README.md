# Langages Objets Avancés (C++)

Notes de cours, travaux pratiques et projets sur le C++ moderne (C++11 et ultérieur) : sémantique de valeur, STL, patrons de conception, Qt.

## Contenu

```
Langages_Objets_avancees/
├── TPs/
│   ├── 01_-_TP_Note/   # TP noté STL (Makefile, tests R)
│   ├── TP0/            # Projet CMake — BoundedRandom, IncrementalMean
│   └── TP1/            # Projet CMake STL avancé — foncteurs, vérificateurs
├── Exercises/
│   └── Basics/
│       └── Objects_Types_Values.cpp  # Exercices sur les types et valeurs
├── projets/
│   ├── GCPA Calcutor/               # Calculateur de GPA (CLI, JSON)
│   └── Projet_LAOA_Carent_Patients/ # Application Qt — carnet patients
├── notes.md       # Index des notes
├── notes1.md      # Rappels C, classes, constructeurs, règle des 0/3/5
├── notes1_1.md    # Initialisation avancée, friend, surcharge d'opérateurs
└── notes2.md      # Compléments
```

## Notes de cours

| Fichier | Contenu |
|---|---|
| [notes1.md](notes1.md) | Rappels C/C++, lvalues/rvalues, classes, constructeurs, copie/déplacement |
| [notes1_1.md](notes1_1.md) | Initialisation avancée, `friend`, surcharge d'opérateurs, `const`-correctness |
| [notes2.md](notes2.md) | Compléments et bonnes pratiques |

## Travaux Pratiques

| TP | Description |
|---|---|
| [TP0](TPs/TP0/) | Générateurs bornés, opérateur de moyenne incrémentale — CMake |
| [TP1](TPs/TP1/) | STL avancée : foncteurs, vérificateurs, pipelines — CMake |
| [01\_-\_TP\_Note](TPs/01_-_TP_Note/) | TP noté : STL avec vérification automatisée en R |

## Projets

| Projet | Description |
|---|---|
| [GCPA Calculator](projets/GCPA%20Calcutor/) | Calculateur de GPA en CLI avec sauvegarde JSON |
| [Carnet Patients (Qt)](projets/Projet_LAOA_Carent_Patients/) | Application Qt : QTreeView, recherche multi-critères |

## Compilation standard

```bash
# CMake (TP0, TP1)
mkdir build && cd build
cmake ..
make

# Make (TP noté)
cd TPs/01_-_TP_Note
make
```

## Conventions du cours

- Standard : **C++11** (`-std=c++11 -Wall -Wextra -pedantic`)
- En-têtes : `.h`/`.hpp` avec `#pragma once`
- Éviter `using namespace std;` dans les en-têtes
