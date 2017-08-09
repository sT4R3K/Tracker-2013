# Tracker: Application de suivi de véhicules dans un réseau de capteurs véhiculaires (2013)

## Usage

- simulation.cc à mettre dans ns-3***/scratch/  (<- remplacer par votre chemin vers le dossier)
- avant de mettre le dossier tracker dans  ns-3***/src/
  créer un nouveau module nomé tracker à l'aide de create_model.py et remplacer le dossier
- pour lancer une simulation:
 ~# ./waf --run scratch/simulation
 ceci lancera une simulation avec des noeuds fixes, pour utiliser un modele de mobilité, il nous faut un fichier mobility.tcl crée par SUMO
 et par le moyen de cette commande:
 ~# ./waf --run "scratch/imulation --traceFile=/.../mobility.tcl --logFile=/.../sim.log"
 ( ^ remplacer par le chemin vers le fichier mobility.tcl, sim.log est un fichier de sortie log, crée par le scripte de simulation)

- pour changer le nombre de noeuds:
	nodeNum -> ligne 48 -> simulation.cc

- pour changer le temps de simulation:
	totalTime -> ligne 50 -> simulation.cc

- pour changer le node cible ou le node initiateur principal:
	la methode GetTypeId -> tracker.cc -> (doosier tracker/model)

- pour changer maxdur ou maxdst:
	le constructeur Tracker ->  tracker.cc
.

