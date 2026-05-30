# Notes de cours — Optimisations Système et Noyau

> **Module** : Architecture OS — ARSE · ENSIIE  
> **Auteur du support** : Aurélien Cedeyn  
> **Année** : 2021–2022  
> **Version EN** : [notes.en.md](notes.en.md)

---

> **Règle d'or** : mesurer → comprendre → modifier → re-mesurer. Ne jamais optimiser sans données.
>
> **Commandes clés**
> ```
> ulimit -a                        # limites du processus
> sysctl -a                        # paramètres noyau
> numactl --hardware               # topologie NUMA
> perf record / perf report        # profiling
> cat /proc/cmdline                # options de boot
> lsmod / modinfo <module>         # modules noyau
> ```

---

## Table des matières

1. [Objectif général du cours](#1-objectif-général-du-cours)
2. [Définition de l’optimisation](#2-définition-de-loptimisation)
3. [Les différents points de vue de l’optimisation](#3-les-différents-points-de-vue-de-loptimisation)
4. [Évolutions logicielles](#4-évolutions-logicielles)
5. [Évolutions matérielles](#5-évolutions-matérielles)
6. [UMA, SMP et NUMA](#6-uma-smp-et-numa)
7. [Exemple d’architecture : Intel Knight Landing](#7-exemple-darchitecture--intel-knight-landing)
8. [Optimisation du système](#8-optimisation-du-système)
9. [Interfaces de configuration : `ulimit`, `/proc/sys`, `sysctl`](#9-interfaces-de-configuration--ulimit-procsys-sysctl)
10. [Configuration du noyau Linux](#10-configuration-du-noyau-linux)
11. [Modules noyau](#11-modules-noyau)
12. [Tickless / Adaptive tick](#12-tickless--adaptive-tick)
13. [Isolation CPU : `isolcpus`](#13-isolation-cpu--isolcpus)
14. [Placement NUMA avec `numactl`](#14-placement-numa-avec-numactl)
15. [Isolation et limitation avec les cgroups](#15-isolation-et-limitation-avec-les-cgroups)
16. [Analyse du système](#16-analyse-du-système)
17. [Flamegraph](#17-flamegraph)
18. [Analyse avec `perf probe`](#18-analyse-avec-perf-probe)
19. [Méthodologie pratique d’optimisation](#19-méthodologie-pratique-doptimisation)
20. [Commandes essentielles](#20-commandes-essentielles)
21. [Questions possibles d’examen avec réponses](#21-questions-possibles-dexamen-avec-réponses)
22. [Résumé final](#22-résumé-final)

---

## 1. Objectif général du cours

Ce cours explique comment **optimiser un système d’exploitation Linux**, en particulier dans un contexte où les performances système et noyau sont importantes.

L’objectif n’est pas uniquement de modifier du code applicatif. Le cours montre que les performances dépendent aussi :

- du matériel utilisé ;
- de l’architecture processeur et mémoire ;
- de la configuration du noyau ;
- des modules noyau ;
- des paramètres système ;
- de la manière dont on place les processus sur les CPU ;
- de la façon dont on mesure réellement les problèmes.

L’idée centrale est simple :

> On ne peut pas optimiser sérieusement un système sans comprendre son matériel, son noyau et son comportement réel à l’exécution.

---

## 2. Définition de l’optimisation

Optimiser signifie rendre quelque chose :

- plus efficace ;
- plus fonctionnel ;
- mieux adapté à un objectif ;
- plus performant dans un contexte donné.

Dans le cours, trois grandes pistes d’optimisation sont distinguées.

### 2.1 Optimisation matérielle

Elle consiste à utiliser ou configurer le matériel pour accélérer certaines opérations.

Exemples :

- déléguer certains traitements au GPU ;
- utiliser RDMA pour réduire le coût des communications réseau ;
- exploiter de nouvelles instructions CPU ;
- utiliser de la mémoire rapide comme la MCDRAM ;
- adapter le placement mémoire sur une machine NUMA.

### 2.2 Optimisation logicielle

Elle consiste à modifier le code d’une application pour améliorer ses performances.

Exemples :

- changer un algorithme ;
- éviter les lectures inutiles ;
- réduire les communications ;
- factoriser du code ;
- utiliser la vectorisation ;
- limiter les accès disque ;
- mieux gérer la mémoire.

### 2.3 Optimisation système

Elle consiste à configurer ou modifier le système d’exploitation afin qu’il réponde mieux aux besoins de l’application.

Exemples :

- régler des paramètres noyau avec `sysctl` ;
- modifier les limites de ressources avec `ulimit` ;
- charger un module noyau avec certains paramètres ;
- isoler des CPU avec `isolcpus` ;
- activer le mode tickless ;
- placer un processus sur un CPU ou un nœud NUMA précis ;
- limiter des ressources avec les cgroups.

---

## 3. Les différents points de vue de l’optimisation

L’optimisation dépend fortement de l’objectif recherché.

### 3.1 Point de vue du développeur

Le développeur cherche souvent à :

- rendre le code maintenable ;
- factoriser le code ;
- améliorer l’algorithme ;
- rendre le programme plus simple à comprendre et à modifier.

Mais un code très propre n’est pas forcément le plus rapide possible.

### 3.2 Point de vue matériel

Le matériel est soumis à des contraintes physiques :

- consommation énergétique ;
- taille mémoire ;
- dissipation thermique ;
- nombre de transistors ;
- capacité des bus ;
- latence mémoire ;
- débit réseau.

L’optimisation matérielle cherche donc à fournir un service rapide, fiable et sûr en utilisant le moins de ressources possible.

### 3.3 Point de vue système

Le système d’exploitation cherche à :

- offrir beaucoup de fonctionnalités ;
- gérer correctement les ressources matérielles ;
- prendre en compte les contraintes de la machine ;
- exécuter les applications rapidement ;
- isoler les processus ;
- préserver la stabilité du système.

Dans un contexte HPC, le système doit surtout éviter de perturber les codes de calcul.

### 3.4 Idée importante

Ces objectifs doivent converger.

Un système bien optimisé n’est pas seulement un système rapide. C’est un système :

- mesuré ;
- compris ;
- adapté à son usage ;
- stable ;
- maintenable ;
- cohérent avec le matériel.

---

## 4. Évolutions logicielles

### 4.1 Tout code a une histoire

Un programme est toujours développé à un instant donné, sur une architecture donnée, avec un système donné.

Mais avec le temps :

- les processeurs évoluent ;
- les systèmes d’exploitation évoluent ;
- les bibliothèques changent ;
- les architectures mémoire deviennent plus complexes ;
- le nombre de cœurs augmente ;
- de nouvelles instructions apparaissent ;
- les systèmes de fichiers et les réseaux changent.

Un code ancien peut donc devenir sous-optimal sur une architecture moderne.

### 4.2 Exemple classique : lecture d’un fichier partagé

Considérons un programme simple :

1. ouverture d’un fichier de données ;
2. lecture du contenu ;
3. fermeture du fichier ;
4. traitement des données ;
5. partage du résultat.

Sur une seule machine, ce programme fonctionne bien.

Mais si :

- le fichier est situé sur un système de fichiers partagé ;
- 1000 machines lancent le même programme en même temps ;
- chaque machine lit le même fichier ;

alors le système peut s’effondrer.

### 4.3 Pourquoi le système s’effondre ?

Le problème vient du fait que les 1000 machines sollicitent en même temps le même serveur ou le même système de fichiers.

Cela peut provoquer :

- saturation réseau ;
- saturation du serveur de fichiers ;
- trop d’ouvertures de fichiers ;
- trop de lectures concurrentes ;
- latence très élevée ;
- baisse massive des performances.

Le code est correct localement, mais mauvais à grande échelle.

### 4.4 Trois types d’optimisation possibles

#### Solution 1 — Renforcer le serveur

On peut utiliser des serveurs plus robustes capables d’absorber la charge.

Avantage :

- pas besoin de modifier le code.

Inconvénients :

- coûteux ;
- ne règle pas toujours le problème ;
- risque de déplacer le goulot d’étranglement.

#### Solution 2 — Modifier le code applicatif

Une machine lit le fichier, puis partage les données avec les autres.

Le code de la première machine :

1. ouvre le fichier ;
2. lit les données ;
3. ferme le fichier ;
4. partage les données avec les autres machines ;
5. traite les données ;
6. partage le résultat.

Le code des autres machines :

1. attend les données ;
2. traite les données ;
3. partage le résultat.

Avantage :

- réduction forte de la charge sur le système de fichiers.

Inconvénient :

- il faut modifier l’application.

#### Solution 3 — Modifier le système

On peut ajouter une couche système, par exemple un cache, pour éviter que toutes les machines relisent les mêmes données depuis le serveur.

Avantage :

- le code applicatif peut rester inchangé.

Inconvénient :

- nécessite une configuration système correcte ;
- demande de bien comprendre le comportement des accès.

---

## 5. Évolutions matérielles

Le matériel devient de plus en plus performant, mais aussi plus complexe.

### 5.1 GPU

Les GPU permettent d’utiliser la puissance du processeur graphique pour faire du calcul.

Ils sont particulièrement adaptés :

- au calcul matriciel ;
- aux traitements massivement parallèles ;
- aux applications numériques ;
- aux simulations ;
- au machine learning.

Avantage important :

- très bon rapport performance / consommation énergétique.

### 5.2 Réseau et RDMA

RDMA signifie **Remote Direct Memory Access**.

L’idée est de permettre à une machine d’accéder directement à la mémoire d’une autre machine sans passer par toute la pile logicielle classique.

Avantages :

- latence plus faible ;
- moins de copies mémoire ;
- moins de charge CPU ;
- meilleur débit réseau ;
- très utile en HPC.

### 5.3 CPU

Les CPU évoluent avec :

- de nouvelles instructions ;
- plus de cœurs ;
- plus de parallélisme ;
- des extensions de vectorisation ;
- des caches plus complexes ;
- des architectures NUMA.

L’optimisation système doit donc suivre ces évolutions.

---

## 6. UMA, SMP et NUMA

## 6.1 UMA — Uniform Memory Access

Au début des architectures multiprocesseurs, tous les processeurs avaient un accès uniforme à la mémoire.

Cela signifie que chaque CPU accède à la mémoire avec une latence comparable.

Cette architecture est typique des systèmes SMP.

### SMP — Symmetric Multi-Processor

Dans une architecture SMP :

- plusieurs CPU partagent la même mémoire ;
- l’accès à la mémoire est uniforme ;
- le système est relativement simple à programmer.

### Limite du SMP

Le problème apparaît quand le nombre de processeurs augmente.

Plus il y a de CPU, plus le bus mémoire est sollicité.

Conséquence :

- saturation du bus ;
- augmentation des latences ;
- baisse du passage à l’échelle ;
- performances non linéaires.

---

## 6.2 NUMA — Non Uniform Memory Access

Pour résoudre les limites du SMP, on utilise des architectures NUMA.

Dans une architecture NUMA :

- chaque processeur ou groupe de processeurs possède une mémoire locale ;
- un CPU peut accéder à sa mémoire locale rapidement ;
- un CPU peut aussi accéder à la mémoire d’un autre nœud, mais plus lentement ;
- l’accès mémoire n’est donc plus uniforme.

### Conséquences de NUMA

NUMA impose une gestion fine de la localisation :

- placement des processus ;
- placement des threads ;
- placement des pages mémoire ;
- placement des interruptions ;
- placement des entrées/sorties ;
- affinité CPU / mémoire.

### Idée essentielle

Sur NUMA, deux exécutions du même programme peuvent avoir des performances différentes selon :

- le CPU sur lequel le processus tourne ;
- le nœud NUMA où la mémoire est allouée ;
- la proximité du périphérique d’E/S ;
- le comportement de l’ordonnanceur.

---

## 7. Exemple d’architecture : Intel Knight Landing

Le cours présente l’architecture **Knight Landing** comme exemple d’architecture CPU particulière.

Caractéristiques importantes :

- compatibilité avec l’architecture `x86_64` ;
- nouvelles instructions de vectorisation ;
- très grand nombre de CPU logiques ;
- introduction d’un nouveau type de mémoire rapide : **MCDRAM** ;
- présence de mémoire classique DDR4 ;
- complexité accrue du placement mémoire et CPU.

Le support mentionne une organisation de type :

```text
36 × 2 × 2 = 144 CPUs
```

Cela signifie que le nombre important de cœurs et de threads accentue les problèmes suivants :

- bruit système ;
- placement des processus ;
- synchronisation ;
- accès mémoire ;
- contention ;
- choix de la mémoire utilisée.

---

## 8. Optimisation du système

Le système d’exploitation se situe entre :

```text
Application / logiciel
        ↓
Système d’exploitation
        ↓
Matériel
```

Il joue donc un rôle central.

### 8.1 Types d’optimisation système

Au niveau du système, plusieurs actions sont possibles.

#### 1. Jouer sur la configuration

Exemples :

- `ulimit` ;
- `sysctl` ;
- paramètres `/proc/sys` ;
- paramètres de modules ;
- ligne de commande du noyau.

#### 2. Utiliser des services ou outils adaptés

Exemples :

- `numactl` ;
- cgroups ;
- Slurm ;
- outils de profiling ;
- outils de tracing.

#### 3. Modifier ou configurer le noyau

Exemples :

- options de compilation du noyau ;
- activation de fonctionnalités ;
- modules noyau ;
- `CONFIG_NO_HZ_FULL` ;
- `isolcpus` ;
- configuration tickless.

### 8.2 Pourquoi mesurer ?

Le système permet d’analyser le comportement des composants.

À partir de cette analyse, on peut décider :

- quel paramètre modifier ;
- quelle ressource est saturée ;
- quel processus perturbe le calcul ;
- quel CPU est trop sollicité ;
- quelle mémoire est mal placée ;
- quel appel système est trop fréquent.

---

## 9. Interfaces de configuration : `ulimit`, `/proc/sys`, `sysctl`

Les distributions Linux choisissent des paramètres par défaut adaptés au plus grand nombre.

Mais ces paramètres ne sont pas forcément optimaux pour :

- le HPC ;
- les serveurs critiques ;
- les applications temps réel ;
- les charges massivement parallèles ;
- les systèmes avec beaucoup de fichiers ;
- les applications réseau intensives.

Deux interfaces système importantes :

- `ulimit` ;
- `sysctl`.

---

## 9.1 `ulimit`

`ulimit` permet de consulter ou modifier certaines limites associées aux processus.

Commande :

```bash
ulimit -a
```

Exemples de limites :

- taille des fichiers core ;
- taille du segment de données ;
- priorité d’ordonnancement ;
- nombre de fichiers ouverts ;
- nombre de signaux pendants ;
- mémoire verrouillée ;
- taille de pile ;
- temps CPU ;
- nombre maximal de processus utilisateur ;
- mémoire virtuelle ;
- verrous fichiers.

On peut aussi consulter les limites d’un processus précis :

```bash
cat /proc/<PID>/limits
```

### 9.1.1 Configuration permanente

La configuration est stockée dans :

```bash
/etc/security/limits.conf
```

Elle est activée via PAM, c’est-à-dire les **Pluggable Authentication Modules**.

Format général :

```text
<domain> <type> <item> <value>
```

### 9.1.2 Domaine

Le domaine peut être :

- un utilisateur ;
- un groupe avec `@groupe` ;
- `*` pour tous ;
- `%` pour certains cas comme `maxlogins`.

### 9.1.3 Type

Le type peut être :

- `soft` : limite souple ;
- `hard` : limite dure.

### 9.1.4 Exemple

```text
@student hard nproc 20
@faculty soft nproc 20
@faculty hard nproc 50
@student - maxlogins 4
```

Interprétation :

- les étudiants peuvent avoir une limite stricte sur le nombre de processus ;
- les enseignants peuvent avoir une limite plus élevée ;
- certains groupes peuvent avoir un nombre maximal de connexions.

---

## 9.2 `/proc/sys`

`/proc/sys` est un répertoire du pseudo-système de fichiers `procfs`.

Il contient de nombreux paramètres du noyau.

Commande :

```bash
ls /proc/sys
```

Exemple de contenu :

```text
abi  debug  dev  fs  kernel  net  user  vm
```

### 9.2.1 Principaux sous-répertoires

| Répertoire | Rôle |
|---|---|
| `fs` | paramètres liés au système de fichiers virtuel |
| `net` | paramètres réseau |
| `kernel` | paramètres généraux du noyau |
| `vm` | mémoire virtuelle |
| `dev` | périphériques |
| `user` | namespaces utilisateur |
| `abi` | compatibilité ABI |
| `debug` | fonctions de debugging |

---

## 9.3 `sysctl`

`sysctl` est l’outil qui permet de gérer les paramètres présents dans `/proc/sys`.

### 9.3.1 Lister les paramètres

```bash
sysctl -a
```

### 9.3.2 Lire un paramètre

```bash
sysctl vm.swappiness
```

### 9.3.3 Modifier un paramètre temporairement

```bash
sudo sysctl vm.swappiness=10
```

Cela revient à modifier :

```bash
/proc/sys/vm/swappiness
```

### 9.3.4 Rendre une configuration permanente

Les configurations permanentes peuvent être placées dans :

```bash
/etc/sysctl.d/
```

Exemple :

```bash
sudo nano /etc/sysctl.d/99-custom.conf
```

Puis :

```text
vm.swappiness = 10
fs.file-max = 1000000
```

Appliquer :

```bash
sudo sysctl --system
```

### 9.3.5 Remarque importante

Changer un paramètre système sans mesure préalable est dangereux.

Il faut toujours :

1. mesurer ;
2. comprendre ;
3. modifier ;
4. mesurer de nouveau.

---

## 9.4 `procfs`, `sysfs`, `debugfs`

Le cours mentionne trois interfaces importantes.

### `procfs`

Système de fichiers virtuel historique.

Exemples :

```bash
/proc/cpuinfo
/proc/meminfo
/proc/<PID>/limits
/proc/sys
```

### `sysfs`

Interface plus moderne pour exposer les objets du noyau.

Exemple :

```bash
/sys/module/
/sys/devices/
/sys/fs/cgroup/
```

### `debugfs`

Interface orientée debugging noyau.

Elle expose des informations utiles pour le développement, le tracing et l’analyse.

---

## 10. Configuration du noyau Linux

Chaque noyau Linux possède une configuration précise.

Cette configuration détermine :

- les pilotes disponibles ;
- les options activées ;
- les fonctionnalités compilées ;
- les modules possibles ;
- les mécanismes de performance disponibles ;
- les fonctions de debugging.

### 10.1 Origine de la configuration

La configuration est issue de :

```bash
make config
make menuconfig
make xconfig
```

ou d’outils équivalents.

### 10.2 Où consulter la configuration ?

#### `/proc/config.gz`

Disponible si le noyau a été compilé avec :

```text
CONFIG_IKCONFIG_PROC
```

Commande :

```bash
zcat /proc/config.gz
```

#### `extract-ikconfig`

Utilisable si le noyau contient sa configuration avec :

```text
CONFIG_IKCONFIG
```

#### `/boot/config-<kernel-version>`

Les distributions fournissent souvent la configuration dans :

```bash
/boot/config-$(uname -r)
```

Commande :

```bash
cat /boot/config-$(uname -r)
```

### 10.3 Exemple d’options noyau

```text
CONFIG_64BIT=y
CONFIG_X86_64=y
CONFIG_MMU=y
CONFIG_STACKTRACE_SUPPORT=y
CONFIG_LOCKDEP_SUPPORT=y
```

Ces options indiquent les fonctionnalités compilées dans le noyau.

---

## 10.4 Ligne de commande du noyau

Certaines fonctionnalités du noyau se configurent au démarrage, sur la ligne de commande du noyau.

Commande pour la consulter :

```bash
cat /proc/cmdline
```

Exemple :

```text
BOOT_IMAGE=/vmlinuz-linux root=UUID=... rw crashkernel=auto
```

### Éléments importants

- `root=` indique le disque ou la partition racine ;
- les paramètres commençant par `rd.` sont liés à l’initrd ;
- certains paramètres activent des comportements noyau spécifiques ;
- la documentation se trouve dans les sources du noyau.

Documentation typique :

```text
Documentation/kernel-parameters.txt
```

---

## 11. Modules noyau

Un module noyau est du code supplémentaire que l’on peut charger dans le noyau.

### 11.1 Charger un module

Commande principale :

```bash
sudo modprobe <module>
```

`modprobe` utilise en interne :

```bash
insmod <fichier.ko>
```

mais `modprobe` gère aussi automatiquement les dépendances.

### 11.2 Paramètres de modules

Les paramètres sont souvent configurés dans :

```bash
/etc/modprobe.d/*.conf
```

Format :

```text
options nom_du_module parametre=valeur
```

Exemple fictif :

```text
options kvm ignore_msrs=1
```

### 11.3 Obtenir des informations sur un module

```bash
modinfo <module>
```

Exemple :

```bash
modinfo kvm
```

Cela affiche :

- le fichier `.ko` ;
- la licence ;
- l’auteur ;
- les dépendances ;
- les paramètres disponibles ;
- la version noyau compatible.

### 11.4 Modules chargés

```bash
lsmod
```

Cette commande affiche :

- le nom du module ;
- sa taille ;
- le nombre d’utilisations ;
- les modules qui en dépendent.

### 11.5 Arborescence `/sys/module`

Quand un module est chargé, le noyau crée une arborescence :

```bash
/sys/module/<module>/
```

Exemple :

```bash
find /sys/module/kvm -type d
```

On peut trouver :

```text
/sys/module/kvm/
/sys/module/kvm/parameters
/sys/module/kvm/sections
/sys/module/kvm/holders
```

Le répertoire important est :

```bash
/sys/module/<module>/parameters
```

Il contient les paramètres du module.

Certains paramètres peuvent être modifiés à chaud, d’autres uniquement au chargement.

---

## 12. Tickless / Adaptive tick

### 12.1 Problème historique

Historiquement, le noyau réveillait régulièrement tous les processeurs avec un **tick** d’horloge.

Ce tick permettait au système d’évaluer le temps qui passe et de gérer certaines tâches internes.

Mais cela pose problème :

- un CPU idle est réveillé inutilement ;
- le CPU ne peut pas rester en économie d’énergie ;
- un processus en cours peut être perturbé ;
- en HPC, ces interruptions peuvent dégrader les performances.

### 12.2 `NO_HZ_IDLE`

Avec `NO_HZ_IDLE`, seuls les processeurs qui ne sont pas idle continuent à recevoir certains ticks.

C’est le mode **dyntick-idle**.

### 12.3 `NO_HZ_FULL`

Depuis le noyau 3.10, une fonctionnalité permet de ne pas réveiller inutilement certains CPU, même lorsqu’ils ne sont pas idle.

C’est le mode tickless complet ou adaptive tick.

### 12.4 Intérêt en HPC

Le tickless est intéressant en HPC parce qu’il permet de réduire le bruit système.

Avantages :

- moins d’interruptions ;
- moins de perturbations sur les processus de calcul ;
- meilleure stabilité des temps d’exécution ;
- meilleure efficacité énergétique ;
- meilleure synchronisation entre processus.

### 12.5 Mise en place

Le noyau doit être compilé avec :

```text
CONFIG_NO_HZ_FULL
```

Puis activer au boot :

```text
nohz_full=[cpus]
```

Exemple :

```text
nohz_full=1-15
```

Ou compiler avec :

```text
CONFIG_NO_HZ_FULL_ALL
```

Remarque importante :

> Seul le CPU sur lequel le noyau a booté recevra les ticks.

Documentation :

```text
Documentation/timers/NO_HZ.txt
```

---

## 13. Isolation CPU : `isolcpus`

### 13.1 Pourquoi isoler des CPU ?

En HPC, les codes de calcul sont sensibles aux perturbations du système d’exploitation.

Exemples de perturbations :

- interruptions ;
- tâches noyau ;
- migrations de processus ;
- ordonnanceur ;
- démons système ;
- ticks d’horloge ;
- processus utilisateurs parasites.

La synchronisation des communications entre processus est essentielle.

Une petite perturbation sur un processus peut ralentir toute l’application parallèle.

### 13.2 Idée de `isolcpus`

L’idée est d’isoler certains CPU du scheduler général.

Ainsi, l’ordonnanceur Linux ne place plus automatiquement des processus sur ces CPU.

### 13.3 Mise en place

Sur la ligne de boot du noyau :

```text
isolcpus=[cpus]
```

Exemples :

```text
isolcpus=2
isolcpus=2,4,6
isolcpus=4-15
isolcpus=2,4-8,12
```

### 13.4 Effet

Après activation :

- l’ordonnanceur ne choisit plus automatiquement ces CPU ;
- il faut placer explicitement les processus dessus ;
- on utilise typiquement `numactl`, `taskset` ou Slurm.

### 13.5 Limites

- nécessite une bonne stratégie de placement ;
- peut poser problème avec des programmes multi-threadés ;
- OpenMP gère souvent mieux ce type de placement ;
- une mauvaise isolation peut réduire l’utilisation globale de la machine.

---

## 14. Placement NUMA avec `numactl`

### 14.1 Le problème

Sur une machine NUMA, il ne suffit pas de lancer un programme.

Il faut parfois choisir :

- le CPU d’exécution ;
- le nœud NUMA mémoire ;
- la politique d’allocation mémoire ;
- l’affinité du processus.

### 14.2 Pinning

Le **pinning** consiste à fixer un processus ou un thread sur un ou plusieurs CPU.

En français, on peut parler de **punaisage**.

### 14.3 `numactl`

`numactl` permet de contrôler :

- l’affinité CPU ;
- l’affinité mémoire ;
- le nœud NUMA préféré ;
- l’interleaving mémoire ;
- le placement physique.

### 14.4 Appels systèmes utilisés

`numactl` s’appuie notamment sur :

```text
sched_getaffinity
sched_setaffinity
```

- `sched_getaffinity` récupère l’affinité CPU d’un processus ;
- `sched_setaffinity` modifie l’affinité CPU.

### 14.5 Afficher le matériel NUMA

```bash
numactl --hardware
```

Exemple de sortie :

```text
available: 1 nodes (0)
node 0 cpus: 0 1 2 3
node 0 size: 7878 MB
node 0 free: 3784 MB
node distances:
node 0
0: 10
```

### 14.6 Exécuter un programme avec placement CPU et mémoire

Exemple :

```bash
numactl --physcpubind=3 --membind=0 /usr/sbin/httpd
```

Signification :

- exécuter le programme sur le CPU physique 3 ;
- allouer la mémoire sur le nœud NUMA 0.

### 14.7 Options utiles

| Option | Rôle |
|---|---|
| `--hardware` ou `-H` | affiche la topologie NUMA |
| `--show` ou `-s` | affiche la politique actuelle |
| `--physcpubind=<cpus>` | fixe les CPU physiques |
| `--cpunodebind=<nodes>` | fixe les nœuds CPU |
| `--membind=<nodes>` | fixe les nœuds mémoire |
| `--interleave=<nodes>` | répartit les allocations mémoire |
| `--localalloc` | privilégie la mémoire locale |
| `--preferred=<node>` | indique un nœud préféré |

---

## 15. Isolation et limitation avec les cgroups

Les **cgroups** permettent d’isoler et de limiter finement les ressources des processus.

### 15.1 Ce que permettent les cgroups

Ils permettent notamment de :

- limiter la mémoire ;
- limiter le nombre de CPU utilisés ;
- utiliser des cpusets ;
- limiter les entrées/sorties ;
- limiter l’accès aux périphériques ;
- contrôler des groupes de processus ;
- organiser les processus par utilisateur, service ou job.

### 15.2 Arborescence

Les cgroups sont visibles dans :

```bash
/sys/fs/cgroup/
```

Exemple :

```bash
ls /sys/fs/cgroup/
```

On peut y trouver :

```text
blkio
cpu
cpuacct
cpuset
devices
freezer
memory
net_cls
net_prio
pids
perf_event
systemd
unified
```

### 15.3 Intérêt en optimisation

Les cgroups sont utiles pour :

- éviter qu’un processus consomme toute la mémoire ;
- isoler un job HPC ;
- réserver des CPU ;
- limiter les I/O ;
- contrôler un service système ;
- éviter qu’une charge parasite perturbe un calcul.

Documentation mentionnée :

```text
Documentation/cgroup-v2.txt
```

---

## 16. Analyse du système

Le cours résume bien les deux phrases typiques des utilisateurs :

```text
C’est lent !
Ça ne marche pas !
```

La vraie question devient alors :

```text
Où est le docteur ?
```

Autrement dit :

> Quels outils utiliser pour diagnostiquer correctement le problème ?

### 16.1 Pourquoi analyser ?

Parce qu’une optimisation sans mesure peut :

- ne rien améliorer ;
- dégrader les performances ;
- cacher le vrai problème ;
- déplacer le goulot d’étranglement ;
- rendre le système instable.

### 16.2 Ce qu’il faut chercher

On peut analyser :

- CPU ;
- mémoire ;
- I/O disque ;
- réseau ;
- interruptions ;
- appels système ;
- fonctions noyau ;
- locks ;
- migrations de processus ;
- défauts de cache ;
- contention NUMA.

---

## 17. Flamegraph

Un **Flamegraph** est une représentation graphique des traces d’exécution.

### 17.1 Principe

Un Flamegraph prend un ensemble de traces provenant d’outils externes comme :

- `gdb` ;
- `perf` ;
- `systemtap`.

Puis il analyse ces traces et génère un histogramme d’utilisation.

### 17.2 À quoi sert un Flamegraph ?

Il permet de voir rapidement :

- quelles fonctions consomment le plus de temps ;
- quels chemins d’appel sont dominants ;
- où se situe le coût principal ;
- si le temps est passé dans le code utilisateur ou dans le noyau ;
- si une fonction est appelée trop souvent.

### 17.3 Comment lire un Flamegraph ?

Règles générales :

- la largeur d’un bloc représente le temps cumulé ;
- plus un bloc est large, plus il consomme du temps ;
- la hauteur représente la profondeur de pile ;
- les appels parents sont en bas ;
- les appels enfants sont au-dessus.

### 17.4 Intérêt

Le Flamegraph est très utile pour passer de :

```text
C’est lent
```

à :

```text
Cette fonction ou ce chemin d’appel consomme trop de temps
```

Lien mentionné dans le cours :

```text
http://www.brendangregg.com/flamegraphs.html
```

---

## 18. Analyse avec `perf probe`

### 18.1 Rôle de `perf`

`perf` est un outil d’analyse des performances sous Linux.

Il permet notamment de :

- mesurer les événements CPU ;
- profiler une application ;
- analyser le noyau ;
- enregistrer des traces ;
- observer les fonctions appelées ;
- ajouter des sondes dynamiques.

### 18.2 `perf probe`

`perf probe` permet d’ajouter dynamiquement des points de trace sur des fonctions du noyau ou de l’espace utilisateur.

Le cours montre l’exemple de la fonction noyau :

```text
vfs_open
```

Cette fonction intervient dans l’ouverture de fichiers via la VFS.

### 18.3 Lister le code autour d’une fonction

```bash
perf probe -L vfs_open
```

Cela permet de voir les lignes disponibles dans la fonction.

### 18.4 Ajouter une sonde

Exemple :

```bash
perf probe vfs_open:8 path
```

Cette commande ajoute une sonde sur la ligne 8 de `vfs_open` et récupère l’argument `path`.

### 18.5 Enregistrer les événements

```bash
perf record -e probe:vfs_open_1 -aR sleep 1
```

Signification :

- enregistrer les événements de la sonde ;
- sur tout le système avec `-a` ;
- pendant une seconde ;
- puis stocker les résultats dans `perf.data`.

### 18.6 Lire les traces

```bash
perf script
```

Le cours montre ensuite des événements comme :

```text
sleep 1279 [000] ... probe:vfs_open_1 ...
crazy 955 [000] ... probe:vfs_open_1 ...
```

Cela permet d’identifier quels processus ouvrent des fichiers et à quel rythme.

### 18.7 Utilité pratique

`perf probe` permet de répondre à des questions comme :

- quel processus appelle cette fonction noyau ?
- combien de fois cette fonction est appelée ?
- quel argument est passé ?
- quel comportement anormal apparaît ?
- quel processus génère trop d’événements ?

---

## 19. Méthodologie pratique d’optimisation

Une bonne optimisation suit une méthode stricte.

### Étape 1 — Comprendre le contexte

Questions à poser :

- Quelle application ?
- Quel matériel ?
- Combien de CPU ?
- Architecture NUMA ou non ?
- GPU ou non ?
- Réseau utilisé ?
- Système de fichiers local ou partagé ?
- Charge interactive ou batch ?
- Contexte HPC ou serveur classique ?

### Étape 2 — Mesurer

Il faut obtenir des faits.

Exemples d’outils :

```bash
top
htop
vmstat
iostat
pidstat
perf
numactl --hardware
cat /proc/cmdline
sysctl -a
ulimit -a
lsmod
```

### Étape 3 — Identifier le goulot d’étranglement

Le problème peut venir :

- du CPU ;
- de la mémoire ;
- du réseau ;
- du disque ;
- du système de fichiers ;
- du scheduler ;
- du placement NUMA ;
- des interruptions ;
- d’un module noyau ;
- d’une mauvaise configuration.

### Étape 4 — Choisir une optimisation

Exemples :

| Problème | Optimisation possible |
|---|---|
| Trop de lectures sur un fichier partagé | cache ou modification du code |
| Bruit système sur CPU de calcul | `isolcpus`, `nohz_full` |
| Mauvais placement mémoire | `numactl --membind` |
| Trop de processus | `ulimit`, cgroups |
| Trop de fichiers ouverts | augmenter `nofile` |
| Appels noyau trop fréquents | `perf probe`, Flamegraph |
| Mauvais paramètre noyau | `sysctl` |

### Étape 5 — Modifier avec prudence

Ne jamais changer plusieurs paramètres à la fois sans méthode.

Bonne pratique :

1. mesurer état initial ;
2. changer un seul paramètre ;
3. refaire le test ;
4. comparer ;
5. conserver ou annuler.

### Étape 6 — Documenter

Une optimisation doit être documentée.

Il faut noter :

- paramètre modifié ;
- raison ;
- valeur initiale ;
- valeur finale ;
- impact mesuré ;
- risques ;
- méthode de retour arrière.

---

## 20. Commandes essentielles

### 20.1 Informations noyau

```bash
uname -a
cat /proc/cmdline
cat /boot/config-$(uname -r)
zcat /proc/config.gz
```

### 20.2 Limites processus

```bash
ulimit -a
cat /proc/<PID>/limits
```

### 20.3 Paramètres noyau

```bash
sysctl -a
sysctl vm.swappiness
sudo sysctl vm.swappiness=10
sudo sysctl --system
```

### 20.4 Modules noyau

```bash
lsmod
modinfo kvm
sudo modprobe kvm
find /sys/module/kvm -type d
ls /sys/module/kvm/parameters
```

### 20.5 NUMA

```bash
numactl --hardware
numactl --show
numactl --physcpubind=3 --membind=0 ./programme
```

### 20.6 Cgroups

```bash
ls /sys/fs/cgroup/
```

### 20.7 Perf

```bash
perf top
perf record ./programme
perf report
perf probe -L vfs_open
perf probe vfs_open:8 path
perf record -e probe:vfs_open_1 -aR sleep 1
perf script
```

### 20.8 Boot options importantes

```text
nohz_full=1-15
isolcpus=1-15
```

---

## 21. Questions possibles d’examen avec réponses

### Question 1 — Qu’est-ce que l’optimisation système ?

L’optimisation système consiste à configurer ou modifier le système d’exploitation pour qu’il réponde mieux aux besoins des applications. Elle peut passer par `sysctl`, `ulimit`, les modules noyau, la ligne de commande du noyau, `numactl`, les cgroups ou encore l’isolation CPU.

---

### Question 2 — Quelle est la différence entre optimisation logicielle, matérielle et système ?

L’optimisation logicielle modifie le code ou l’algorithme. L’optimisation matérielle exploite ou adapte le matériel. L’optimisation système agit sur l’OS, ses paramètres, ses modules et son comportement pour mieux servir l’application.

---

### Question 3 — Pourquoi faut-il analyser avant d’optimiser ?

Parce qu’une optimisation sans mesure peut être inutile ou dangereuse. L’analyse permet d’identifier le vrai goulot d’étranglement : CPU, mémoire, I/O, réseau, noyau, placement NUMA ou appels système.

---

### Question 4 — Pourquoi un programme correct sur une machine peut s’effondrer sur 1000 machines ?

Parce que le passage à l’échelle change le comportement. Par exemple, si 1000 machines lisent simultanément le même fichier sur un système partagé, le serveur de fichiers ou le réseau peut être saturé.

---

### Question 5 — Quelles solutions existent pour éviter 1000 lectures identiques d’un fichier partagé ?

On peut renforcer les serveurs, modifier le code pour lire une seule fois puis partager les données, ou ajouter une couche système comme un cache pour éviter de modifier le code.

---

### Question 6 — Qu’est-ce que NUMA ?

NUMA signifie Non Uniform Memory Access. Chaque CPU ou groupe de CPU possède une mémoire locale. L’accès à la mémoire locale est plus rapide que l’accès à la mémoire distante.

---

### Question 7 — Pourquoi NUMA complique l’optimisation ?

Parce que les performances dépendent du placement des processus, des threads, des pages mémoire, des interruptions et des I/O. Un mauvais placement peut provoquer des accès mémoire distants coûteux.

---

### Question 8 — À quoi sert `ulimit` ?

`ulimit` permet de consulter ou définir les limites de ressources d’un processus : nombre de fichiers ouverts, taille de pile, temps CPU, mémoire, nombre de processus, etc.

---

### Question 9 — Où sont configurées les limites permanentes de `ulimit` ?

Elles sont configurées dans `/etc/security/limits.conf` et appliquées via PAM.

---

### Question 10 — À quoi sert `sysctl` ?

`sysctl` sert à lire ou modifier les paramètres du noyau exposés dans `/proc/sys`.

---

### Question 11 — Comment rendre une configuration `sysctl` permanente ?

Il faut placer la configuration dans un fichier sous `/etc/sysctl.d/`, puis appliquer avec `sysctl --system`.

---

### Question 12 — Qu’est-ce qu’un module noyau ?

Un module noyau est un morceau de code que l’on peut charger dans le noyau pour ajouter une fonctionnalité ou un pilote sans recompiler tout le noyau.

---

### Question 13 — Quelle est la différence entre `insmod` et `modprobe` ?

`insmod` charge directement un fichier `.ko`. `modprobe` charge un module en gérant automatiquement les dépendances.

---

### Question 14 — À quoi sert `modinfo` ?

`modinfo` affiche les informations d’un module : fichier, licence, auteur, dépendances, paramètres disponibles et version compatible.

---

### Question 15 — Où trouve-t-on les paramètres d’un module chargé ?

Dans `/sys/module/<module>/parameters`.

---

### Question 16 — Qu’est-ce que le mode tickless ?

Le mode tickless réduit ou supprime les ticks périodiques envoyés aux CPU, afin de diminuer les réveils inutiles et les perturbations système.

---

### Question 17 — Pourquoi le tickless est-il intéressant en HPC ?

Parce qu’il réduit le bruit système sur les CPU de calcul, évite des interruptions inutiles et améliore la stabilité des performances.

---

### Question 18 — Comment activer `NO_HZ_FULL` ?

Le noyau doit être compilé avec `CONFIG_NO_HZ_FULL`, puis on active les CPU concernés avec `nohz_full=[cpus]` sur la ligne de boot.

---

### Question 19 — À quoi sert `isolcpus` ?

`isolcpus` permet d’isoler certains CPU du scheduler général. Linux ne placera plus automatiquement des processus sur ces CPU.

---

### Question 20 — Que faut-il utiliser après `isolcpus` pour lancer un processus sur un CPU isolé ?

Il faut utiliser un outil de placement explicite comme `numactl`, `taskset` ou Slurm.

---

### Question 21 — À quoi sert `numactl` ?

`numactl` permet de contrôler le placement CPU et mémoire d’un processus, notamment sur les architectures NUMA.

---

### Question 22 — Donne un exemple d’utilisation de `numactl`.

```bash
numactl --physcpubind=3 --membind=0 ./programme
```

Cette commande exécute le programme sur le CPU 3 avec allocation mémoire sur le nœud NUMA 0.

---

### Question 23 — À quoi servent les cgroups ?

Les cgroups permettent de limiter et isoler les ressources des processus : CPU, mémoire, I/O, périphériques, nombre de processus, etc.

---

### Question 24 — Où trouve-t-on les cgroups dans le système de fichiers ?

Dans `/sys/fs/cgroup/`.

---

### Question 25 — Qu’est-ce qu’un Flamegraph ?

Un Flamegraph est une représentation graphique des traces d’exécution. Il montre les fonctions qui consomment le plus de temps et les chemins d’appel dominants.

---

### Question 26 — Quels outils peuvent fournir des traces pour un Flamegraph ?

`gdb`, `perf` et `systemtap` peuvent fournir des traces exploitables pour générer un Flamegraph.

---

### Question 27 — À quoi sert `perf probe` ?

`perf probe` permet d’ajouter dynamiquement des sondes sur des fonctions du noyau ou de l’espace utilisateur afin d’enregistrer des événements précis.

---

### Question 28 — Que fait cette commande ?

```bash
perf probe -L vfs_open
```

Elle liste le code source disponible autour de la fonction noyau `vfs_open` pour permettre de choisir un point de sonde.

---

### Question 29 — Que fait cette commande ?

```bash
perf probe vfs_open:8 path
```

Elle ajoute une sonde sur la ligne 8 de la fonction `vfs_open` et enregistre l’argument `path`.

---

### Question 30 — Que fait cette commande ?

```bash
perf record -e probe:vfs_open_1 -aR sleep 1
```

Elle enregistre pendant une seconde, sur toute la machine, les événements liés à la sonde `probe:vfs_open_1`.

---

## 22. Résumé final

Ce cours montre que l’optimisation système et noyau repose sur une idée fondamentale :

> Avant d’optimiser, il faut comprendre et mesurer.

Les points essentiels à retenir sont :

- l’optimisation peut être logicielle, matérielle ou système ;
- un code peut devenir inefficace quand l’échelle augmente ;
- les architectures modernes comme NUMA rendent le placement très important ;
- le noyau Linux possède de nombreuses interfaces de configuration ;
- `ulimit` contrôle les limites de ressources des processus ;
- `sysctl` modifie les paramètres noyau exposés par `/proc/sys` ;
- les modules noyau ajoutent des fonctionnalités configurables ;
- `nohz_full` réduit les ticks et le bruit système ;
- `isolcpus` isole des CPU du scheduler ;
- `numactl` permet le placement CPU/mémoire ;
- les cgroups isolent et limitent les ressources ;
- Flamegraph et `perf probe` permettent de diagnostiquer précisément les problèmes.

En contexte HPC, l’objectif est de réduire au maximum les perturbations du système afin que les codes de calcul utilisent efficacement le matériel.

---

## Mini-fiche ultra rapide

```text
Optimisation = rendre le système plus adapté à un objectif.

Trois niveaux :
- matériel : GPU, RDMA, CPU, NUMA ;
- logiciel : algorithme, code, accès fichiers ;
- système : noyau, modules, sysctl, ulimit, cgroups, numactl.

Commandes clés :
- ulimit -a
- sysctl -a
- cat /proc/cmdline
- lsmod
- modinfo <module>
- numactl --hardware
- ls /sys/fs/cgroup
- perf record / perf report / perf probe

HPC :
- réduire le bruit système ;
- éviter les ticks inutiles ;
- isoler les CPU ;
- contrôler le placement NUMA ;
- mesurer avant de modifier.
```