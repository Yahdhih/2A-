# Notes de cours — Systèmes de fichiers locaux

> **Module** : Architecture OS — ARSE · ENSIIE  
> **Auteur du support** : Jacques-Charles Lafoucrière  
> **Version EN** : [notes.en.md](notes.en.md)  
> **Thèmes** : supports de stockage, VFS Linux, FUSE, DOS/FAT, FFS, LFS, Ext2/3/4, ZFS/OpenZFS, NOVA, LTFS.

---

> **Synthèse rapide**
>
> | Système | Support cible | Idée clé |
> |---|---|---|
> | FAT | Disquettes / petits appareils | Table d'allocation chainée |
> | FFS | HDD | Localité (cylinder groups) |
> | LFS | HDD (petites écritures) | Tout écrire séquentiellement |
> | Ext4 | Linux généraliste | FFS + journal + extents |
> | ZFS | Stockage avancé | CoW + checksum + pool |
> | NOVA | Mémoire persistante | Log par inode, accès direct |
> | LTFS | Bandes magnétiques | Index XML + données séquentielles |
>
> **Principe fondamental** : le design d'un système de fichiers dépend toujours du support physique visé.

---

## Table des matières

1. [Vision générale du cours](#1-vision-générale-du-cours)
2. [Définitions : qu’est-ce qu’un système de fichiers ?](#2-définitions--quest-ce-quun-système-de-fichiers-)
3. [Concepts fondamentaux d’un système de fichiers](#3-concepts-fondamentaux-dun-système-de-fichiers)
4. [Types de systèmes de fichiers locaux](#4-types-de-systèmes-de-fichiers-locaux)
5. [Supports de stockage](#5-supports-de-stockage)
6. [Linux VFS : Virtual File System](#6-linux-vfs--virtual-file-system)
7. [FUSE : Filesystem in Userspace](#7-fuse--filesystem-in-userspace)
8. [DOS / FAT File System](#8-dos--fat-file-system)
9. [FFS : Berkeley Fast File System](#9-ffs--berkeley-fast-file-system)
10. [LFS : Log-Structured File System](#10-lfs--log-structured-file-system)
11. [Ext2 / Ext3 / Ext4](#11-ext2--ext3--ext4)
12. [ZFS / OpenZFS](#12-zfs--openzfs)
13. [NOVA File System](#13-nova-file-system)
14. [LTFS : Linear Tape File System](#14-ltfs--linear-tape-file-system)
15. [Synthèse comparative](#15-synthèse-comparative)
16. [Questions de révision avec réponses](#16-questions-de-révision-avec-réponses)
17. [Résumé final à retenir](#17-résumé-final-à-retenir)

---

# 1. Vision générale du cours

Le cours présente les **systèmes de fichiers locaux**. Un système de fichiers est une partie essentielle du système d’exploitation : il permet d’organiser, stocker, retrouver, modifier et sécuriser les données sur un support physique.

Le cours suit deux grandes parties :

## Day 1

- Définitions générales.
- Supports de stockage.
- Linux VFS.
- FUSE.
- DOS/FAT.

## Day 2

- FFS.
- LFS.
- Ext2/Ext3/Ext4.
- NOVA.
- LTFS.

L’idée centrale est la suivante :

> Il n’existe pas un seul système de fichiers universel. Le bon système de fichiers dépend du support matériel, des objectifs de performance, de fiabilité, de sécurité et du type d’accès aux données.

---

# 2. Définitions : qu’est-ce qu’un système de fichiers ?

Un support de stockage brut, par exemple `/dev/sdX`, peut être vu comme une très longue suite d’octets. Sans organisation, cette suite d’octets est difficilement exploitable.

Un système de fichiers sert à organiser ces données.

## Définition simple

Un système de fichiers est :

1. Une **organisation des données** sur un support de stockage.
2. Un **logiciel** qui contrôle comment les données sont stockées et récupérées.
3. Une abstraction permettant de manipuler des fichiers et des répertoires au lieu de blocs physiques.

## Objectifs principaux

Un système de fichiers cherche à optimiser plusieurs propriétés :

- **Vitesse** : accès rapide aux fichiers.
- **Flexibilité** : création, suppression, déplacement, extension de fichiers.
- **Intégrité** : garantie que les données lues correspondent aux données écrites.
- **Sécurité** : contrôle des accès aux données.
- **Fiabilité** : résistance aux pannes, arrêts brutaux, erreurs matérielles ou logicielles.

## Exemples d’optimisation selon le support

Certains systèmes de fichiers sont conçus pour des supports particuliers :

- **ISO 9660** : adapté aux disques optiques comme les CD-ROM.
- **tmpfs** : système de fichiers en mémoire vive.
- **LTFS** : système de fichiers adapté aux bandes magnétiques.
- **NOVA** : conçu pour la mémoire persistante.

---

# 3. Concepts fondamentaux d’un système de fichiers

## 3.1 Gestion de l’espace

Le système de fichiers doit gérer l’espace disponible sur le support.

Il doit être capable de :

- Allouer de l’espace pour un fichier.
- Libérer de l’espace lorsqu’un fichier est supprimé.
- Savoir quels blocs appartiennent à quels fichiers.
- Savoir quels blocs sont libres.

L’espace est souvent découpé en **blocs** ou **clusters**.

## 3.2 Fragmentation

La fragmentation apparaît lorsque les blocs d’un fichier ne sont pas contigus sur le support.

### Exemple

Un fichier peut être stocké ainsi :

```text
Bloc 10 -> Bloc 47 -> Bloc 12 -> Bloc 90
```

Au lieu de :

```text
Bloc 10 -> Bloc 11 -> Bloc 12 -> Bloc 13
```

### Conséquences

La fragmentation entraîne :

- Plus de déplacements sur disque dur.
- Des accès moins performants.
- Des lectures/écritures plus coûteuses.

Elle apparaît surtout quand :

- Beaucoup de fichiers sont créés et supprimés.
- Le support est presque plein.
- Les fichiers grandissent progressivement.

## 3.3 Organisation des fichiers et répertoires

Un système de fichiers doit organiser :

- Les fichiers.
- Les répertoires.
- Les liens symboliques.
- Les métadonnées.

## 3.4 Noms de fichiers

Un nom de fichier permet d’identifier un fichier.

Selon le système de fichiers, il peut y avoir des limites :

- Longueur maximale du nom.
- Sensibilité à la casse ou non.
- Caractères interdits.
- Encodage Unicode ou non.

### Exemple

Sous Linux, `rapport.txt` et `Rapport.txt` peuvent désigner deux fichiers différents, car beaucoup de systèmes de fichiers Linux sont sensibles à la casse.

## 3.5 Répertoires

Un répertoire sert à regrouper des fichiers.

Il peut être :

- **Plat** : tous les fichiers au même niveau.
- **Hiérarchique** : organisation en arbre avec sous-répertoires.

Exemple hiérarchique :

```text
/home/user/documents/cours/arse.pdf
```

Un répertoire peut aussi avoir des limites :

- Nombre d’entrées.
- Longueur totale du chemin.
- Format des noms.

## 3.6 Liens symboliques et jonctions

Un lien symbolique est un fichier spécial qui référence un autre fichier ou répertoire.

Exemple Linux :

```bash
ln -s /var/log logs
```

Ici, `logs` pointe vers `/var/log`.

Un lien symbolique permet d’accéder indirectement à une ressource.

## 3.7 Métadonnées

Les métadonnées décrivent les fichiers et les répertoires.

Elles incluent par exemple :

- Taille du fichier.
- Date de création.
- Date de modification.
- Date d’accès.
- Propriétaire.
- Groupe.
- Droits d’accès.
- Type du fichier.
- Pointeurs vers les blocs de données.

Il existe aussi des métadonnées globales :

- Bitmap des blocs libres.
- Carte des blocs disponibles.
- Liste des mauvais secteurs.
- Statistiques du système de fichiers.

## 3.8 Politique de mise à jour des métadonnées

La manière dont un système de fichiers met à jour ses métadonnées a un énorme impact sur :

- Les performances.
- La cohérence.
- La résistance aux crashs.
- La sécurité.

Exemple : si un fichier est créé, plusieurs informations doivent être mises à jour :

- Répertoire parent.
- Inode du fichier.
- Bitmap des blocs.
- Compteurs globaux.

Si un crash arrive au mauvais moment, le système de fichiers peut devenir incohérent.

## 3.9 Sécurité

La sécurité consiste à empêcher certains utilisateurs ou groupes de lire, modifier ou supprimer certaines données.

Les mécanismes classiques sont :

- Bits de permissions Unix.
- ACL : Access Control Lists.
- Propriétaire et groupe.
- Attributs étendus.

## 3.10 Intégrité

L’intégrité vise à garantir que :

> La donnée lue est bien la donnée qui a été écrite.

Les menaces contre l’intégrité sont :

- Panne matérielle.
- Arrêt brutal.
- Bug logiciel.
- Corruption silencieuse.
- Bit flip.

Exemple de corruption silencieuse :

```text
A = 0x41
Q = 0x51
```

Un seul bit modifié peut transformer une donnée en une autre.

## 3.11 Données utilisateur

Le but principal d’un système de fichiers est de gérer les données utilisateur :

- Stocker.
- Lire.
- Modifier.
- Supprimer.

La sémantique d’accès peut varier selon les systèmes :

- Modèle clé-valeur.
- Modèle POSIX : flux d’octets.
- Modèle OpenVMS : enregistrements.

## 3.12 Snapshots

Un snapshot est une image figée du système de fichiers à un instant donné.

### Propriétés

- Il est cohérent.
- Il permet des sauvegardes rapides.
- Il peut servir à faire une copie longue sans bloquer le système.
- Il est très utile pour les sauvegardes à chaud.

### Principe : copy-on-write

Le copy-on-write fonctionne ainsi :

1. Lors d’une modification, la nouvelle donnée est écrite ailleurs.
2. L’ancienne référence est conservée pour le snapshot.
3. La référence active est modifiée pour pointer vers la nouvelle donnée.

Ainsi :

- Le système actif voit la nouvelle version.
- Le snapshot voit l’ancienne version.

### Conséquences

Les snapshots :

- Consomment de l’espace.
- Complexifient les mises à jour.
- Peuvent ralentir certaines opérations.

Un snapshot en lecture-écriture est appelé **clone**.

## 3.13 Quotas

Les quotas permettent de contrôler l’utilisation des ressources.

On peut limiter :

- Le nombre de fichiers.
- Le nombre de blocs disque.
- Par utilisateur.
- Par groupe.
- Par projet.

### Deux limites

| Type de limite | Signification |
|---|---|
| Soft limit | Peut être dépassée temporairement pendant une période de grâce. |
| Hard limit | Ne peut jamais être dépassée. |

Les quotas sont vérifiés au moment de l’allocation.

## 3.14 Verrouillage de fichiers

Le verrouillage de fichiers sert à contrôler le partage des fichiers.

Types possibles :

- Verrou lecture seule.
- Verrou lecture-écriture.
- Verrou exclusif.

Sur les systèmes Unix, le verrouillage est souvent explicite. Sur certains systèmes comme NTFS, il peut être plus implicite.

## 3.15 Déduplication

La déduplication permet d’éviter de stocker plusieurs fois la même donnée.

Principe :

1. Le système détecte des blocs identiques.
2. Il ne garde qu’une seule copie.
3. Plusieurs références pointent vers le même bloc.
4. À la première écriture, la référence est modifiée.

C’est une forme d’optimisation proche de la compression, mais basée sur le partage de blocs identiques.

---

# 4. Types de systèmes de fichiers locaux

Les systèmes de fichiers locaux peuvent être classés selon le support visé.

| Type de support | Exemples de systèmes de fichiers |
|---|---|
| Disques durs HDD | FAT, ExtN, FFS, LFS, Versioning FS |
| Disques optiques | ISO 9660, UDF |
| Flash / SSD | NOVA, systèmes adaptés au flash |
| Mémoire vive | tmpfs |
| Bandes magnétiques | LTFS |

Conclusion importante :

> Le design d’un système de fichiers dépend fortement du support ciblé.

---

# 5. Supports de stockage

## 5.1 HDD : Hard Disk Drive

Un HDD est un support mécanique.

### Caractéristiques

- Support rotatif.
- Enregistrement magnétique.
- Données enregistrées sur des cercles concentriques appelés pistes.
- Temps d’accès dépendant du déplacement mécanique.

### Impact sur le système de fichiers

Un système de fichiers pour HDD doit limiter :

- Les déplacements de tête.
- Les lectures aléatoires inutiles.
- La fragmentation.

C’est pourquoi FFS utilise l’idée de **localité** : placer les données liées proches les unes des autres.

## 5.2 Futur des HDD

Les HDD de très grande capacité posent des défis techniques importants.

Exemple évoqué dans le cours : pour atteindre des capacités de l’ordre de 50 TB, la tête de lecture/écriture doit se déplacer très vite tout en restant positionnée avec une précision nanométrique.

## 5.3 SMR : Shingled Magnetic Recording

SMR signifie **Shingled Magnetic Recording**.

L’idée est d’augmenter la densité de stockage en superposant partiellement les pistes, comme des tuiles.

### Avantages

- Plus grande densité.
- Intéressant pour les écritures séquentielles.
- Bon pour les lectures.

### Inconvénients

- Les écritures aléatoires deviennent plus complexes.
- La réécriture peut nécessiter de réécrire plusieurs zones.

## 5.4 SSD : Solid State Drive

Un SSD est basé sur de la mémoire flash.

### Caractéristiques

- Pas de mouvement mécanique.
- Accès plus uniforme.
- Enregistrement électronique basé sur des transistors.
- Très bon temps d’accès.

### Problèmes spécifiques au flash

La mémoire flash nécessite une gestion spéciale :

- On ne peut pas simplement réécrire une donnée en place.
- Il faut effacer avant d’écrire.
- L’effacement se fait par blocs.
- Les cellules ont un nombre limité d’écritures.
- Il faut du garbage collection.

Le système de fichiers peut aider le SSD à garder de bonnes performances.

## 5.5 Zoned Devices

Les zoned devices sont une nouvelle classe de stockage.

### Principe

L’espace d’adressage est divisé en zones.

Chaque zone possède :

- Une contrainte d’écriture.
- Un pointeur d’écriture.
- Une obligation d’écriture séquentielle.

On ne peut pas écraser directement une donnée dans une zone. Il faut d’abord réinitialiser la zone avec une commande spéciale.

## 5.6 Bandes magnétiques

Les bandes sont des supports linéaires.

### Caractéristiques

- Longueur de bande supérieure à 800 m.
- Enregistrement magnétique.
- Enregistrement serpentin.
- Accès séquentiel.

Elles sont très bonnes pour :

- Archivage.
- Gros volumes.
- Stockage longue durée.

Mais elles sont mauvaises pour :

- Accès aléatoire.
- Petites lectures dispersées.

## 5.7 Comparaison HDD / SSD / Tape

| Critère | HDD | SSD | Bande magnétique |
|---|---|---|---|
| Capacité | Bonne | Bonne | Très grande |
| Temps d’accès | Environ 10 ms | Quelques ms ou moins | Minutes possibles |
| Débit | Quelques centaines MB/s | Plusieurs centaines MB/s | Bon en streaming |
| Accès aléatoire | Correct | Très bon | Mauvais |
| Accès séquentiel | Très bon | Très bon | Très bon si continu |
| Consommation | Moyenne | Variable | Très bonne hors utilisation |
| Durée de vie | Bonne | Dépend de l’usage | Très bonne |

Conclusion :

> Aucun support n’est meilleur dans tous les cas. Le système de fichiers doit être adapté au support.

---

# 6. Linux VFS : Virtual File System

## 6.1 Définition

VFS signifie **Virtual File System**.

C’est une sous-couche du noyau Linux qui fournit une interface commune pour accéder à différents systèmes de fichiers.

## 6.2 Problème résolu par VFS

Sans VFS, chaque application devrait connaître les détails de chaque système de fichiers.

Avec VFS :

- Les applications utilisent les appels système standards.
- Le noyau redirige vers le bon système de fichiers.
- Plusieurs systèmes de fichiers peuvent coexister.

Exemple :

```bash
cp fichier1 /mnt/usb/fichier2
```

La commande `cp` n’a pas besoin de savoir si la source est sur ext4 et la destination sur FAT32.

## 6.3 Pile d’appel pour une écriture

Exemple :

```c
write(file_descriptor, &buffer, length)
```

Le chemin logique est :

```text
User space
   |
   v
write()
   |
   v
sys_write()
   |
   v
VFS
   |
   v
Méthode write du système de fichiers
   |
   v
Block driver
   |
   v
Media
```

## 6.4 Concepts VFS

VFS suit les abstractions Unix :

- Fichier.
- Répertoire.
- Point de montage.
- Inode.
- Espace de noms global.

Tous les systèmes de fichiers montés apparaissent dans un arbre unique.

Exemple :

```text
/
├── home
├── var
├── mnt
│   └── usb
└── proc
```

## 6.5 Objets VFS

VFS est orienté objet : chaque objet contient des données et des méthodes.

Les principaux objets sont :

| Objet | Rôle |
|---|---|
| Superblock | Représente un système de fichiers monté. |
| Inode | Représente un fichier ou répertoire. |
| Dentry | Représente une composante de chemin. |
| File | Représente un fichier ouvert par un processus. |

## 6.6 Superblock object

Le superblock représente un système de fichiers monté.

Il contient :

- Taille des blocs.
- Taille maximale des fichiers.
- Type du système de fichiers.
- Magic number.
- Flags.
- Listes d’inodes dirty.
- Pointeurs vers des méthodes.

Il correspond souvent au superblock réel du système de fichiers local.

## 6.7 Méthodes du superblock

Le superblock possède des méthodes telles que :

- Allocation d’inode.
- Destruction d’inode.
- Écriture d’inode.
- Synchronisation du système de fichiers.
- Gel/dégel du système de fichiers.
- Remontage.
- Lecture/écriture des quotas.

## 6.8 Inode object

Un inode représente un fichier ou un répertoire manipulé par le noyau.

Il contient :

- Compteurs de références.
- Métadonnées Unix.
- Numéro d’inode.
- Pointeurs vers les méthodes.

L’inode est central dans Unix : il décrit le fichier, mais ne contient pas son nom.

## 6.9 Méthodes de l’inode

Les opérations possibles sur un inode incluent :

- `lookup` : rechercher un nom.
- `permission` : vérifier les droits.
- `create` : créer un fichier.
- `link` : créer un lien dur.
- `unlink` : supprimer une entrée.
- `mkdir` : créer un répertoire.
- `rmdir` : supprimer un répertoire.
- `rename` : renommer.
- `getattr` / `setattr` : lire ou modifier les attributs.

## 6.10 Dentry object

Une dentry représente une composante d’un chemin.

Exemple :

```text
/home/user/file.txt
```

Les composants sont :

```text
home, user, file.txt
```

Chaque composant peut correspondre à une dentry.

La dentry n’a pas forcément d’équivalent direct sur disque.

### États possibles

| État | Signification |
|---|---|
| Used | Référence active. |
| Unused | Plus de référence active. |
| Negative | Le nom n’existe pas, `d_inode = NULL`. |

Les dentries négatives sont utiles pour accélérer les recherches de fichiers inexistants.

## 6.11 File object

Un objet `file` représente un fichier ouvert par un processus.

Important : plusieurs objets `file` peuvent exister pour le même fichier réel.

Il contient :

- Compteur de références.
- Offset courant.
- Référence à la dentry.
- Méthodes de lecture/écriture.

## 6.12 Méthodes du fichier

Les méthodes d’un objet fichier incluent :

- `read`.
- `write`.
- `open`.
- `release`.
- `fsync`.
- `mmap`.
- `llseek`.
- `ioctl`.
- `poll`.
- `lock`.
- `fallocate`.
- `copy_file_range`.
- `dedupe_file_range`.

---

# 7. FUSE : Filesystem in Userspace

## 7.1 Idée générale

FUSE permet d’implémenter un système de fichiers en espace utilisateur au lieu de l’implémenter directement dans le noyau.

Il existe deux endroits possibles pour exécuter un système de fichiers :

1. **Kernel space**.
2. **User space**.

## 7.2 Système de fichiers dans le noyau

Avantage :

- Meilleures performances pour les accès locaux.

Inconvénients :

- Développement plus difficile.
- Risque de crash du noyau.
- Debug compliqué.
- Langages limités.

## 7.3 Système de fichiers en espace utilisateur

Avantages :

- Interfaces plus stables et documentées.
- Portabilité plus facile : Linux, BSD, macOS.
- Possibilité d’utiliser des langages comme Python.
- Debug plus simple.
- Réseau et cache plus faciles à gérer.
- Peut tourner sans privilèges élevés.
- Meilleure résilience : si le système de fichiers crash, le système entier ne crash pas forcément.

Inconvénient principal :

- Performances généralement moins bonnes que dans le noyau.

## 7.4 Architecture FUSE

Schéma logique :

```text
Application
   |
   v
libc / appels système
   |
   v
VFS
   |
   v
FUSE kernel driver
   |
   v
/dev/fuse
   |
   v
FUSE daemon en user space
   |
   v
Système de fichiers FUSE
```

## 7.5 Développer un système FUSE

FUSE propose une interface riche avec environ 40 opérations possibles.

Mais beaucoup d’opérations ont des comportements par défaut.

Un système minimal peut se contenter de quelques opérations.

### Opérations minimales fréquentes

- `getattr` : obtenir les attributs.
- `readdir` : lire un répertoire.
- `open` : ouvrir un fichier.
- `read` : lire un fichier.

Exemple en C :

```c
static struct fuse_operations simple_oper = {
    .getattr = simple_getattr,
    .readdir = simple_readdir,
    .open    = simple_open,
    .read    = simple_read,
};
```

## 7.6 Grandes familles d’opérations FUSE

- Opérations sur répertoires.
- Opérations sur fichiers.
- Opérations sur métadonnées.
- Verrouillage.
- Initialisation et destruction.

## 7.7 Monter et démonter un système FUSE

### Monter

```bash
./my_new_fs DIR
```

### Démonter

```bash
fusermount -u DIR
```

---

# 8. DOS / FAT File System

## 8.1 Origine

Le système de fichiers DOS a été conçu pour MS-DOS.

Il était initialement prévu pour des disquettes de petite capacité, par exemple 360 KB.

## 8.2 Caractéristiques

- Système simple.
- Disponible sur presque toutes les plateformes.
- Utilisé dans beaucoup de petits appareils, comme les appareils photo.
- Format de nom simple : 8.3.
- Support ultérieur des noms longs.
- Initialement espace de noms plat.
- Support des répertoires ajouté avec DOS 2.
- Chemins limités historiquement.

## 8.3 FAT : File Allocation Table

DOS/FAT repose sur une table d’allocation des fichiers.

Variantes :

- FAT12.
- FAT16.
- FAT32.

## 8.4 Organisation du disque DOS

Structure générale :

```text
Reserved sectors
File Allocation Tables
Root directory
Data area
Hidden sectors
```

## 8.5 Reserved sectors

Les secteurs réservés contiennent notamment :

- Le secteur de boot.
- Les caractéristiques du disque.
- Le nombre de copies de la FAT.

## 8.6 File Allocation Table

La FAT contient une entrée pour chaque bloc logique du volume.

Chaque entrée indique :

- Bloc libre.
- Ou numéro du prochain bloc logique du fichier.

Un fichier est donc une chaîne de clusters.

Exemple logique :

```text
Cluster 5 -> Cluster 8 -> Cluster 20 -> EOF
```

## 8.7 Root directory

Le répertoire racine contient les entrées des fichiers.

Chaque entrée contient le numéro du premier cluster du fichier.

## 8.8 Format d’une entrée de répertoire

Une entrée de répertoire contient notamment :

| Octets | Contenu |
|---|---|
| 0-7 | Nom du fichier ou volume |
| 8-10 | Extension |
| 11 | Attribut |
| 12-21 | Inutilisé |
| 22-23 | Heure |
| 24-25 | Date |
| 26-27 | Premier cluster |
| 28-31 | Taille du fichier |

Codes particuliers :

- `0xE5` ou `0x05` : entrée supprimée.
- `0x00` : dernière entrée.

## 8.9 Avantages et limites de FAT

### Avantages

- Très simple.
- Très portable.
- Facile à implémenter.
- Compatible avec beaucoup de systèmes.

### Limites

- Peu robuste.
- Gestion limitée de la sécurité.
- Fragmentation possible.
- Pas adapté aux très grands systèmes modernes sans adaptations.

---

# 9. FFS : Berkeley Fast File System

## 9.1 Origine

FFS signifie **Fast File System**.

Il est aussi appelé UFS, Unix File System.

Il date de 1984 et vient de Berkeley.

## 9.2 Objectifs

FFS a été conçu pour optimiser les accès aux disques durs.

L’idée majeure est :

> Placer les données associées et les métadonnées proches les unes des autres pour réduire les déplacements de tête du disque.

## 9.3 Disponibilité

FFS a été utilisé par de nombreux systèmes Unix :

- FreeBSD.
- OpenBSD.
- NetBSD.
- Linux en lecture seule dans certains cas.
- Ancien MacOS.

## 9.4 Design de FFS

Structure :

```text
Boot blocks
Superblock
Cylinder groups
    - Copy of superblock
    - Cylinder group header
    - Inodes
    - Data blocks
```

## 9.5 Boot blocks

Les boot blocks sont réservés au système d’exploitation.

Ils ne sont pas vraiment utilisés par le système de fichiers lui-même.

## 9.6 Superblock

Le superblock contient :

- Magic number.
- Géométrie du système de fichiers.
- Statistiques.
- Paramètres de tuning.

## 9.7 Cylinder groups

Un cylinder group regroupe :

- Une copie du superblock.
- Un header.
- Des statistiques.
- Des listes de blocs libres.
- Des inodes.
- Des blocs de données.

But : améliorer la localité.

## 9.8 Bitmaps

FFS utilise des bitmaps pour gérer l’espace libre.

Avantages :

- Rapide.
- Efficace pour trouver un grand espace libre.
- Possible de faire du pattern matching.

## 9.9 Création d’un fichier

Créer un fichier implique plusieurs étapes :

1. Allouer un inode.
2. Mettre à jour la bitmap d’inodes.
3. Allouer des blocs de données.
4. Mettre à jour la bitmap de données.
5. Ajouter une entrée dans le répertoire.
6. Modifier le contenu du répertoire.
7. Mettre à jour l’inode du répertoire.
8. Mettre à jour les compteurs globaux.

La politique d’allocation a donc un impact énorme sur les performances.

## 9.10 Structure des blocs dans l’inode

Un inode peut pointer vers les données avec :

- Des blocs directs.
- Un bloc indirect simple.
- Un bloc indirect double.
- Un bloc indirect triple.

Schéma simplifié :

```text
Inode
├── Direct block 1 -> Data
├── Direct block 2 -> Data
├── Single indirect -> table -> Data
├── Double indirect -> table -> table -> Data
└── Triple indirect -> table -> table -> table -> Data
```

## 9.11 Contenu d’un inode

Un inode contient :

- Type du fichier.
- Mode d’accès.
- Numéro d’inode.
- Dates.
- Taille.
- Nombre de blocs.
- Nombre d’entrées de répertoire pointant vers ce fichier.
- Numéro de génération.
- Taille de bloc.
- Taille des attributs étendus.
- Pointeurs vers les blocs de données.

Important :

> L’inode ne contient pas le nom du fichier. Le nom est stocké dans le répertoire.

Conséquence :

- Plusieurs noms peuvent pointer vers le même inode.
- C’est le principe du lien dur.

## 9.12 Répertoires dans FFS

Un répertoire contient des entrées.

Chaque entrée contient :

- Numéro d’inode.
- Taille de l’entrée.
- Type d’entrée.
- Longueur du nom.
- Nom.

Lors de la création d’une nouvelle entrée, le système :

1. Vérifie si le nom existe déjà.
2. Cherche un espace libre suffisant.
3. Compacte si nécessaire.
4. Crée l’entrée.

## 9.13 Soft Updates

FFS doit maintenir la cohérence globale des métadonnées même en cas de crash.

Avant, beaucoup de systèmes utilisaient des écritures synchrones, mais cela pénalisait les performances.

Les soft updates organisent les dépendances entre mises à jour.

Idée :

```text
1. Décrire l’opération
2. Changer les données
3. Valider
```

Opérations concernées :

- Création de fichier/répertoire.
- Suppression.
- Renommage.
- Allocation de blocs.
- Manipulation de blocs indirects.
- Gestion de la carte libre.

## 9.14 Résumé FFS

FFS a introduit de nombreuses idées importantes :

- Organisation consciente du support.
- Localité des données.
- Soft updates.
- Noms longs.
- Liens symboliques.
- Renommage atomique.

Beaucoup de systèmes modernes reprennent des idées de FFS.

---

# 10. LFS : Log-Structured File System

## 10.1 Origine

LFS apparaît dans les années 1990.

À cette époque :

- Il y a plus de mémoire disponible pour le cache.
- Le trafic disque doit être optimisé pour les écritures.
- Les accès aléatoires sont mauvais.
- Les créations de petits fichiers posent problème aux systèmes classiques.

## 10.2 Idée centrale

LFS transforme les écritures en grandes écritures séquentielles.

Au lieu de modifier les données en place :

- Les mises à jour sont tamponnées en mémoire.
- Quand un segment est plein, il est écrit séquentiellement sur disque.
- Les anciennes versions ne sont pas écrasées immédiatement.

## 10.3 Avantage principal

Les écritures séquentielles sont beaucoup plus efficaces sur disque dur.

LFS est donc performant pour des charges avec beaucoup de petites écritures.

## 10.4 Problème : retrouver les inodes

Comme les données et inodes sont écrits partout sur le disque, les inodes n’ont plus une position fixe.

Solution : ajouter une **inodemap**.

L’inodemap est une table d’indirection qui indique où se trouve chaque inode.

## 10.5 Problème : retrouver l’inodemap

L’inodemap elle-même peut changer.

Solution : utiliser une zone fixe appelée **checkpoint region**.

Elle est mise à jour rarement.

## 10.6 Gestion de l’espace

Comme LFS n’écrase jamais directement, il faut récupérer les anciens blocs inutiles.

Cela se fait par **garbage collection**.

## 10.7 Détection des blocs vivants

Un bloc peut contenir :

- Le numéro d’inode.
- L’offset dans le fichier.

On compare ces informations avec la description actuelle de l’inode pour savoir si le bloc est encore vivant.

## 10.8 Politique de nettoyage

Le garbage collection peut être basé sur la température des segments.

Un segment froid contient des données peu modifiées.  
Un segment chaud contient des données fréquemment modifiées.

## 10.9 Limite principale

Le garbage collection est difficile.

C’est le principal inconvénient de l’approche LFS.

---

# 11. Ext2 / Ext3 / Ext4

## 11.1 Origine de Ext

Ext est le premier système de fichiers Linux après Minix, en 1992.

Il corrige deux limites de Minix :

- Taille maximale de 64 MB.
- Noms de fichiers limités à 14 caractères.

## 11.2 Ext2

Ext2 apparaît en 1993.

Il est conçu selon les principes de FFS.

## 11.3 Ext3

Ext3 ajoute notamment :

- Journaling.
- Croissance en ligne du système de fichiers.
- Indexation HTree pour les grands répertoires.

## 11.4 Ext4

Ext4 ajoute :

- Support des très grands systèmes de fichiers.
- Extents.
- Pré-allocation persistante.
- Delayed allocation.
- Nombre illimité de sous-répertoires.
- Checksumming du journal.
- Checksumming des métadonnées.
- Allocateurs multiblocs.
- Timestamps améliorés.

## 11.5 Ext4 et journaling

Ext4 est proche de FFS mais ajoute un mécanisme de journalisation.

Le journaling permet de garder une trace des opérations avant leur validation définitive.

## 11.6 Trois modes de journalisation Ext4

| Mode | Principe | Sécurité |
|---|---|---|
| Journal | Toutes les mises à jour passent par le journal. | Plus sûr mais plus coûteux. |
| Ordered | Seules les métadonnées sont journalisées, mais les données sont écrites avant le commit des métadonnées. | Bon compromis. |
| Writeback | Seules les métadonnées sont journalisées, sans règle stricte sur les données. | Plus rapide mais moins sûr. |

## 11.7 Large file system

Ext4 peut gérer :

- Systèmes de fichiers jusqu’à environ 1 EiB.
- Fichiers jusqu’à environ 16 TiB.

## 11.8 Extents

Les extents remplacent les blocs fixes par des couples :

```text
offset / length
```

Au lieu de stocker chaque bloc individuellement, on stocke une plage continue.

Exemple :

```text
Début = bloc 1000, longueur = 300 blocs
```

C’est plus efficace pour les gros fichiers.

## 11.9 HTree

Ext4 utilise des index HTree pour gérer efficacement les grands répertoires.

Cela permet de chercher plus rapidement une entrée dans un répertoire très volumineux.

## 11.10 Timestamps améliorés

Ext4 améliore la précision temporelle avec des timestamps en nanosecondes.

---

# 12. ZFS / OpenZFS

## 12.1 Origine

ZFS signifie **Zettabyte File System**.

Il a été conçu entre 2001 et 2004.

ZFS combine deux couches traditionnellement séparées :

- Gestionnaire de volumes.
- Système de fichiers.

## 12.2 Idées principales

ZFS repose sur plusieurs idées fortes :

- Ne jamais écraser les données existantes.
- Écrire les changements en mémoire puis les flusher dans un état cohérent.
- Passer d’un état cohérent à un autre état cohérent.
- Tout vérifier par checksum.
- Fusionner gestion des disques et système de fichiers.

## 12.3 Avantages

- Snapshots et clones efficaces.
- Très bonne intégrité.
- Pas de risque classique de corruption partielle lors d’un crash.
- Protection contre la corruption silencieuse grâce aux checksums.
- Gestion flexible de l’espace.

## 12.4 Disponibilité

- Solaris : design original.
- BSD : OpenZFS.
- Linux : OpenZFS, mais pas intégré directement au noyau Linux à cause de problèmes de licence.

## 12.5 Modules ZFS

ZFS est organisé en plusieurs couches :

- ZPL : ZFS POSIX Layer.
- ZAP : ZFS Attribute Processor.
- DMU : Data Management Unit.
- ZIL : ZFS Intent Log.
- Dataset and Snapshot Layer.
- SPA : Storage Pool Allocator.
- ZVOL : ZFS Volume.
- ZFS I/O.
- RAIDZ.
- VDEV : Virtual Device.
- ARC : Adaptive Replacement Cache.

## 12.6 Pools ZFS

ZFS crée un pool d’espace.

Les systèmes de fichiers et volumes consomment de l’espace dans ce pool dynamiquement.

Cela permet de déplacer l’espace entre systèmes de fichiers selon les besoins.

## 12.7 Uberblock

L’uberblock est au sommet de la structure ZFS.

Il pointe vers un ensemble de méta-objets.

Ces méta-objets peuvent représenter :

- Systèmes de fichiers.
- Snapshots.
- Clones.
- ZVOL.
- Space map.

## 12.8 Dnode et Znode

Les métadonnées des objets ZFS sont stockées dans des **dnodes**.

Le DMU gère ces dnodes.

Les métadonnées POSIX sont stockées dans des **znodes**.

Le ZPL gère les znodes.

## 12.9 ZAP objects

Les objets ZAP stockent des tables clé-valeur.

Ils servent notamment aux répertoires :

```text
nom -> numéro d’objet
```

## 12.10 Block pointer ZFS

Un pointeur de bloc ZFS a une taille de 128 octets.

Il contient :

- Pointeurs vers jusqu’à trois copies.
- Device.
- Offset.
- Taille.
- Checksum.

Chaque bloc est checksummé.

## 12.11 RAIDZ

RAIDZ supporte des stripes de taille variable.

Après une panne, ZFS ne reconstruit que l’espace utilisé, pas nécessairement tout le disque.

## 12.12 Allocation et libération des blocs

L’allocation est gérée par le SPA.

Les blocs libres sont décrits par des space maps.

Pour libérer des blocs, ZFS compare la date de naissance du bloc avec le plus jeune snapshot.

S’il est encore utilisé par un snapshot, il ne peut pas être libéré.

## 12.13 Tradeoffs ZFS

### Avantages

- Reconstruction plus intelligente.
- Pas d’écrasement en place.
- Évite le problème du write-hole RAID.
- Snapshots efficaces.
- Intégrité forte.
- Gestion d’espace partagée.

### Inconvénients

- Besoin d’espace libre pour bien fonctionner, souvent au moins 25 %.
- Consommation mémoire importante avec ARC.
- Intégration Linux compliquée par la licence.

## 12.14 Btrfs

Btrfs reprend certaines idées de ZFS mais a été conçu pour le noyau Linux.

Il est :

- Compatible avec la licence du noyau Linux.
- Moins mature historiquement que ZFS.

---

# 13. NOVA File System

## 13.1 Origine

NOVA est conçu pour la mémoire persistante.

La mémoire persistante est :

- Rapide.
- Adressable octet par octet.
- Persistante après arrêt.

Deux approches existent :

1. Adapter des systèmes existants.
2. Créer un nouveau système de fichiers.

NOVA est un système de fichiers de recherche.

## 13.2 Disponibilité

NOVA est disponible sous Linux.

## 13.3 Concepts

NOVA est intégré au noyau mais ne passe pas par la couche bloc classique.

Il utilise un stockage mappé directement dans l’espace d’adressage du noyau.

## 13.4 Conséquence

Cela simplifie beaucoup :

- Pas de coalescence de requêtes.
- Pas de gestion classique de file d’attente.
- Pas de priorisation de requêtes bloc.

Le système accède directement à la mémoire persistante.

## 13.5 Structure NOVA

NOVA contient :

- Superblock.
- Table d’inodes.
- Free space table.

## 13.6 Superblock

Le superblock est la structure de plus haut niveau.

Il contient :

- Statistiques.
- Compteurs.

## 13.7 Table d’inodes

La table d’inodes est organisée en tableaux par CPU.

Avantage :

- Chaque CPU peut allouer des inodes sans locks inter-processeurs coûteux.

## 13.8 Free Space Table

La table d’espace libre est organisée avec des arbres rouge-noir par CPU.

Ces structures sont gérées en mémoire puis flushées au démontage.

Elles peuvent aussi être reconstruites en scannant le périphérique.

## 13.9 Design des inodes NOVA

Physiquement, un inode est une paire de pointeurs.

Logiquement, il est représenté par un log décrivant les changements appliqués au fichier.

## 13.10 Entrées de log NOVA

Une entrée peut décrire :

- Modification des attributs du fichier.
- Ajout d’une entrée dans un répertoire.
- Ajout d’un lien vers un fichier.
- Écriture de données.

## 13.11 Écriture d’un fichier vide

Étapes :

1. Allouer la mémoire nécessaire depuis la liste libre du CPU.
2. Copier les données dans cet espace.
3. Ajouter une entrée au log de l’inode.
4. Indiquer la nouvelle taille du fichier.
5. Indiquer le pointeur vers les données.
6. Mettre à jour atomiquement le pointeur de fin de log.

## 13.12 Réécriture de données

NOVA utilise le copy-on-write.

Étapes :

1. Allouer une nouvelle zone mémoire.
2. Copier les anciennes données utiles.
3. Ajouter les nouvelles données.
4. Ajouter une nouvelle entrée de log.
5. Mettre à jour le pointeur de fin.
6. Invalider l’ancienne entrée de log.
7. Libérer ou réutiliser les anciennes pages.

## 13.13 Format logique

Le format disque peut être vu comme une suite d’instructions.

En les rejouant dans l’ordre, en ignorant les entrées invalidées, on reconstruit l’état complet du fichier.

## 13.14 Limitations

NOVA a plusieurs limites :

- Encore en développement.
- Support x86-64 uniquement.
- Impossible de déplacer le système de fichiers d’un serveur à un autre si le nombre de CPU est différent.
- Pas d’ACL.
- Pas de quotas.
- Pas de `fsck`.

---

# 14. LTFS : Linear Tape File System

## 14.1 Origine

LTFS signifie **Linear Tape File System**.

Il est conçu pour les bandes magnétiques.

Les bandes sont des flux linéaires d’octets, avec accès séquentiel.

LTFS est un standard SNIA.

## 14.2 Objectifs

LTFS permet :

- De rendre les bandes plus portables entre systèmes d’exploitation.
- De manipuler une bande avec des opérations de fichiers standard.
- De ne pas dépendre uniquement d’outils comme `tar`, `dump` ou `cpio`.

## 14.3 Disponibilité

LTFS existe sur :

- Linux.
- Windows.
- macOS.

## 14.4 Concepts

Une bande est divisée en volumes.

Un volume contient :

- Des fichiers de données.
- Des fichiers de métadonnées.

Le volume décrit entièrement la structure des répertoires et des fichiers.

LTFS peut être monté comme un système de fichiers.

## 14.5 Format LTFS

Un volume LTFS contient deux partitions :

1. Partition d’index.
2. Partition de données.

## 14.6 Label

Le label identifie le volume.

Il contient par exemple :

- Version LTFS.
- Créateur.
- Date de formatage.
- UUID du volume.
- Partition d’index.
- Partition de données.
- Taille de bloc.
- Compression activée ou non.

## 14.7 Index

L’index est une structure XML.

Il contient :

- Un numéro de génération.
- Un pointeur vers lui-même.
- Un pointeur vers l’index précédent.
- Les informations sur les fichiers et répertoires.
- La politique de placement des données.
- Les attributs étendus.

## 14.8 Chaînage des index

Chaque nouvel index peut référencer l’index précédent.

Cela permet de retrouver l’historique de la structure du volume.

## 14.9 Répertoire dans l’index LTFS

Un répertoire est décrit en XML avec :

- `fileuid`.
- Nom.
- Dates.
- Attribut lecture seule.
- Contenu.
- Sous-répertoires.
- Fichiers.

## 14.10 Fichier dans l’index LTFS

Un fichier contient :

- Identifiant.
- Nom.
- Taille.
- Dates.
- Attribut lecture seule.
- Attributs étendus.
- Extents indiquant où se trouvent les données.

## 14.11 Extended attributes

LTFS supporte des attributs étendus dans l’index XML.

Les valeurs peuvent être :

- Texte.
- Base64.
- Valeur vide.

## 14.12 Points forts et limites

### Points forts

- Portabilité.
- Standardisation.
- Accès via opérations standard OS.
- Adapté à l’archivage.

### Limites

- Accès naturellement séquentiel.
- Pas adapté aux accès aléatoires fréquents.
- Les mouvements de bande coûtent cher.

---

# 15. Synthèse comparative

## 15.1 Comparaison des systèmes de fichiers étudiés

| Système | Support cible | Idée principale | Points forts | Limites |
|---|---|---|---|---|
| DOS/FAT | Disquettes, petits supports, appareils | Table d’allocation | Simple, portable | Peu sécurisé, peu robuste |
| FFS | HDD | Localité, cylinder groups | Performant sur HDD, idées fondatrices | Moins adapté aux disques modernes abstraits |
| LFS | HDD avec beaucoup d’écritures | Tout écrire séquentiellement | Très bon pour petites écritures | Garbage collection difficile |
| Ext2 | Linux classique | Inspiré de FFS | Simple, stable | Pas de journal |
| Ext3 | Linux | Ext2 + journal | Meilleure robustesse | Moins moderne qu’Ext4 |
| Ext4 | Linux moderne | Extents, journal, grands FS | Performant, robuste | Complexité accrue |
| ZFS | Stockage avancé | Copy-on-write, checksum, pool | Intégrité forte, snapshots | Mémoire, licence Linux |
| NOVA | Mémoire persistante | Logs par inode, accès direct | Adapté au PMEM | Recherche, limitations |
| LTFS | Bandes | Index XML + données séquentielles | Archivage portable | Accès aléatoire mauvais |

## 15.2 Concepts transversaux importants

Les concepts à maîtriser absolument sont :

- Bloc.
- Cluster.
- Inode.
- Répertoire.
- Superblock.
- Métadonnées.
- Fragmentation.
- Allocation.
- Journalisation.
- Copy-on-write.
- Snapshot.
- Quota.
- ACL.
- VFS.
- FUSE.
- Garbage collection.
- Checksum.
- Extent.

## 15.3 Choix d’un système de fichiers selon le besoin

| Besoin | Système adapté |
|---|---|
| Compatibilité maximale avec petits appareils | FAT/FAT32 |
| Système Linux généraliste | Ext4 |
| Snapshots et forte intégrité | ZFS/OpenZFS |
| Beaucoup de petites écritures séquentialisables | LFS |
| Mémoire persistante | NOVA |
| Archivage sur bande | LTFS |
| Prototype de système de fichiers | FUSE |

---

# 16. Questions de révision avec réponses

## Question 1

**Qu’est-ce qu’un système de fichiers ?**

Un système de fichiers est une organisation des données et un logiciel qui permet de stocker, retrouver, modifier et sécuriser les fichiers sur un support de stockage.

## Question 2

**Pourquoi un support brut ne suffit-il pas ?**

Parce qu’un support brut est seulement une longue suite d’octets. Il faut organiser ces octets en fichiers, répertoires, métadonnées et espaces libres.

## Question 3

**Qu’est-ce que la fragmentation ?**

La fragmentation apparaît quand les blocs d’un fichier ne sont pas contigus. Elle réduit les performances, surtout sur HDD.

## Question 4

**Pourquoi les métadonnées sont-elles importantes ?**

Elles décrivent les fichiers : taille, droits, dates, propriétaire, localisation des blocs, etc. Sans elles, le système ne peut pas gérer correctement les données.

## Question 5

**Qu’est-ce qu’un snapshot ?**

C’est une image cohérente et figée d’un système de fichiers à un instant donné.

## Question 6

**Qu’est-ce que le copy-on-write ?**

C’est une technique où une modification est écrite à un nouvel emplacement au lieu d’écraser l’ancien bloc. L’ancienne version reste disponible pour les snapshots.

## Question 7

**Pourquoi les quotas sont-ils utiles ?**

Ils permettent de limiter l’espace disque ou le nombre de fichiers utilisés par un utilisateur, un groupe ou un projet.

## Question 8

**Quel est le rôle de Linux VFS ?**

VFS fournit une interface commune permettant aux applications d’utiliser plusieurs systèmes de fichiers sans connaître leurs détails internes.

## Question 9

**Quels sont les objets principaux de VFS ?**

Les objets principaux sont : superblock, inode, dentry et file.

## Question 10

**Quelle est la différence entre inode et dentry ?**

L’inode décrit un fichier ou répertoire. La dentry représente une composante d’un chemin et relie un nom à un inode.

## Question 11

**Pourquoi l’inode ne contient-il pas le nom du fichier ?**

Parce que le nom est stocké dans le répertoire. Cela permet à plusieurs noms de pointer vers le même inode, ce qui permet les liens durs.

## Question 12

**Qu’est-ce que FUSE ?**

FUSE permet d’implémenter un système de fichiers en espace utilisateur, avec plus de simplicité et de sécurité qu’un développement noyau.

## Question 13

**Pourquoi FUSE est-il utile pour le développement ?**

Parce qu’il permet de développer plus facilement, de déboguer plus simplement, d’utiliser des langages comme Python et d’éviter qu’un crash du FS fasse tomber le noyau.

## Question 14

**Quelle est l’idée principale de FAT ?**

FAT utilise une table d’allocation où chaque entrée indique le prochain cluster d’un fichier.

## Question 15

**Pourquoi FAT est-il encore utilisé ?**

Parce qu’il est simple, portable et supporté par presque toutes les plateformes.

## Question 16

**Quelle est l’idée principale de FFS ?**

FFS cherche à améliorer les performances des HDD en plaçant les données liées proches les unes des autres grâce aux cylinder groups.

## Question 17

**Qu’est-ce qu’un cylinder group ?**

C’est une zone regroupant inodes, données, bitmaps et copie de superblock pour favoriser la localité.

## Question 18

**À quoi servent les bitmaps dans FFS ?**

Elles permettent de suivre efficacement les blocs et inodes libres.

## Question 19

**Qu’est-ce qu’un bloc indirect ?**

C’est un bloc qui contient des pointeurs vers d’autres blocs de données ou vers d’autres blocs indirects.

## Question 20

**Quelle est l’idée principale de LFS ?**

LFS transforme les mises à jour en grandes écritures séquentielles pour améliorer les performances d’écriture.

## Question 21

**Quel est le principal problème de LFS ?**

Le garbage collection, nécessaire pour récupérer les anciens blocs inutiles.

## Question 22

**Quelle est la différence entre Ext2, Ext3 et Ext4 ?**

Ext2 est inspiré de FFS sans journal. Ext3 ajoute le journaling. Ext4 ajoute extents, grands volumes, checksums, allocations améliorées et timestamps plus précis.

## Question 23

**Quels sont les trois modes de journaling Ext4 ?**

Journal, Ordered et Writeback.

## Question 24

**Qu’est-ce qu’un extent ?**

Un extent décrit une plage continue de blocs par un début et une longueur, au lieu de lister chaque bloc individuellement.

## Question 25

**Quelle est l’idée principale de ZFS ?**

ZFS combine système de fichiers et gestion de volumes, utilise le copy-on-write, les checksums et passe d’un état cohérent à un autre.

## Question 26

**Pourquoi ZFS est-il très fort en intégrité ?**

Parce que tous les blocs sont checksummés et que les données ne sont pas écrasées en place.

## Question 27

**Pourquoi ZFS n’est-il pas directement intégré au noyau Linux ?**

À cause de problèmes de licence.

## Question 28

**À quoi sert ARC dans ZFS ?**

ARC est le cache adaptatif de ZFS. Il améliore les performances mais consomme beaucoup de mémoire.

## Question 29

**Pour quel support NOVA est-il conçu ?**

NOVA est conçu pour la mémoire persistante rapide, adressable par octet.

## Question 30

**Pourquoi NOVA ne passe-t-il pas par la couche bloc ?**

Parce qu’il utilise un stockage mappé directement dans l’espace d’adressage du noyau.

## Question 31

**Quelle est l’idée des logs par inode dans NOVA ?**

Chaque inode est décrit par un log des changements appliqués au fichier. En rejouant le log, on reconstruit l’état du fichier.

## Question 32

**Pour quel support LTFS est-il conçu ?**

LTFS est conçu pour les bandes magnétiques.

## Question 33

**Pourquoi LTFS utilise-t-il un index XML ?**

Pour décrire de façon portable la structure des fichiers, répertoires et métadonnées sur la bande.

## Question 34

**Pourquoi il n’existe pas de système de fichiers universel ?**

Parce que chaque support a des contraintes différentes : HDD, SSD, bande, mémoire persistante, optique. Les besoins de performance, fiabilité et sécurité varient aussi.

---

# 17. Résumé final à retenir

Un système de fichiers transforme un support brut en une organisation logique de fichiers, répertoires et métadonnées.

Les concepts essentiels sont :

- Organisation en blocs.
- Allocation et libération d’espace.
- Gestion des noms et répertoires.
- Métadonnées.
- Sécurité.
- Intégrité.
- Snapshots.
- Quotas.
- Verrouillage.
- Déduplication.

Les supports de stockage imposent des contraintes très différentes :

- HDD : mécanique, sensible à la localité.
- SSD : rapide, mais contraintes d’effacement et d’usure.
- Bandes : séquentielles, excellentes pour l’archivage.
- Mémoire persistante : très rapide, adressable par octet.

Linux VFS permet d’unifier l’accès aux différents systèmes de fichiers.

FUSE permet de développer des systèmes de fichiers en espace utilisateur.

Les systèmes étudiés illustrent différentes philosophies :

- FAT : simplicité et portabilité.
- FFS : localité et performance HDD.
- LFS : écritures séquentielles.
- Ext4 : système Linux moderne et généraliste.
- ZFS : intégrité, snapshots et gestion de pool.
- NOVA : mémoire persistante.
- LTFS : bandes magnétiques et archivage portable.

Phrase clé à retenir :

> Le design d’un système de fichiers dépend toujours du support physique, de la sémantique d’accès, des besoins de performance, de fiabilité et de sécurité.
