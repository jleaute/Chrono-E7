/*
Chronomètre semi-automatique pour concours Electro-7
Version 1.2 octobre 2024
jacques.leaute@gmail.com

Réglages
Ligne 35 :      précision du chronomètrage
Ligne 41 :      seuil de détection de la mise en marche moteur
lignes 54-55 :  niveau de charge de la batterie
ligne 56 :      précision de la tension batterie
*/

#include <LiquidCrystal.h>                              // Appel de la bibliothèque pour gérer l'écran LCD
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);                    // Broches LCD (RS, E, D4, D5, D6 et D7)

// Variables bouton poussoir
const int brocheBp = 8;                                 // Broche Arduino du bouton poussoir
int dernierEtatBp = HIGH;                               // Dernier état connu du bouton
int etatBp = HIGH;                                      // État actuel du bouton

// Variables temps de vol
bool marcheTv = false;                                  // Variable pour indiquer si le chronomètre Tps de vol tourne
bool arretTv = false;                                   // Variable pour indiquer si le chronomètre Tps de vol a été arrêté
unsigned long departTv;                                 // Variable pour stocker le temps de début
unsigned long dureeTv = 0;                              // Variable pour stocker le temps de vol écoulé (corrigé)
int minutesTv = 0;
int secondesTv = 0;

// Variables temps moteur
unsigned long precedDepartTm = 0;
unsigned long dureeTmBrutte;                            // Variable pour stocker le temps moteur écoulé (non corrigé)
unsigned long dureeTm;                                  // Variable pour stocker le temps moteur écoulé (corrigé)
int minutesTm = 0;
int secondesTm = 0;
const float correctionTps = 1.0000000;                  // Réglage de la précision des chronos vol et moteur (+ pour accélérer, - pour ralentir)

// Variables signal PPM
const int ppmRx = 10;                                   // Broche Arduino où le signal PPM du RX est connecte
unsigned int impulsInit;                                // Duree impulsion initiale
unsigned int impulsCourant;                             // Duree impulsion courante
const unsigned int seuil = 70;                          // Seuil de detection de mise en marche du moteur (en microsecondes)
int ecart;                                              // Difference entre durée initiale et durée courante
bool moteurOn = false;
bool signalErr = false;

// Variables buzzer
const int brocheBuzzer = 9;                             // Broche où le buzzer est connecté
unsigned long precedDepartTmBuzzer = 0;                 // Variable pour stocker la dernière fois où le buzzer a changé d'état
const long intervalOn = 70;                             // Intervalle pendant lequel le buzzer sonne (en millisecondes)
const long intervalOff = 2000;                          // Intervalle pendant lequel le buzzer est éteint (en millisecondes)
bool etatBuzzer = false;                                // État actuel du buzzer

// Variables batterie
const float tensionMax = 4.04;                          // Tension maxi batterie chargée (100%)
const float tensionMin = 3.54;                          // Tension mini batterie déchargée(0%)
const float vRef = 5.00;                                // Tension de référence de l'Arduino
int analogIn = A0;                                      // Assigne la variable de type integer «analogIn» à la broche A0
long somme = 0;                                         // Variable pour cumuler les mesures de tension batterie

// Variable mode réglages
bool modeReglages = false;				// Mode réglages. Variable de mémorisation de l'entrée en mode réglages

