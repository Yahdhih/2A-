# TP1 — STL avancée : foncteurs et vérificateurs

Projet CMake mettant en œuvre la STL C++11 avec des foncteurs statistiques et un système de vérification multi-backend.

## Structure

```
TP1/
├── CMakeLists.txt
├── include/
│   ├── Arithmetic.h
│   ├── RunException.h
│   ├── checkers/
│   │   ├── ExternalCheck.h    # Vérificateur générique externe
│   │   ├── MatlabCheck.h      # Vérificateur Matlab
│   │   └── RCheck.h           # Vérificateur R
│   └── functors/
│       ├── BoundedRandomGenerator.h  # Générateur aléatoire borné
│       └── StatFunctor.h             # Foncteur de statistiques
├── src/
│   ├── RunException.cpp
│   ├── checkers/   # Implémentations des vérificateurs
│   └── functors/   # Implémentations des foncteurs
├── main.cpp
├── extensions.md   # Extensions optionnelles réalisées
├── TP STL.pdf      # Sujet du TP
└── scripts/        # Scripts auxiliaires
```

## Classes principales

### Foncteurs (`functors/`)

- **`BoundedRandomGenerator`** : génère des nombres aléatoires dans `[min, max]`, utilisable avec `std::generate`.
- **`StatFunctor`** : accumule des statistiques (moyenne, variance) de manière incrémentale via `operator()`.

### Vérificateurs (`checkers/`)

- **`ExternalCheck`** : interface abstraite pour la vérification des résultats.
- **`RCheck`** : exporte et vérifie les résultats avec le langage R.
- **`MatlabCheck`** : exporte et vérifie les résultats avec Matlab/Octave.

## Compilation et exécution

```bash
mkdir -p build && cd build
cmake ..
make
cd ..
./bin/tp1
```

## Extensions

Les extensions optionnelles réalisées sont documentées dans [extensions.md](extensions.md).

## Objectifs pédagogiques

- Concevoir une hiérarchie de classes avec héritage et polymorphisme
- Utiliser les algorithmes STL (`std::generate`, `std::for_each`, `std::accumulate`)
- Gérer les exceptions avec des types personnalisés (`RunException`)
- Interfacer C++ avec des outils externes (R, Matlab/Octave)
