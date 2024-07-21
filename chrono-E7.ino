/*
Chronomètre semi-automatique pour concours Electro-7
Version 1.0 juillet 2024
jacques.leaute@gmail.com

Réglages
Ligne 20 :      précision du chronomètrage
Ligne 40 :      seuil de détection de la mise en marche moteur
lignes 53-54 :  niveau de charge de la batterie
ligne 63 :      précision de la tension batterie
*/

#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);                    // Broches LCD (RS, E, D4, D5, D6 et D7)

const int brocheBp = 8;                                 // Broche Arduino du bouton poussoir
int dernierEtatBp = HIGH;                               // Dernier état connu du bouton
int etatBp = HIGH;                                      // État actuel du bouton

float precision = 1.000000;                             // Réglage de la précision des 2 chronomètres (+ pour accélérer, - pour ralentir)

// Variables du chronometre Tps de vol
bool marcheTv = false;                                  // Variable pour indiquer si le chronomètre Tps de vol tourne
bool arretTv = false;                                   // Variable pour indiquer si le chronomètre Tps de vol a été arrêté
unsigned long departTps;                                // Variable pour stocker le temps de début
unsigned long duree = 0;                                // Variable pour stocker le temps écoulé
int minutesTv = 0;
int secondesTv = 0;

// Variables du chronometre Tps moteur
unsigned long precedMillis = 0;
unsigned long dureeMillis = 0;
int minutesTm = 0;
int secondesTm = 0;

// Variables pour le signal PPM du rx
const int ppmRx = 10;                                   // Broche Arduino ou le signal PPM du RX est connecte
unsigned int impulsInit;                                // Duree impulsion initiale
unsigned int impulsCourant;                             // Duree impulsion courante
const unsigned int seuil = 70;                          // Seuil de detection de mise en marche du moteur (en microsecondes)
int ecart;                                              // Difference entre duree initiale et duree courante
bool moteurOn = false;
bool signalErr = false;

// Variables pour le buzzer
const int brocheBuzzer = 9;                             // Broche où le buzzer est connecté
unsigned long precedMillisBuzzer = 0;                   // Variable pour stocker la dernière fois où le buzzer a changé d'état
const long intervalOn = 70;                             // Intervalle pendant lequel le buzzer sonne (en millisecondes)
const long intervalOff = 2000;                          // Intervalle pendant lequel le buzzer est éteint (en millisecondes)
bool etatBuzzer = false;                                // État actuel du buzzer

// Variables pour la tension batterie
const float tensionMax = 4.03;                          // Tension maxi batterie chargée (100%)
const float tensionMin = 3.30;                          // Tension mini batterie déchargée(0%)

void setup() {
  lcd.begin(16, 2);                                     // Initialise l'écran LCD avec 16 colonnes et 2 lignes

  unsigned long Somme = 0 ;                             // variable pour additionner une série de mesures de tension
  float tensionBatterie ;                               // Variable tension moyenne de la batterie
  for(int i = 0 ; i < 100 ; i++) {                      // Boucle de 100 mesues
  Somme = Somme + analogRead(A0) ;                      // Acquisition de la tension batterie sur la borne A0. Renvoie une valeur de 0 (0V) à 1023 (5.13V)
  }
  tensionBatterie = Somme / 100 * 5.13 / 1024 ;         // Moyenne des 100 mesures effectuées. VREF de la carte Arduino = 5.13V

  byte pourcentBat = ((tensionBatterie - tensionMin) / (tensionMax - tensionMin)) * 100;   // Calcul du pourcentage de charge
  pourcentBat = constrain(pourcentBat, 0, 100);         // Limitation de la valeur entre 0 et 100%

  lcd.print("CHRONO-E7   V1.0");                        // Ecran d'accueil
  lcd.setCursor(0, 1);
  lcd.print("Charge bat: ");
  lcd.print(pourcentBat);
  lcd.print("%");
  delay(1500);                                          // Ecran d'accueil pendant 1,5 seconde

  lcd.setCursor(0, 0);
  lcd.print("TPS DE VOL 00:00");
  lcd.setCursor(0, 1);
  lcd.print("MOTEUR --- 00:00");
  pinMode(brocheBp, INPUT_PULLUP);                      // Configure la broche du bouton comme entrée avec résistance pull-up interne

  // Initialisation de la broche ppmPin comme entrée PPM
  pinMode(ppmRx, INPUT);
  
  // Mesure de l'impulsion initiale
  impulsInit = pulseIn(ppmRx, HIGH);

  // Définit le pin du buzzer comme sortie
  pinMode(brocheBuzzer, OUTPUT);
}