void setup() {
  lcd.begin(16, 2);                                     // Initialiser l'écran LCD avec 16 colonnes et 2 lignes
  pinMode(brocheBp, INPUT_PULLUP);                      // Configure la broche du bouton comme entrée avec résistance pull-up interne
  etatBp = digitalRead(brocheBp);
  if (etatBp == LOW) {					// Mode réglages. Si bouton appuyé lors de la mise sous tension, alors...
    modeReglages = true;				// Mode réglages. Mémorisation du passage en mode réglages
    lcd.print(" J.LEAUTE 2024");			// Mode réglages. Splash screen
    lcd.setCursor(0, 1);
    lcd.print(" MODE REGLAGES");
    delay(1500);					// Mode réglages. Affichage pendant 1,5 seconde
  }
  else {						// Si le mode réglages n'est pas sélectionné, alors..
  lcd.print("CHRONO-E7   V1.2");                        // Splash screen
  lcd.setCursor(0, 1);
  lcd.print("Charge bat: ");
  delay(50);                                            // Attendre la stabilisation du module step-up 3.7V-5V
  pinMode(analogIn,INPUT);                              // Initialise le mode INPUT à la broche d'entrée analogique analogIn (A0)
  for (byte i = 0; i < 200; i++) {                      // Réaliser 200 mesures de tension ...
    int valeurBrute = analogRead(analogIn);
    somme = somme + valeurBrute;                        // ... et les additionner
  }
  float tensionBatterie = somme / 200 * vRef / 1024;    // Calculer la tension moyenne de la batterie
  int pourcentBat = ((tensionBatterie - tensionMin) / (tensionMax - tensionMin)) * 100;   // Calcul du pourcentage de charge
  pourcentBat = constrain(pourcentBat, 0, 100);         // Limitation de la valeur entre 0 et 100%    
  lcd.print(pourcentBat);
  lcd.print("%");
  delay(1500);                                          // Afficher l'écran d'accueil pendant 1,5 seconde

  lcd.setCursor(0, 0);                                  // Ecran de départ
  lcd.print("TPS DE VOL 00:00");
  lcd.setCursor(0, 1);
  lcd.print("MOTEUR --- 00:00");

  pinMode(ppmRx, INPUT);                                // Initialisation de la broche n°10 comme entrée
  impulsInit = pulseIn(ppmRx, HIGH);                    // Mesure de l'impulsion initiale

  pinMode(brocheBuzzer, OUTPUT);                        // Définit le pin du buzzer comme sortie
  }
}

