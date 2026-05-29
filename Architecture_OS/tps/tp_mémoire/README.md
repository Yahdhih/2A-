# TP Mémoire — Allocateur de tas haute performance

Travaux pratiques sur l'implémentation d'un allocateur de tas (`hp_allocator`) et l'analyse de ses performances via une suite de benchmarks.

## Structure

```
tp_mémoire/
├── ETUDIANT/
│   ├── hp_allocator/
│   │   ├── hp_allocator_lib/          # Bibliothèque de base de l'allocateur
│   │   ├── hp_allocator_malloc/       # Allocateur avec malloc personnalisé
│   │   └── hp_allocator_malloc_free/  # Allocateur avec malloc + free + wrapper LD_PRELOAD
│   └── svalat_bench/                  # Suite de benchmarks mémoire
└── TD_mémoire_Yahdhih_ABDEL_WEDOUD.pdf  # Rapport de TD
```

## Implémentations

### `hp_allocator_malloc`
Implémentation d'un allocateur de tas minimal avec gestion de la politique d'allocation (first-fit, best-fit, etc.).

Compilation et exécution :
```bash
cd ETUDIANT/hp_allocator/hp_allocator_malloc/SRC
make
./a.out
```

### `hp_allocator_malloc_free`
Extension de l'allocateur avec support de `free` et un wrapper `LD_PRELOAD` permettant de remplacer `malloc`/`free` système à la volée sans recompiler l'application cible.

Utilisation du wrapper :
```bash
cd ETUDIANT/hp_allocator/hp_allocator_malloc_free/SRC
make
LD_PRELOAD=./libmalloc_wrapper.so <votre_programme>
# ou via le script fourni :
./run_with_hp_malloc.sh <votre_programme>
```

## Benchmarks (`svalat_bench`)

Suite de benchmarks mesurant les performances de l'allocateur (débit, latence, fragmentation) en comparaison avec `malloc` standard.

```bash
cd ETUDIANT/svalat_bench
make
./main           # benchmark standard
./main_huge      # benchmark avec grandes allocations
python3 plot_benchmark.py        # génère les graphiques
python3 clean_and_convert_to_csv.py  # convertit les résultats en CSV
python3 show_summary.py          # affiche un résumé statistique
```

Les résultats CSV sont disponibles dans `results_hp_malloc.csv` et fichiers associés.

## Rapport

Le rapport complet du TP est disponible au format PDF : [TD_mémoire_Yahdhih_ABDEL_WEDOUD.pdf](TD_mémoire_Yahdhih_ABDEL_WEDOUD.pdf)
