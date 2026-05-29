# Notes de cours — Langages Objets Avancés (C++)

Index des notes du module C++ moderne (C++11+).

## Documents disponibles

| Fichier | Contenu |
|---|---|
| [notes1.md](notes1.md) | Rappels C, passage d'arguments, lvalues/rvalues, classes, constructeurs, règle des 0/3/5, copie/déplacement |
| [notes1_1.md](notes1_1.md) | Initialisation avancée, classes amies (`friend`), surcharge d'opérateurs, bonnes pratiques C++ moderne |
| [notes2.md](notes2.md) | Compléments et thèmes avancés |

## Points clés du cours

- **Standard** : C++11 — compilation avec `-std=c++11 -Wall -Wextra -pedantic`
- **Fichiers** : interface dans `.h`/`.hpp` (avec `#pragma once`), implémentation dans `.cpp`
- **Éviter** `using namespace std;` dans les en-têtes

### Règle des 0/3/5

Si vous définissez l'un des éléments suivants, définissez-les tous :
- Destructeur
- Constructeur de copie
- Opérateur d'affectation par copie
- *(C++11)* Constructeur de déplacement
- *(C++11)* Opérateur d'affectation par déplacement

### Sémantique de déplacement (C++11)

```cpp
class Vecteur {
    double *data;
    size_t n;
public:
    Vecteur(Vecteur&& other) noexcept
        : data(other.data), n(other.n) {
        other.data = nullptr;
        other.n = 0;
    }
};
```

### STL — Algorithmes fréquents

```cpp
#include <algorithm>
#include <numeric>

std::generate(v.begin(), v.end(), gen);
std::transform(v.begin(), v.end(), out, func);
std::accumulate(v.begin(), v.end(), 0, op);
std::for_each(v.begin(), v.end(), action);
std::sort(v.begin(), v.end(), cmp);
```

## Lien vers les TPs et projets

- [TPs/TP0/](TPs/TP0/) — Foncteurs, CMake
- [TPs/TP1/](TPs/TP1/) — STL avancée, hiérarchie de classes
- [projets/GCPA Calcutor/](projets/GCPA%20Calcutor/) — Calculateur GPA (CLI)
- [projets/Projet_LAOA_Carent_Patients/](projets/Projet_LAOA_Carent_Patients/) — Application Qt
