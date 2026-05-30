# Analyse de `kernel/sched/completion.c`

> **Module** : Architecture OS — ARSE · ENSIIE  
> **Fichier étudié** : `kernel/sched/completion.c` (Linux kernel)  
> **Auteur du cours** : Aurélien Cedeyn  
> **Objectif** : comprendre la primitive `completion` du noyau Linux, son fonctionnement interne et ses différences avec les sémaphores.

---

## Table des matières

1. [Qu'est-ce qu'une completion ?](#1-quest-ce-quune-completion-)
2. [Différences avec les sémaphores](#2-différences-avec-les-sémaphores)
3. [Structure `struct completion`](#3-structure-struct-completion)
4. [Analyse du code source](#4-analyse-du-code-source)
5. [Fonctions principales](#5-fonctions-principales)
6. [Résumé](#6-résumé)

---

## 1. Qu'est-ce qu'une completion ?

Une **completion** est une primitive de synchronisation du noyau Linux permettant à un thread d'attendre qu'une activité noyau se termine ou qu'un état précis soit atteint.

Son fonctionnement ressemble à `pthread_barrier()` en espace utilisateur : un thread attend un signal (`done`) avant de continuer.

### Pourquoi une completion plutôt qu'un mutex ou un sémaphore ?

La completion se concentre sur **une tâche unique et précise** avec une stratégie minimaliste qui combine qualité et simplicité. Elle est construite au-dessus de l'infrastructure `waitqueue` et `wakeup` de l'ordonnanceur Linux.

Les threads qui doivent attendre sont mis en sommeil, puis réveillés par un simple signal stocké dans la structure `completion`.

---

## 2. Différences avec les sémaphores

Le code source l'explique directement dans son commentaire d'en-tête :

```c
/*
 * Generic wait-for-completion handler;
 *
 * It differs from semaphores in that their default case is the opposite,
 * wait_for_completion default blocks whereas semaphore default non-block.
 * The interface also makes it easy to 'complete' multiple waiting threads,
 * something which isn't entirely natural for semaphores.
 *
 * But more importantly, the primitive documents the usage. Semaphores would
 * typically be used for exclusion which gives rise to priority inversion.
 * Waiting for completion is a typically sync point, but not an exclusion point.
 */
```

### Tableau comparatif

| Critère | Sémaphore | Completion |
|---|---|---|
| Comportement par défaut | Non bloquant | Bloquant |
| Usage principal | Protéger l'accès à une ressource | Synchroniser des threads |
| Réveil collectif | Non naturel | Oui (`complete_all`) |
| Lisibilité des API | Générique | Explicite (`wait_for_completion`, `complete`) |
| Risque d'inversion de priorité | Oui | Non |

**Sémaphore** : le processus regarde si la ressource est libre, l'utilise, ou attend qu'elle se libère.

**Completion** : le processus s'assoit et attend le signal — il ne contrôle aucune ressource.

---

## 3. Structure `struct completion`

La structure est définie dans `include/linux/completion.h` :

```c
struct completion {
    unsigned int done;          /* compteur d'achèvement */
    struct swait_queue_head wait; /* file d'attente */
};
```

- `done` : compteur indiquant combien de fois `complete()` a été appelé. `UINT_MAX` signifie "permanentemente terminé".
- `wait` : file d'attente des threads endormis en attente du signal.

### Initialisation

```c
/* Statique */
DECLARE_COMPLETION(ma_completion);

/* Dynamique */
struct completion c;
init_completion(&c);
```

---

## 4. Analyse du code source

### Licence

```c
// SPDX-License-Identifier: GPL-2.0
```

La GNU GPL v2 autorise l'utilisation, la modification et la redistribution du code sous réserve de fournir les sources. `SPDX-License-Identifier` est une annotation machine, lue par des outils d'analyse de licences, pas seulement par les développeurs.

### Includes

```c
#include <linux/linkage.h>
#include <linux/sched/debug.h>
#include <linux/completion.h>
#include "sched.h"
```

---

## 5. Fonctions principales

### `complete_with_flags` — cœur interne

```c
static void complete_with_flags(struct completion *x, int wake_flags)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&x->wait.lock, flags);

    if (x->done != UINT_MAX)
        x->done++;                         /* incrémente le compteur */
    swake_up_locked(&x->wait, wake_flags); /* réveille un thread en attente */

    raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

- **`raw_spin_lock_irqsave`** : verrou spinlock avec désactivation des interruptions — protège la section critique même en contexte d'interruption.
- **`x->done != UINT_MAX`** : évite le débordement ; `UINT_MAX` = état "toujours terminé".
- **`swake_up_locked`** : réveille exactement un thread de la file d'attente.

### `complete` — signal à un thread

```c
void complete(struct completion *x)
{
    complete_with_flags(x, 0);
}
```

### `complete_on_current_cpu` — signal sur le CPU courant

```c
void complete_on_current_cpu(struct completion *x)
{
    return complete_with_flags(x, WF_CURRENT_CPU);
}
```

Optimisation NUMA : réveille le thread préférentiellement sur le même CPU pour une meilleure localité cache.

### `complete_all` — réveil de tous les threads en attente

```c
void complete_all(struct completion *x)
{
    unsigned long flags;

    raw_spin_lock_irqsave(&x->wait.lock, flags);
    x->done = UINT_MAX;              /* marque comme "toujours prêt" */
    swake_up_all_locked(&x->wait);   /* réveille tous les waiters */
    raw_spin_unlock_irqrestore(&x->wait.lock, flags);
}
```

### `wait_for_completion` — attente bloquante

```c
void wait_for_completion(struct completion *x)
{
    wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}
```

Le thread est mis dans l'état `TASK_UNINTERRUPTIBLE` : il ne peut pas être réveillé par un signal, seulement par `complete()`.

### `wait_for_completion_interruptible` — attente interruptible

```c
int wait_for_completion_interruptible(struct completion *x)
{
    long t = wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_INTERRUPTIBLE);
    return (t < 0) ? (int)t : 0;
}
```

Retourne `-ERESTARTSYS` si un signal interrompt l'attente.

### `wait_for_completion_timeout` — attente avec délai

```c
unsigned long wait_for_completion_timeout(struct completion *x,
                                          unsigned long timeout)
{
    return wait_for_common(x, timeout, TASK_UNINTERRUPTIBLE);
}
```

---

## 6. Résumé

| Concept | Explication |
|---|---|
| Rôle | Synchroniser des threads sans protéger de ressource |
| Mécanisme | `waitqueue` + compteur `done` |
| `complete()` | Réveille un thread endormi |
| `complete_all()` | Réveille tous les threads endormis, marque comme permanent |
| `wait_for_completion()` | Bloque jusqu'au signal, non interruptible |
| Différence clé vs sémaphore | Bloquant par défaut, réveil collectif, pas d'inversion de priorité |

La completion est l'outil idéal pour les **points de rendez-vous one-shot** entre un initiateur et un ou plusieurs threads qui attendent un événement unique dans le noyau.
