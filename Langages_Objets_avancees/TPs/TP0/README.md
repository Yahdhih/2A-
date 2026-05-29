# TP0 — Foncteurs et opérateurs en C++

Projet CMake introduisant les foncteurs (objets-fonctions) et la surcharge d'opérateurs en C++11.

## Structure

```
TP0/
├── CMakeLists.txt
├── include/
│   ├── BoundedRandom.h          # Générateur d'entiers aléatoires borné
│   ├── IncrementalMeanOperator.h # Opérateur de moyenne incrémentale
│   └── ConstantSubstractor.h    # Foncteur soustraction d'une constante
├── src/
│   ├── BoundedRandom.cpp
│   ├── IncrementalMeanOperator.cpp
│   ├── ConstantSubstractor.cpp
│   └── main.cpp
└── bin/
```

## Classes implémentées

### `BoundedRandom`
Générateur d'entiers aléatoires dans un intervalle `[min, max]`. Surcharge de `operator()` pour l'utiliser comme foncteur.

### `IncrementalMeanOperator`
Calcule la moyenne de manière incrémentale (sans stocker toutes les valeurs). Utile pour des flux de données.

### `ConstantSubstractor`
Foncteur qui soustrait une constante fixée à la construction. Exemple d'usage avec les algorithmes STL (`std::transform`).

## Compilation

```bash
mkdir -p build && cd build
cmake ..
make
./bin/tp0        # exécutable principal
```

## Objectifs pédagogiques

- Définir et utiliser des foncteurs (surcharge de `operator()`)
- Utiliser des objets-fonctions avec la STL (`std::generate`, `std::transform`, `std::accumulate`)
- Maîtriser la liste d'initialisation des constructeurs
- Appliquer la `const`-correctness
