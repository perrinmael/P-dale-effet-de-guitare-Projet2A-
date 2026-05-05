# Pédale multi-effets pour guitare - STM32F746G

## Description du projet

Ce projet consiste en la réalisation d’une **pédale multi-effets numérique pour guitare** basée sur la carte **STM32F746G Discovery**.

L’objectif est de traiter un signal audio en temps réel et d’y appliquer différents effets audio, tout en permettant à l’utilisateur de contrôler l’interface via un écran tactile LCD.

Le système fonctionne en temps réel avec une chaîne complète :
**Entrée audio → traitement DSP (STM32) → sortie audio**

---

## Architecture du projet

Le projet est divisé en deux grandes parties :

### Interface utilisateur (UI)
- Écran tactile LCD
- Sélection des effets via des boutons
- Contrôle des paramètres via des sliders

### Traitement audio (DSP)
- Traitement en temps réel sur STM32
- Utilisation de buffers audio et SDRAM
- Application d’effets numériques selon l’effet sélectionné

---

## Effets disponibles

### Bypass
Aucun traitement appliqué, le signal est directement transmis.

---

### Delay (Écho)
Effet de répétition du signal.

Paramètres :
- **Delay (ms)** : temps entre les répétitions
- **Feedback** 
- **Mix** 

---

### Chorus
Effet d’épaississement du son par modulation de délai.

Paramètres :
- **Depth** : profondeur de modulation
- **Rate** 
- **Mix** 

---

### Tremolo
Variation périodique du volume du signal.

Paramètres :
- **Depth** : intensité de la modulation
- **Rate** : vitesse de variation
- **Mix** 

---

## Fonctionnement du code

### UI (`ui.c`)
- Gestion de l’écran tactile
- Détection des boutons d’effets
- Gestion des sliders
- Mapping des sliders vers les paramètres audio

### DSP (`effects.c`)
- Traitement audio en temps réel
- Implémentation des effets :
  - Delay (buffer circulaire en SDRAM)
  - Chorus (LFO + delay modulé)
  - Tremolo (modulation d’amplitude)
- Fonction principale : `processAudioEffect()`

---

## Flux du signal

1. Capture audio (entrée ADC / codec)
2. Stockage dans buffer
3. Application de l’effet sélectionné
4. Lecture du signal traité
5. Sortie audio

---

## Démo

Une vidéo de démonstration du projet est disponible ici :  
https://youtube.com/shorts/hceOGY2BNg8