void loop() {
  // Acquisition du signal Rx
  impulsCourant = pulseIn(ppmRx, HIGH);                 // Mesure de la largeur de l'impulsion actuelle
  ecart = impulsCourant - impulsInit;                   // Calcul de l'ecart avec la duree initiale
  // Comparaison avec la largeur de l'impulsion initiale
  if (impulsCourant > 700 && impulsCourant < 2300) {    // verifie si le signal PPM est valide
    signalErr = false;
    if (abs(ecart) > seuil) { moteurOn = true; }
    else { moteurOn = false; }
  }
  else { signalErr = true; }

  // Chronometre du Tps de vol
  etatBp = digitalRead(brocheBp);
  if (etatBp == LOW && dernierEtatBp == HIGH) {         // Si le bouton vient d'être appuyé (transition HIGH à LOW)
    if (!marcheTv && !arretTv) {                        // Si le chronomètre n'a pas démarré et n'a jamais été arrêté
      marcheTv = true;
      departTps = millis();                             // Enregistre le temps de début
    } else if (marcheTv) {
      marcheTv = false;                                 // Arrête le chronomètre
      arretTv = true;                                   // Marque le chronomètre comme arrêté
    }
    delay(50);                                          // Délai pour éviter les rebonds du contact du bouton poussoir
  }
  dernierEtatBp = etatBp;                               // Mise à jour de l'état du bouton

  if (marcheTv) {
    duree = (millis() * precision) - departTps;         // Calcule le temps écoulé
    minutesTv = (duree / 60000) % 60;
    secondesTv = (duree / 1000) % 60;
  }

  // Chronometre du moteur
  if (marcheTv && moteurOn) {
    unsigned long actuelMillis = millis() * precision;
    dureeMillis += actuelMillis - precedMillis;
    precedMillis = actuelMillis;

    minutesTm = (dureeMillis / 60000) % 60;             // Calcule le temps écoulé
    secondesTm = (dureeMillis / 1000) % 60;

  // Afficher le chrono moteur sur l'ecran LCD
    lcd.setCursor(11, 1);
    lcd.print(minutesTm < 10 ? "0" : "");               // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTm);
    lcd.print(":");
    lcd.print(secondesTm < 10 ? "0" : "");              // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondesTm);

  } else {
    precedMillis = millis() * precision;
  }

  // Afficher le chrono Tps de vol sur l'ecran LCD
  if (marcheTv) {                                       // Chronometre Tps de vol
    lcd.setCursor(11, 0);
    lcd.print(minutesTv < 10 ? "0" : "");               // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTv);
    lcd.print(":");
    lcd.print(secondesTv < 10 ? "0" : "");              // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondesTv);
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
      if (actuelMillisBuzzer - precedMillisBuzzer >= intervalOn) {
        // Si l'intervalle de temps pour le buzzer allumé est écoulé
        etatBuzzer = false;                             // Change l'état du buzzer à éteint
        precedMillisBuzzer = actuelMillisBuzzer;        // Met à jour le temps précédent
        digitalWrite(brocheBuzzer, LOW);                // Éteint le buzzer
      }
    } else {
      // Si le buzzer est actuellement éteint
      if (actuelMillisBuzzer - precedMillisBuzzer >= intervalOff) {
        // Si l'intervalle de temps pour le buzzer éteint est écoulé
        etatBuzzer = true;                              // Change l'état du buzzer à allumé
        precedMillisBuzzer = actuelMillisBuzzer;        // Met à jour le temps précédent
        digitalWrite(brocheBuzzer, HIGH);               // Allume le buzzer
      }
    }
  }
  else { lcd.setCursor(7,1); lcd.print("OFF"); digitalWrite(brocheBuzzer, LOW); } // Moteur à l'arret, buzzer éteint
  
}