void loop() {
  if (modeReglages == true) {				// Mode réglages. Si le mode réglages est sélectionné, alors...
    impulsCourant = pulseIn(ppmRx, HIGH);		// Mode réglages. Mesure de la largeur de l'impulsion
    pinMode(analogIn,INPUT);				// Mode réglages. Initialise le mode INPUT à la broche d'entrée analogique analogIn (A0)
    for (byte i = 0; i < 200; i++) {			// Mode réglages. Réaliser 200 mesures de tension ...
      int valeurBrute = analogRead(analogIn);
      somme = somme + valeurBrute;			// Mode réglages. ... et les additionner
    }
    float tensionBatterie = somme / 200 * vRef / 1024;	// Mode réglages. Calculer la tension moyenne de la batterie
    lcd.setCursor(0, 0);
    lcd.print("SIGNAL RX ");
    lcd.print(impulsCourant);				// Mode réglages. Afficher le signal PPM
    lcd.print("ms ");
    lcd.setCursor(0, 1);
    lcd.print("BATTERIE: ");
    lcd.print(tensionBatterie);				// Mode réglages. Afficher la tension de l'accu
    lcd.print("V ");
    somme = 0;
  }
  else {						// Si le mode réglages n'est pas sélectionné, alors..
  // Acquisition du signal PPM
  impulsCourant = pulseIn(ppmRx, HIGH);                 // Mesure de la largeur de l'impulsion actuelle
  ecart = impulsCourant - impulsInit;                   // Calcul de l'écart avec la durée initiale
  if (impulsCourant > 700 && impulsCourant < 2300) {    // verifie si le signal PPM est valide
    signalErr = false;
    if (abs(ecart) > seuil) { moteurOn = true; }        // Comparaison avec la largeur de l'impulsion initiale
    else { moteurOn = false; }
  }
  else { signalErr = true; }

  // Chronometre du temps de vol
  etatBp = digitalRead(brocheBp);
  if (etatBp == LOW && dernierEtatBp == HIGH) {         // Si le bouton vient d'être appuyé (transition HIGH à LOW)
    if (!marcheTv && !arretTv) {                        // Si le chronomètre n'a pas démarré et n'a jamais été arrêté
      marcheTv = true;
      departTv = millis();                              // Enregistre le temps de début
    } else if (marcheTv) {
      marcheTv = false;                                 // Arrête le chronomètre
      arretTv = true;                                   // Marque le chronomètre comme arrêté
    }
    delay(50);                                          // Délai pour éviter les rebonds du contact du bouton poussoir
  }
  dernierEtatBp = etatBp;                               // Mise à jour de l'état du bouton

  if (marcheTv) {
    dureeTv = (millis() - departTv) * correctionTps;    // Calcule le temps écoulé
    minutesTv = (dureeTv / 60000) % 60;                 // Converti les ms en minutes et secondes
    secondesTv = (dureeTv / 1000) % 60;
  }
  // Afficher le chrono Tps de vol sur l'ecran LCD
  if (marcheTv) {                                       // Chronomètre Tps de vol
    lcd.setCursor(11, 0);
    lcd.print(minutesTv < 10 ? "0" : "");               // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTv);
    lcd.print(":");
    lcd.print(secondesTv < 10 ? "0" : "");              // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondesTv);
  }
  // Chronometre temps moteur
  if (marcheTv && moteurOn) {
    dureeTmBrutte += millis() - precedDepartTm;         // Calcule le temps moteur écoulé
    precedDepartTm = millis();
    dureeTm = dureeTmBrutte * correctionTps;            // Correction du temps moteur écoulé
    minutesTm = (dureeTm / 60000) % 60;                 // Converti les ms en minutes et secondes
    secondesTm = (dureeTm / 1000) % 60;

  // Afficher le chrono moteur sur l'ecran LCD
    lcd.setCursor(11, 1);
    lcd.print(minutesTm < 10 ? "0" : "");               // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTm);
    lcd.print(":");
    lcd.print(secondesTm < 10 ? "0" : "");              // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondesTm);

  } else {
    precedDepartTm = millis();
  }
  // Afficher le signal venant du récepteur. Faire sonner le buzzer
  unsigned long actuelMillisBuzzer = millis();          // Récupère le temps écoulé depuis le démarrage du programme
  if (signalErr) { 
    lcd.setCursor(7,1); lcd.print("Err");               // Pas de reception du signal
    digitalWrite(brocheBuzzer, HIGH);                   // Faire sonner le buzzer
  } 
  else if (moteurOn) {
    lcd.setCursor(7,1); lcd.print("ON ");               // Moteur en fonctionnement
    if (etatBuzzer) {
      // Si le buzzer est actuellement allumé
      if (actuelMillisBuzzer - precedDepartTmBuzzer >= intervalOn) {
        // Si l'intervalle de temps pour le buzzer allumé est écoulé
        etatBuzzer = false;                             // Change l'état du buzzer à éteint
        precedDepartTmBuzzer = actuelMillisBuzzer;      // Met à jour le temps précédent
        digitalWrite(brocheBuzzer, LOW);                // Éteint le buzzer
      }
    } else {
      // Si le buzzer est actuellement éteint
      if (actuelMillisBuzzer - precedDepartTmBuzzer >= intervalOff) {
        // Si l'intervalle de temps pour le buzzer éteint est écoulé
        etatBuzzer = true;                              // Change l'état du buzzer à allumé
        precedDepartTmBuzzer = actuelMillisBuzzer;      // Met à jour le temps précédent
        digitalWrite(brocheBuzzer, HIGH);               // Allume le buzzer
      }
    }
  }
  else { lcd.setCursor(7,1); lcd.print("OFF"); digitalWrite(brocheBuzzer, LOW); } // Moteur à l'arret, buzzer éteint
  }
}
