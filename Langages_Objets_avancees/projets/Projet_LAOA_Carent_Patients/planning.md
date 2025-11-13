# Planning projet Qt – Carnet patients générique (21 jours, 1h/jour)

## Objectif général

Application Qt « Carnet patients générique » avec :
- Modèle Patients → Visites
- Champs validés
- Recherche multi-critères
- Historique en QTreeView

---

## Semaine 1 – Modèle & conception (Jours 1 à 7)

### Jour 1 – Clarifier le besoin fonctionnel
- Définir ce qu’est un **patient** (nom, prénom, date de naissance, ID, téléphone, etc.).
- Définir ce qu’est une **visite** (date, motif, note, médecin, etc.).
- Lister les actions possibles :
  - Créer / modifier / supprimer un patient.
  - Ajouter / modifier / supprimer une visite.
  - Voir l’historique des visites d’un patient dans un QTreeView.
  - Rechercher par critères (nom, date, maladie, etc.).
- Noter tout dans un petit fichier texte ou `.md`.

### Jour 2 – Concevoir le modèle (UML simple)
- Dessiner un mini diagramme de classes :
  - `Patient`
  - `Visit`
  - `PatientModel` (ou usage d’un `QStandardItemModel` au début).
- Décider de la structure du QTreeView :
  - Racine : **liste de patients**.
  - Enfants de chaque patient : **ses visites**.

### Jour 3 – Création du projet Qt
- Créer un projet C++ Qt (application Widgets).
- Ajouter les fichiers :
  - `patient.h/.cpp`
  - `visit.h/.cpp`
- Implémenter des classes simples (sans Qt pour l’instant) :
  - Attributs, constructeurs, getters/setters.

### Jour 4 – Intégration Qt dans le modèle
- Décider si `Patient` (et/ou `Visit`) doit hériter de `QObject` (optionnel).
- Ajouter dans `Patient` :
  - Un identifiant unique (int ou `QString`).
  - Une collection de visites (ex. `QList<Visit>`).
- Ajouter quelques méthodes utilitaires (`addVisit`, `removeVisit`, etc.).

### Jour 5 – Esquisser `PatientModel`
- Créer `patientmodel.h/.cpp`.
- Choisir la stratégie :
  - Soit un modèle custom (`QAbstractItemModel`).
  - Soit un `QStandardItemModel` pour démarrer plus simple.
- Définir le rôle des lignes :
  - Lignes « parent » : patients.
  - Lignes « enfant » : visites.
- Définir les colonnes (ex. nom, prénom, date de naissance / date visite, motif).

### Jour 6 – Structure de base du modèle
- Implémenter les méthodes minimales :
  - `rowCount()`
  - `columnCount()`
  - `index()`
  - `parent()`
- Remplir le modèle avec 2–3 patients de test « en dur » pour commencer.

### Jour 7 – Implémenter `data()` (affichage)
- Implémenter `QVariant PatientModel::data(const QModelIndex &, int role)` :
  - Gérer au minimum `Qt::DisplayRole`.
- Vérifier en debug que les valeurs retournées sont cohérentes.

---

## Semaine 2 – Interface graphique & CRUD (Jours 8 à 14)

### Jour 8 – MainWindow & QTreeView
- Créer une `MainWindow` avec Qt Designer.
- Layout proposé :
  - À gauche : `QTreeView` (patients → visites).
  - À droite : un panneau de détails pour le patient sélectionné.
- Dans le code :
  - Instancier `PatientModel`.
  - `ui->treeView->setModel(&patientModel);`

### Jour 9 – Sélection & affichage des détails
- Connecter la sélection du `QTreeView` :
  - `QItemSelectionModel::currentChanged`.
- Quand un **patient** est sélectionné :
  - Afficher ses détails dans des widgets (`QLineEdit`, `QDateEdit`, etc.).
- Pour une **visite**, soit ignorer pour l’instant, soit afficher une info minimale.

### Jour 10 – Édition d’un patient
- Ajouter des champs pour :
  - Nom, prénom, date de naissance, téléphone, etc.
