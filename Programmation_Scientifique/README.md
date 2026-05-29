# Programmation Scientifique

Calcul scientifique en C++11 — résolution numérique de l'oscillateur harmonique quantique (OHQ) 1D et étude de la densité locale d'un système nucléaire.

## Objectifs

1. Calculer les solutions de l'équation de Schrödinger stationnaire pour l'OHQ 1D via la récurrence des polynômes d'Hermite.
2. Tracer les premières fonctions propres et vérifier numériquement les énergies $E_n$.
3. Vérifier l'orthonormalité des solutions par quadrature numérique.
4. Présenter les résultats avec [remark.js](https://remarkjs.com).

## Contexte physique

L'hamiltonien de l'OHQ 1D est :

$$
\hat{H} = \frac{\hat{p}_z^2}{2m} + \frac{1}{2} m\omega^2 z^2
$$

Les niveaux d'énergie sont discrets :

$$
E_n = \hbar\omega\left(n + \frac{1}{2}\right), \quad n = 0, 1, 2, \ldots
$$

Les solutions sont construites à partir des **polynômes d'Hermite** (version physiciens) avec la relation de récurrence :

$$
H_0(z) = 1, \quad H_1(z) = 2z, \quad H_{n+1}(z) = 2z\,H_n(z) - 2n\,H_{n-1}(z)
$$

## Exigences techniques

- Code source en **C++11**, compilé avec GNU Make
- Documentation générée par **Doxygen**
- Présentation : `pres/index.html` (remark.js)
- Fichier `AUTHORS` requis
- Unités cohérentes (ex. : temps en fs, distance en fm, énergie en MeV)

## Structure attendue du projet

```
Programmation_Scientifique/
├── AUTHORS
├── Makefile
├── src/           # Code source C++11
├── include/       # En-têtes
├── pres/
│   └── index.html # Présentation remark.js
├── doc/           # Documentation Doxygen générée
└── notes.md       # Notes de cours et rappels théoriques
```

## Ressources

- Notes et rappels théoriques : [notes.md](notes.md)
- Illustration des niveaux d'énergie : [énergie et solution de léquation de Schrondinger.png](<énergie et solution de léquation de Schrondinger.png>)
