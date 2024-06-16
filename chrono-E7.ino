/*
Chronomètre semi-automatique pour concours Electro-7
Version 1.0 juin 2024
jacques.leaute@gmail.com

Réglages
Ligne 17 : précision du chronomètrage
Ligne 39 : seuil de détection de la mise en marche moteur
ligne 52 : mesure du niveau de charge de la batterie
*/

#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // Broches LCD (RS, E, D4, D5, D6 et D7)

const int buttonPin = 8;  // Pin du bouton poussoir
int lastButtonState = HIGH; // Dernier état connu du bouton
int buttonState = HIGH;    // État actuel du bouton

float precision = 0.9992; // Réglage de la précision des 2 chronomètres (+ pour accélérer, - pour ralentir)

// Variables du chronometre Tps de vol
bool runningTv = false;      // Variable pour indiquer si le chronomètre Tps de vol tourne
bool stopped = false;      // Variable pour indiquer si le chronomètre Tps de vol a été arrêté
unsigned long startTime;   // Variable pour stocker le temps de début
unsigned long elapsedTime = 0;  // Variable pour stocker le temps écoulé
int minutesTv = 0;
int secondsTv = 0;

// Variables du chronometre Tps moteur
unsigned long previousMillis = 0;
unsigned long elapsedMillis = 0;
//bool runningTm = false;
int minutesTm = 0;
int secondsTm = 0;

// Variables pour le signal PPM du rx
const int ppmRx = 10; // Broche RX ou le signal PPM est connecte
unsigned int impulsInit; // Duree impulsion initiale
unsigned int impulsCourant; // Duree impulsion courante
const unsigned int seuil = 70; // Seuil de detection de mise en marche du moteur en ms
int ecart; // Difference entre duree initiale et duree courante
bool moteurOn = false;
bool signalErr = false;

// Variables pour le buzzer
const int buzzerPin = 9; // Pin où le buzzer est connecté
unsigned long previousMillisBuzzer = 0; // Variable pour stocker la dernière fois où le buzzer a changé d'état
const long intervalOn = 70; // Intervalle pendant lequel le buzzer sonne (en millisecondes)
const long intervalOff = 2000; // Intervalle pendant lequel le buzzer est éteint (en millisecondes)
bool buzzerState = false; // État actuel du buzzer

// Variables pour la tension batterie
const float tensionMax = 4.1; // Tension maxi batterie chargée (100%)
const float tensionMin = 3.0; // Tension mini batterie déchargée(0%)

