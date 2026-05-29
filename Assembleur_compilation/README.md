# Assembleur & Compilation

Notes de cours et travaux pratiques sur le langage assembleur RISC-V et les principes de compilation.

## Contenu

```
Assembleur_compilation/
├── TPs/
│   ├── TP1/   # Exercices d'assembleur RISC-V (ex1_1.s … ex2.s)
│   └── TP2/   # Suite des exercices assembleur
├── similator/  # Simulateur Jupiter (RISC-V, Java)
├── notes.md
└── compiler_architecture.png
```

## Simulateur Jupiter

Le simulateur [Jupiter](similator/) est un environnement RISC-V interactif en Java permettant d'exécuter et de déboguer des programmes assembleur.

Lancement :
```bash
cd similator
./bin/jupiter        # Linux/macOS
```

## Travaux Pratiques

### TP1 — Bases de l'assembleur RISC-V

Exercices couvrant :
- Instructions arithmétiques et logiques (`add`, `sub`, `and`, `or`, `xor`)
- Chargement et stockage en mémoire (`lw`, `sw`, `lb`, `sb`)
- Branchements et sauts conditionnels (`beq`, `bne`, `jal`, `jalr`)
- Appels de fonctions et convention d'appel RISC-V

Fichiers : [TPs/TP1/](TPs/TP1/)

### TP2 — Exercices avancés

Suite des exercices assembleur, manipulation de la pile et structures de contrôle complexes.

Fichiers : [TPs/TP2/](TPs/TP2/)

## Architecture de compilation

Le fichier [compiler_architecture.png](compiler_architecture.png) illustre les étapes du pipeline de compilation :
préprocesseur → compilateur → assembleur → éditeur de liens.

## Ressources

- Notes de cours : [notes.md](notes.md)
- Documentation RISC-V : [riscv.org](https://riscv.org)
