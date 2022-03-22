//Code for NFC alarm made by Casperdroid5 
//Special thanks to StefanL38 on the arduino forum
//Code is provided as is.
//v1.0

#define dbg(myFixedText, variableName) \
   Serial.print( F(#myFixedText " "  #variableName"=") ); \
   Serial.println(variableName);
// usage: dbg("1:my fixed text",myVariable);
// myVariable can be any variable or expression that is defined in scope

#define dbgi(myFixedText, variableName,timeInterval) \
   do { \
   static unsigned long intervalStartTime; \
   if ( millis() - intervalStartTime >= timeInterval ){ \
   intervalStartTime = millis(); \
   Serial.print( F(#myFixedText " "  #variableName"=") ); \
   Serial.println(variableName); \
   } \
   } while (false);

#include <Wire.h>
#include <avr/sleep.h>  
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Or use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

const int sensor = 8;       // door sensor input pin
int state;                  // statemachine state
int door;                   // doorstate 0 closed or 1 open
bool tagdata;
uint32_t cardid;       //nfc tag cardid

//this is for the blinking led:
int ledState = LOW;                     // ledState used to set the LED
unsigned long previousMillis = 0;       // will store last time LED was updated
const long interval = 1000;             // interval at which to blink (milliseconds)

//this is for the alarm timer:
int counter;
bool flag;

//this is for the buzzer
static const int buzzerPin = 9;    // set the variable "buzzerPin" to pin 12

const byte redLED_Pin   =  6;
const byte greenLED_Pin =  7;
const byte TestLED_Pin  = 13;
const byte relayLED = 4;
const byte relayHORN = 5;

const byte OPENED = true;
const byte CLOSED = false;

int wakePin = 8;                 // pin used for waking up  

void setup() {
   
   attachInterrupt(0, wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW  
   Serial.begin(115200);
   Serial.println( F("Setup-Start") );
   pinMode(relayLED, OUTPUT);     // sets the digital pin 4 as output for relay 1 = 30W led relay
   pinMode(relayHORN, OUTPUT);     // sets the digital pin 5 as output for relay 2 = Horn relay
   pinMode(buzzerPin, OUTPUT);       // set the buzzerPin as output
   pinMode(redLED_Pin, OUTPUT);        //  sets the digital pin 6 as output RED LED
   pinMode(greenLED_Pin, OUTPUT);     // sets the digital pin 7 as output GREEN LED
   pinMode(TestLED_Pin, OUTPUT);    // sets the digital pin 13 as output TEST LED
   pinMode(sensor, INPUT_PULLUP);  // sets pullup resistor on for sensor
   state = 1;                     // starts machine with first state
}


void loop () {
   // print content of variable "state" once every 1000 milliseconds
   dbgi("1:top of loop", state, 2000);
   // state = 1; <==== this is wrong 
   // loop shall L O O P ! and variable state controls
   // which part of the code gets executed
   // that is the sense of purpose state-machine
   
   switch (state)  {
      case 1:
         dbg("entering case 1", 0);
         digitalWrite(relayLED, HIGH);  // turn relay 1 off
         digitalWrite(relayHORN, HIGH);  // turn relay 2 off
         Serial.println("Hello!");
         Serial.println("Alarmsystem booting up");
         Serial.println("Alarm status: UNARMED");
         Serial.println("Alarm will be armed when door is closed");
         
         dbg("direct before for-loop blink led", 1);
         for (int i = 0; i <= 1; i++) // 10 seconds bootup (when set to 9)
         {
            digitalWrite(greenLED_Pin, HIGH);  // turn green light on
            delay(500);
            digitalWrite(greenLED_Pin, LOW);  // turn green light off
            digitalWrite(redLED_Pin, HIGH);  // turn red light on
            delay(500);
            digitalWrite(redLED_Pin, LOW);  // turn red light off
         }
         dbg("direct after for-loop blink led", 2);
         
         dbg("checkdoorstate()", 3);
         checkdoorstate();
         if (door == CLOSED) 
         {
            Serial.println("Door is closed");
            Serial.println("Alarmsystem booted");
            state = 2;
         }
         dbg("leaving case 1", 4);
         break;
         
      case 2:
         attachInterrupt(0, wakeUpNow, HIGH);
         LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
         detachInterrupt(0);
         dbgi("entering case 2", 5, 3000);
         blinkingled();
         checkdoorstate();
         if (door == OPENED) 
         {
            Serial.println("Door is opened");
            state = 3;
         }
         dbgi("leaving case 2", 6,3000);
         break;
         
      case 3:
         dbgi("entering case 3", 7, 3000);
         Serial.println("Door is opened");
         digitalWrite(redLED_Pin, LOW);  // turn red light off
         nfcstart();
         checktag(cardid);
         tensecondstimer();
         if (tagdata  == true)   // if a tag has been recognized
         {
            beep(); // execute the "beep" function
            Serial.println("Alarm has been Dissarmed");
            digitalWrite(greenLED_Pin, HIGH);  // turn green light on
            Serial.println("Waiting for door to be closed again...");
            state = 4;
         }
         if (flag == true) // If the ten second timer is over. Alarm will go off
         {
            Serial.println("NFC not detected or too late");
            Serial.println("ALARM TRIGGERD");
            Serial.println("Activating light and horn");
            state = 5;
         }
         dbgi("leaving case 3", 8, 3000);
         break;
         
      case 4:
         dbgi("entering case 4", 9, 3000);      
         checkdoorstate();
         if (door == CLOSED) // if the door is closed 
         {
            cardid = 0;
            counter = 0;
            Serial.println("Door is closed");
            Serial.println("Alarm status: ARMED");
            digitalWrite(greenLED_Pin, LOW);  // turn green light off
            state = 2;
         }
         dbgi("leaving case 4", 10, 3000);
         break;
         
      case 5:
         dbg("entering case 5", 11);      
         digitalWrite(relayLED, LOW);  // turn relay 1 on
         digitalWrite(relayHORN, LOW);  // turn relay 2 on
         delay (3000); // alarm goes wild for 3 seconds.
         flag = false;
         counter = 0; // reset counter
         state = 1;   // reboot system
         dbg("leaving case 5", 12);
         break;
         
      default:
         dbg("default", 13);
         break;
   }
}


int checkdoorstate() {
   int doorstate;
   doorstate = digitalRead(sensor); // check the status of the door (open or closed)
   
   if (doorstate == HIGH) {        // obsolete door OPENED
      door = OPENED; //obsolete door is open
   }
   else
   {
      door = CLOSED; // obsolete door is closed
   }
   return door;
}

void blinkingled() {
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= interval)  {
      previousMillis = currentMillis;
      if (ledState == LOW){
         ledState = HIGH;
      }
      else {
         ledState = LOW;
      }
      digitalWrite(redLED_Pin, ledState);
   }
}

void nfcstart() // start of nfc system
{
   nfc.begin();
   uint32_t versiondata = nfc.getFirmwareVersion();
   if (! versiondata)
   {
      Serial.print("Didn't find PN53x board");
      while (1); // halt
   }
   // Got ok data, print it out!
   Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
   Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
   Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
   
   // configure board to read RFID tags
   nfc.SAMConfig();
   Serial.println("Waiting for an ISO14443A Card ...");
   uint8_t success;
   uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
   uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
   uint8_t mifareclassic_ReadDataBlock;
   
   // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
   // 'uid' will be populated with the UID, and uidLength will indicate
   // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
   success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
   
   if (success)  // if a nfc tag has been detected
   {
      // Display some basic information about the card
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
      Serial.print("  UID Value: ");
      
      nfc.PrintHex(uid, uidLength);
      
      if (uidLength == 4)
      {
         // We probably have a Mifare Classic card ...
         cardid = uid[0];
         cardid <<= 8;
         cardid |= uid[1];
         cardid <<= 8;
         cardid |= uid[2];
         cardid <<= 8;
         cardid |= uid[3];
         Serial.print("Seems to be a Mifare Classic card #");
         Serial.println(cardid);
         return cardid;
      }
   }
}

bool checktag(uint32_t cardid) // checks the nfc tag database for matches.
{
   if (cardid == 0)
   {
      dbg(No cardid detected to be checked, cardid);
      tagdata = false;
      return tagdata;
   }
   Serial.println("Searching for match in NFC tag database...");
   dbg(Detected cardid: , cardid); //Serial.println(cardid);
   if (cardid == 2861708310)
   {
      digitalWrite(TestLED_Pin, HIGH);  // sets the digital pin 13 on
      Serial.print("Detected card id match: ");
      Serial.print(cardid);
      Serial.println(" = NFC card: 1");
      tagdata = true;
      return tagdata;
   }
   else if (cardid == 2864376086)
   {
      digitalWrite(TestLED_Pin, HIGH);  // sets the digital pin 13 on
      Serial.print("Detected card id match: ");
      Serial.print(cardid);
      Serial.println(" = NFC card: 2");
      tagdata = true;
      return tagdata;
   }
   else if (cardid == 3403313686)
   {
      digitalWrite(TestLED_Pin, HIGH);  // sets the digital pin 13 on
      Serial.print("Detected card id match: ");
      Serial.print(cardid);
      Serial.println(" = NFC card: 3");
      tagdata = true;
      return tagdata;
   }
   else if (cardid == 2863228950)
   {
      digitalWrite(TestLED_Pin, HIGH);  // sets the digital pin 13 on
      Serial.print("Detected card id match: ");
      Serial.print(cardid);
      Serial.println(" = NFC card: 4");
      tagdata = true;
      return tagdata;
   }
   else if (cardid == 3392821014)
   {
      digitalWrite(TestLED_Pin, HIGH);  // sets the digital pin 13 on
      Serial.print("Detected card id match: ");
      Serial.print(cardid);
      Serial.println(" = NFC card: 5");
      tagdata = true;
      return tagdata;
   }
   Serial.println("No match found in NFC tag database...");
}

bool tensecondstimer() // starts ten second timer and returns flag when 10 seconds are over.
{
   unsigned long currentMillis = millis(); // grab current time
   if ((unsigned long)(currentMillis - previousMillis) >= interval)  // check if "interval" time has passed (1000 milliseconds)
   {
      flag = false;
      counter ++;
      Serial.print("Door opened for: ");
      Serial.print(counter);
      Serial.println(" seconds.");
      previousMillis = millis();
      if (counter > 9) // reset timer afer 10 seconds
      {
         Serial.println("counter reached 10 seconds");
         counter = 0;
         flag = true;
         return flag;
      }
   }
}

void beep() 
{
   tone(buzzerPin, 800, 50);        // give a tone on the buzzerPin, at 196hz for, 200ms  
}