void setup() {
  lcd.begin(16, 2); // Initialise l'écran LCD avec 16 colonnes et 2 lignes

  int valeurBrute = analogRead(A0); // Acquisition de la tension batterie sur la borne A0. Renvoie une valeur de 0 (0V) à 1023 (5V)
  float tensionBatterie = valeurBrute * 5.0 / 1023; // conversion en Volt
  byte pourcentBat = ((tensionBatterie - tensionMin) / (tensionMax - tensionMin)) * 100; // Calcul du pourcentage de charge
  pourcentBat = constrain(pourcentBat, 0, 100); // Limitation de la valeur entre 0 et 100%

  lcd.print("CHRONO-E7   V1.0"); // Ecran d'accueil
  lcd.setCursor(0, 1);
  lcd.print("Charge bat: ");
  lcd.print(pourcentBat);
  lcd.print("%");


  delay(1500); // Ecran d'accueil pendant 1,5 seconde
  lcd.setCursor(0, 0);
  lcd.print("TPS DE VOL 00:00");
  lcd.setCursor(0, 1);
  lcd.print("MOTEUR --- 00:00");
  pinMode(buttonPin, INPUT_PULLUP);  // Configure la broche du bouton comme entrée avec résistance pull-up interne

  // Initialisation de la broche ppmPin comme entrée PPM
  pinMode(ppmRx, INPUT);
  
  // Mesure de l'impulsion initiale
  impulsInit = pulseIn(ppmRx, HIGH);

  // Définit le pin du buzzer comme sortie
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  // Acquisition du signal Rx
  impulsCourant = pulseIn(ppmRx, HIGH); // Mesure de la largeur de l'impulsion actuelle
  ecart = impulsCourant - impulsInit; // Calcul de l'ecart avec la duree initiale
  // Comparaison avec la largeur de l'impulsion initiale
  if (impulsCourant > 700 && impulsCourant < 2300) { // verifie si le signal PPM est valide
    signalErr = false;
    if (abs(ecart) > seuil) { moteurOn = true; }
    else { moteurOn = false; }
  }
  else { signalErr = true; }

  // Chronometre du Tps de vol
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {  // Si le bouton vient d'être appuyé (transition HIGH à LOW)
    if (!runningTv && !stopped) {  // Si le chronomètre n'a pas démarré et n'a jamais été arrêté
      runningTv = true;
      startTime = millis() * precision;  // Enregistre le temps de début
    } else if (runningTv) {
      runningTv = false;  // Arrête le chronomètre
      stopped = true;   // Marque le chronomètre comme arrêté
    }
    delay(50);  // Délai pour éviter les rebonds du bouton
  }
  lastButtonState = buttonState;  // Mise à jour de l'état du bouton

  if (runningTv) {
    elapsedTime = (millis() * precision) - startTime;  // Calcule le temps écoulé (réglage de la précision du chrono temps de vol possible)
    minutesTv = (elapsedTime / 60000) % 60; // initialement 60000 (+ pour ralentir, - pour accelerer)
    secondsTv = (elapsedTime / 1000) % 60; // initialement 1000 (+ pour ralentir, - pour accelerer)
  }

  // Chronometre du moteur
  if (runningTv && moteurOn) {
    unsigned long currentMillis = millis() * precision;
    elapsedMillis += currentMillis - previousMillis;
    previousMillis = currentMillis;

    minutesTm = (elapsedMillis / 60000) % 60; // Calcule le temps écoulé (réglage de la précision du chrono moteur possible)
    secondsTm = (elapsedMillis / 1000) % 60; // initialement 60000 et 1000 (+ pour ralentir, - pour accelerer)

  // Afficher le chrono moteur sur l'ecran LCD
    lcd.setCursor(11, 1);
    lcd.print(minutesTm < 10 ? "0" : "");  // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTm);
    lcd.print(":");
    lcd.print(secondsTm < 10 ? "0" : "");  // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondsTm);

  } else {
    previousMillis = millis() * precision;
  }

  // Afficher le chrono Tps de vol sur l'ecran LCD
  if (runningTv) { // Chronometre Tps de vol
    lcd.setCursor(11, 0);
    lcd.print(minutesTv < 10 ? "0" : "");  // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(minutesTv);
    lcd.print(":");
    lcd.print(secondsTv < 10 ? "0" : "");  // Ajoute un zéro devant les chiffres inférieurs à 10
    lcd.print(secondsTv);
  }
  // Afficher le signal venant du récepteur. Faire sonner le buzzer
  unsigned long currentMillisBuzzer = millis(); // Récupère le temps écoulé depuis le démarrage du programme
  if (signalErr) { 
    lcd.setCursor(7,1); lcd.print("Err"); // Pas de reception du signal
    digitalWrite(buzzerPin, HIGH);
  } 
  else if (moteurOn) {
    lcd.setCursor(7,1); lcd.print("ON "); // Moteur en fonctionnement
    if (buzzerState) {
      // Si le buzzer est actuellement allumé
      if (currentMillisBuzzer - previousMillisBuzzer >= intervalOn) {
        // Si l'intervalle de temps pour le buzzer allumé est écoulé
        buzzerState = false; // Change l'état du buzzer à éteint
        previousMillisBuzzer = currentMillisBuzzer; // Met à jour le temps précédent
        digitalWrite(buzzerPin, LOW); // Éteint le buzzer
      }
    } else {
      // Si le buzzer est actuellement éteint
      if (currentMillisBuzzer - previousMillisBuzzer >= intervalOff) {
        // Si l'intervalle de temps pour le buzzer éteint est écoulé
        buzzerState = true; // Change l'état du buzzer à allumé
        previousMillisBuzzer = currentMillisBuzzer; // Met à jour le temps précédent
        digitalWrite(buzzerPin, HIGH); // Allume le buzzer
      }
    }
  }
  else { lcd.setCursor(7,1); lcd.print("OFF"); } // Moteur à l'arret
  
}
