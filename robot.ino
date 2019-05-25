// il programma seguente si riferisce ad una specifica app android: Joystick BT Commander
// questa istruzione aggiunge una libreria che permette di semplificare il codice per la comunicazione da seriale
#include "SoftwareSerial.h"

// di seguito

//--- INIZIALIZAZIONE MOTORI ---//
int motorA1 = 11; // Motore B2
int motorA2 = 10; // Motore B1
int motorB1 = 5; // Motore A1
int motorB2 = 6; // Motore A2
int vel = 255; // Speed Of Motors (0-255)
int state = '0'; // Initialise Motors


//--- INIZIALIZZAZIONE VARIABILI GLOBALI ---//
//SoftwareSerial serialeHC05(7, 8); // questo oggetto rappresenterà la scheda bluetooth HC05, // 8 = RX | 9 = TX


byte lettura[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // la struttura del dati in arrivo è definita dalla app che stiamo utilizzando, nello specifico è una sequenza di 8 byte

#define INIZIO_TRASMISSIONE 0x02   // 6 Bytes  ex: < INIZIO_TRASMISSIONE "200" "180" FINE_TRASMISSIONE >        3 Bytes  ex: < INIZIO_TRASMISSIONE "C" FINE_TRASMISSIONE >
#define FINE_TRASMISSIONE 0x03

boolean arma_uno = false;
boolean arma_due = false;

void setup() // questa funzione viene chiamata all'inizio e solo una volta
{
  Serial.begin(9600); // questo avvia la comunicazione seriale di arduino via cavo, servirà per monitorare il comportamento del robot
  //serialeHC05.begin(9600); // questo avvia la comunicazione sul dispositivo bluetooth per poter ricevere i comandi dall'app

  //SETUP PIN MOTORI COME OUTPUT
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
}

void loop() // arduino chiamerà ciclicamente questa funzione
{

  if (Serial.available()) // la scheda HC ha una comunicazione attiva?
  {
    delay(5); // un piccolo delay per compensare il tempo lento in cui i dati vengono ricevuti rispetto alla velocità dell'hardware di processarli

    lettura[0] = Serial.read();// si è attiva, posso quindi iniziare a leggere i dati ricevuti;

    if (lettura[0] == INIZIO_TRASMISSIONE)
    {
      int i = 1;
      while (Serial.available())
      {
        delay(1);
        lettura[i] = Serial.read();

        // controllo se ci sono possibili errori:
        // 1) il valore letto deve essere inferiore a 127,
        // 2) la sequenza inviata al massimo ha 8 valori quindi non posso avere un indice maggiore di 7                      [0, 1, 2, 3, 4, 5, 6, 7] ->8 valori
        if (lettura[i] > 127 || i > 7)
        {
          // smetto di leggere uscendo dal ciclo while
          break;
        }
        // ho ricevuto il messaggio di fine trasmissione? E la lunghezza del messaggio è compatibile? i=2 quando ricevo dati della pressione di un bottone, 7 quando ricevo quella del joystick
        if ((lettura[i] == FINE_TRASMISSIONE) && ((i == 2) || (i == 7)))
        {
          break;
        }
        // aumento l'indice per salvare la successiva lettura nella casella adatta
        i++;
      }
      if (i == 2)
      {
        getButtonState(lettura[1]);
      } else if (i == 7)
      {
        getStatoJoystick(lettura);
      }
    }
  }
}

void getStatoJoystick(byte data[8])
{
  int joyX = (data[1] - 48) * 100 + (data[2] - 48) * 10 + (data[3] - 48);       // ottengo un intero da una sequenza ASCII di cifre ex: 1 7 5    1*100 + 7*10 + 5 = 175
  int joyY = (data[4] - 48) * 100 + (data[5] - 48) * 10 + (data[6] - 48);
  joyX = joyX - 200;                                                  // correggo l'offset di 200
  joyY = joyY - 200;                                                  // che mi permette di non trasmettere numeri negativi

  // controllo che i valori elaborati siano compresi fra -100 e 100
  if (joyX < -100 || joyX > 100 || joyY < -100 || joyY > 100)
  {
    return; // c'è un errore, esco dalla funzione
  }
  
  convertiAssiInMotoreDXeSX(joyX,joyY);
  // scrivo sul monitor seriale cosa ho ottenuto
  //Serial.print("Joystick position:  ");
  //Serial.print(joyX);
  //Serial.print(", ");
  //Serial.println(joyY);
}

void getButtonState(int pulsantePremuto)
{
  switch (pulsantePremuto)
  {
// -----------------  BUTTON #1  -----------------------
    case 'A':
      arma_uno = true;
      //Serial.println("\n** Button_1: ON **");
      // your code...
      break;
    case 'B':
      arma_uno = true;
      //Serial.println("\n** Button_1: OFF **");
      // your code...
      break;
  
// -----------------  BUTTON #2  -----------------------
   case 'C':
      arma_due = true;
      //Serial.println("\n** Button_2: ON **");
      // your code...
      break;
    case 'D':
      arma_due = true;
      //Serial.println("\n** Button_2: OFF **");
      // your code...
      break;
  }
}

void convertiAssiInMotoreDXeSX(int y, int x)
{
  int motoreDX = 0;
  int motoreSX = 0;
  
  int gapSterzo = 10; // range valori 0 - 100;
  int gapAcceleratore = 10;
  
  if(-gapSterzo <= y && y <= gapSterzo) // fascia verticale centrale per dare la stessa accelerazione ai motori
  {
    if(x < -gapAcceleratore || gapAcceleratore < x) // fascia interte orizzontale per muovere i motori solo se c'è una velocità minima abbastanza elevata
    {
      motoreDX = x;
      motoreSX = x;
    }
  }
  else
  {
    if(-gapAcceleratore <= x && x <= gapAcceleratore) // fascia di sterzo orizzontale destra e sinistra, quando il joystick è in questa zona il robot ruoterà su sè stesso a velocità diverse
    {
      motoreDX = y;
      motoreSX = -y;
    }
    else
    {
      float coefficienteDX = 2.5;
      float coefficienteSX = 2.5;
      
      // una delle ruote rimarrà a piena potenza, l'altra avrà una potenza ridotta
      if(y < -gapSterzo) // area di sterzo sinistra
      {
        coefficienteDX = 1;
        coefficienteSX = (float)((float)(y + 100) / (float)(100));
      }
      if(gapSterzo < y) // area di sterzo destra
      {
         coefficienteDX = (float)((float)(100 - y) / (float)(100));
         coefficienteSX = 1;
      }

      motoreDX = x * coefficienteDX;
      motoreSX = x * coefficienteSX;
      //Serial.print("cSX ");
      //Serial.print(coefficienteDX);
      //Serial.print(", cDX ");
      //Serial.println(coefficienteDX);
    }
  }

//GESTISCO I MOTORI DX E SX

//------ GESTIONE MOTORE DX-------
  if (motoreDX > 0)
    {
      analogWrite(motorA1, motoreDX); 
      analogWrite(motorA2, 0); 
    }
  if(motoreDX < 0)
    {
      analogWrite(motorA1, 0); 
      analogWrite(motorA2, motoreDX);
    }
  if(motoreDX == 0)
    {
      analogWrite(motorA1, 0); 
      analogWrite(motorA2, 0); 
    }

//------ GESTIONE MOTORE SX-------

  if (motoreSX > 0)
    {
      analogWrite(motorB1, motoreSX); 
      analogWrite(motorB2, 0); 
    }
  if(motoreSX < 0)
    {
      analogWrite(motorB1, 0); 
      analogWrite(motorB2, motoreSX);
    }
  if(motoreSX == 0)
    {
      analogWrite(motorB1, 0); 
      analogWrite(motorB2, 0); 
    }
  //Serial.print("SX ");
  //Serial.print(motoreSX);
  //Serial.print(", DX ");
  //Serial.println(motoreDX);
}