- Ajouter un bouton « Enregistrer / Appliquer ».
- Lors du clic :
  - Mettre à jour l’objet `Patient` via le modèle.
  - Émettre les signaux nécessaires (`dataChanged`, etc.).

### Jour 11 – Création d’un patient
- Ajouter un bouton « Nouveau patient ».
- Lors du clic :
  - Créer un `Patient` avec quelques valeurs par défaut.
  - L’ajouter au modèle.
  - Le sélectionner automatiquement dans le `QTreeView`.
  - Afficher ses détails à droite.

### Jour 12 – Suppression d’un patient
- Ajouter un bouton « Supprimer patient ».
- Lors du clic :
  - Récupérer l’index sélectionné.
  - Supprimer le patient du modèle.
  - Rafraîchir la vue si nécessaire (`layoutChanged`, etc.).

### Jour 13 – Gestion des visites
- Dans le panneau de droite, ajouter :
  - Une petite liste ou zone pour les visites du patient.
  - Boutons « Ajouter visite » et « Supprimer visite ».
- Lors de « Ajouter visite » :
  - Créer une visite (date du jour + motif vide par exemple).
  - L’ajouter comme enfant du patient dans le modèle/QTreeView.

### Jour 14 – Historique en QTreeView
- Vérifier que :
  - Chaque patient a bien ses visites en enfants dans le `QTreeView`.
- Ajuster l’implémentation de `index() / parent() / rowCount()` si besoin.
- Tester avec plusieurs patients et visites.

---

## Semaine 3 – Recherche, validation, persistance & polish (Jours 15 à 21)

### Jour 15 – Mise en place de la recherche multi-critères
- Ajouter une zone de recherche (en haut de la fenêtre) :
  - Champ texte (nom/prénom).
  - Éventuels champs pour date ou maladie.
- Choisir la technique :
  - `QSortFilterProxyModel` avec critères.
  - Ou filtrage manuel dans le modèle.

### Jour 16 – Implémentation de la recherche
- Implémenter d’abord une recherche simple :
  - Filtre sur nom/prénom (contient le texte saisi).
- Ajouter un deuxième critère ensuite (ex. maladie ou date de visite).
- Faire en sorte que le `QTreeView` ne montre que les patients/visites correspondant aux critères.

### Jour 17 – Validation des champs
- Utiliser des validateurs :
  - `QValidator` / `QRegularExpressionValidator` pour téléphone, etc.
- Contraintes possibles :
  - Nom/prénom non vides.
  - Date de naissance < date du jour.
- Bloquer l’enregistrement si les champs obligatoires sont invalides :
  - Afficher un message (boîte de dialogue ou barre d’état).

### Jour 18 – Sauvegarde des données (JSON ou XML)
- Choisir un format simple, par exemple **JSON** :
  - Tableau de patients.
  - Chaque patient contient un tableau de visites.
- Ajouter un menu « Fichier → Enregistrer… » :
  - Utiliser `QJsonDocument`, `QJsonObject`, `QJsonArray` (ou XML).
  - Sérialiser patients + visites dans un fichier.

### Jour 19 – Chargement des données
- Ajouter un menu « Fichier → Ouvrir… » :
  - Charger le fichier JSON.
  - Reconstruire la liste des patients et leurs visites dans le modèle.
- Vérifier que :
  - Le QTreeView reflète bien le contenu du fichier.
  - L’historique des visites est correct.

### Jour 20 – Nettoyage & finition de l’interface
- Améliorer l’UI :
  - Icône de l’application.
  - Labels clairs, dispositions propres.
- Ajouter quelques messages utilisateur :
  - Dans la barre d’état : nombre de patients chargés, résultat de la recherche, etc.
- Vérifier la cohérence des actions (ajout/suppression/modification).

### Jour 21 – Tests finaux & mini-documentation
- Tester tout le flux :
  - CRUD patients.
  - CRUD visites.
  - Historique dans QTreeView.
  - Recherche multi-critères.
  - Validation.
  - Sauvegarde/chargement.
- Mettre à jour un petit fichier `.md` :
  - Diagramme de classes (texte ou image référencée).
  - Liste des fonctionnalités implémentées.
  - Quelques notes sur les choix techniques (modèle, format de fichier, etc.).

---
