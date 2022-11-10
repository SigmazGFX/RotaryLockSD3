/* RotaryLockSD3

This new version of the RotaryLock puzzle/controller utilizes a new file based configuration, as opposed to the previous hard coded combination number and media.

## Getting Started

You will need an Arduino UNO, an SD card with SD shield or SD card adaptor/MicroSD which you can wire directly to SPI

Oh, and of course, a Rotary Phone.

### Prerequisites
You will also need to already be familiar with using the RotaryDialer library and how to wire the rotary dial into your project. 
Becasue it's super easy, I'm not going to cover it.
If you are planning on soldering wires from the SPI port directly to your SD card adaptor, Bravo! That's what I did and it works fantastic.
I see no need to purchase an SD shield nor waste valuable space inside the phone.. 
I selected to use the TMRPCM library in order to eliminate the need to use a wav shield. 
All audio is processed and played back by the Arduino on pin 9. (again we save $$ and space).
Both of the libraries I use are included in this repo.
### Installing
The pins used by hardware is as follows:

  | Pin/Name |Description|
  |--------------|---------|
  | 2 Handset_PIN   | Used to tell MCU when handset is On-Hook|
  | 3 Pulse_PIN| Pulse contacts of Rotary Dial|
  | 4  SD Chip Select | Needed for SD card support|
  | 5  ioPIN5| in service as digital toggle pin|
  | 6  DIALRDY_PIN    | Rotary dial at rest contacts|
  | 7  ioPIN7| in service as digital toggle pin|
  | 8  ioPIN8| in service as digital toggle pin|
  | 9  Speaker_PIN| Generated audio played via this pin 
  | 10 SPI SS         | Needed for SD card support|
  | 11 SPI MOSI |Needed for SD card support|
  | 12 SPI MISO     |   Needed for SD card support|
  | 13 SPI SCK |Needed for SD card support|
  | A0-A5|ioPIN14-ioPIN19 in service as digital toggle pin|

  
  
 Users may now use custom sound files which can be triggered by any 7 digit number dialed.
 The same dialed numbers may also be used to control any one of the 9 available digital pins states. 

  |Available Pins on UNO|
  |------------------------|
  |Pin5,Pin7,Pin8,A0,A1,A2,A3,A4,A5|
                         
 Sound Files:
The system has a set of preloaded "generic bad destination" sound files stored in the stdsnd/ folder that are used when a number is dialed that is not matched to a custom .wav file
Convert your custom .wav file to 8bit 8-16khz mono and copy it to the root of the SD card. 
Rename your sound file to the 7 digit number you'd like to use to activate it. ( i.e. 5555555.wav )  
When the number is dialed, your custom file will be selected and played to the user. This could be handy for an information kiosk, puzzle clue, or whatever...
  
  Locks/Switches:
The system now has the ability to trigger a digital pin to change its state in coordination with the custom sound file.
If you would like to trigger a pin on the arduino with a phone number dialed you must first create the custom .wav file and place it in the root of the SD card.
The next thing you will need to do is create a custom .loc file. This file can be created in notepad and the name must reflect the same number as the associated .wav file
(i.e. 5555555.loc)
 
  |Inside the .loc file we will need to populate 3 variables:|
  |--------------------------------------------------------------------------------------------------------------------------------------------------------|
  | [exLockPIN=x] "x" being the pin # to use (5,7,8 or 14-19 for A0-A5)                                                                                                               |
  | [exDelay=xxxx] "x" being the # of milliseconds to wait before the first state change. Useful if you want to sync with events playing in the recording.|
  | [exHold=xxxx] "x" being the # of milliseconds to wait before changing the state back to it's idle setting.                                            |
  
  Configuration:
When the system is first started up the digital pins need to be configured to reflect your needs.
normally this is done in the code prior to compiling and should something change with the installation you'd need to recompile every time.
In this iteration I have built in the ability to set the pins up via config.txt
Because several of the pins are in use by various aspects of the hardware the only free pins for digital switching are D5 and A0-A5, 7 pins.
The idle default state of these pins are configured as such in the config.txt file.

 |CONFIG.TXT
 |----------------------------------------------|
 |[ioPIN5HL=1] digital pin 5  1 = HIGH / 0 = LOW|
 |[ioPIN7HL=0] digital pin 7  1 = HIGH / 0 = LOW|
 |[ioPIN8HL=1] digital pin 8  1 = HIGH / 0 = LOW|
 |[ioPIN14HL=0] analog A0     1 = HIGH / 0 = LOW|
 |[ioPIN15HL=1] analog A1     1 = HIGH / 0 = LOW|
 |[ioPIN16HL=0] analog A2     1 = HIGH / 0 = LOW|
 |[ioPIN17HL=1] analog A3     1 = HIGH / 0 = LOW|
 |[ioPIN18HL=0] analog A4     1 = HIGH / 0 = LOW|
 |[ioPIN19HL=1] analog A5     1 = HIGH / 0 = LOW|

You may change the default state of any or all of the pins, the settings presented here are just an example.
NOTE: A recommended best practice would be to set any unused pins low.


  Enjoy


  Sigmaz (JB) 1/22/2018
*/

#include <SD.h>
#include <SPI.h>
#include <RotaryDialer.h>

// change this to match your SD shield or module;
const int chipSelect = 4;
int currentNumber = 0;

// set up sound functions
#include <TMRpcm.h>
TMRpcm tmrpcm;

// pins(************ THESE ALL MUST BE REDEFINED BASED ON ACTUAL HARDWARE **************)
#define SPEAKER_PIN 9
#define HANDSET_PIN 2
#define PULSE_PIN 3
#define DIALRDY_PIN 6
#define ioPIN5   5
#define ioPIN7   7
#define ioPIN8   8
#define ioPIN14 A0
#define ioPIN15 A1
#define ioPIN16 A2
#define ioPIN17 A3
#define ioPIN18 A4
#define ioPIN19 A5

#define NUMLENGTH 7

// handset pin states
#define HANDSET_DOWN HIGH
#define HANDSET_UP LOW

// Handset & Dialing Management
enum {
  STATE_INITIALIZE_UNIT,
  STATE_ON_HOOK,
  STATE_OFF_HOOK,
  STATE_DIALING,
  STATE_DIALING_TIMEOUT_ERROR,
  STATE_NUMBER_DIALED,
  STATE_CONNECTING,
  STATE_UNLOCK,
  STATE_LOCK,
  STATE_CONT_DIALING,
  STATE_VICTORY_SONG,
  STATE_WRONG_NUMBER,

} PhoneState;

// INITIAL STATE
int state;
boolean handsetState;
boolean dialerState = true;
long lastDialerToggle = 0;
int dialedNumberAcc = 0;
int unlockEnable = 0;
long startDialingTStamp;
long silenceStartedTStamp;
int digitLoop = 0;
int numArray[10]; // Size of array for dialed digits to hang out in

RotaryDialer dialer = RotaryDialer(DIALRDY_PIN, PULSE_PIN); // Oh yeah, lets define the dialer.. we might need that.
int wrongNumLoop = 0;

//added for SD .loc file test and control
File myFile;
char unlockFile[8];
int exLockPIN = 0;
float exDelay = 1000;
float exHold = 1000;
int ioPIN5HL = 0;
int ioPIN7HL = 0;
int ioPIN8HL = 0;
int ioPIN14HL = 0;
int ioPIN15HL = 0;
int ioPIN16HL = 0;
int ioPIN17HL = 0;
int ioPIN18HL = 0;
int ioPIN19HL = 0;



void setup() {
//Serial.println ("Pin/Name          | Description                                      |");
//Serial.println ("2  Handset_PIN    | Used to tell MCU when handset is On-Hook         |");
//Serial.println ("3  Pulse_PIN      | Pulse contacts of Rotary Dial                    |");
//Serial.println ("4  SD Chip Select | Needed for SD card support                       |");
//Serial.println ("5  ioPIN5         | in service as digital toggle pin                 |");
//Serial.println ("6  DIALRDY_PIN    | Rotary dial at rest contacts                     |");
//Serial.println ("7  ioPIN7         | in service as digital toggle pin                 |");
//Serial.println ("8  ioPIN8         | in service as digital toggle pin                 |");
//Serial.println ("9  Speaker_PIN    | Generated audio played via this pin              |");
//Serial.println ("10 SPI SS         | Needed for SD card support                       |");
//Serial.println ("11 SPI MOSI       | Needed for SD card support                       |");
//Serial.println ("12 SPI MISO       | Needed for SD card support                       |");
//Serial.println ("13 SPI SCK        | Needed for SD card support                       |");
//Serial.println ("A0-A5             | ioPIN14-ioPIN19 in service as digital toggle pin |");



  dialer.setup();

  tmrpcm.speakerPin = 9; //used for SD card audio playback
  Serial.begin(9600);


  state = STATE_INITIALIZE_UNIT;
  handsetState = HANDSET_DOWN;


  //Handset off-on hook pin
  pinMode(HANDSET_PIN, INPUT);
  digitalWrite(HANDSET_PIN, HIGH); // turn on pull-up


  //UNLOCK_PIN init
  pinMode(ioPIN5, OUTPUT);
  pinMode(ioPIN7, OUTPUT);
  pinMode(ioPIN8, OUTPUT);
  pinMode(ioPIN14, OUTPUT);
  pinMode(ioPIN15, OUTPUT);
  pinMode(ioPIN16, OUTPUT);
  pinMode(ioPIN17, OUTPUT);
  pinMode(ioPIN18, OUTPUT);
  pinMode(ioPIN19, OUTPUT);

  if (!SD.begin(chipSelect)) {  // see if the card is present and can be initialized:
    // Serial.println("SD fail");
    return;   // don't do anything more if not
  }
  readConfigFile();
  reconfigPins();
  //play SIT sound after init to let us know everything is ready to go
  tmrpcm.play("stdsnds/sit.wav"); //the sound file "music" will play each time the arduino powers up, or is reset
  delay(1000);
}

void reconfigPins() {
  digitalWrite(ioPIN5,   ioPIN5HL);
  digitalWrite(ioPIN7,   ioPIN5HL); 
  digitalWrite(ioPIN8,   ioPIN5HL);
  digitalWrite(ioPIN14, ioPIN14HL);
  digitalWrite(ioPIN15, ioPIN15HL);
  digitalWrite(ioPIN16, ioPIN16HL);
  digitalWrite(ioPIN17, ioPIN17HL);
  digitalWrite(ioPIN18, ioPIN18HL);
  digitalWrite(ioPIN19, ioPIN19HL);
  
}


void loop() {
  switch (state) {
    case STATE_INITIALIZE_UNIT:         state = handleInitializeUnit();        break;
    case STATE_ON_HOOK:                 state = handleOnHook();                break;
    case STATE_OFF_HOOK:                state = handleOffHook();               break;
    case STATE_DIALING:                 state = handleDialing();               break;
    case STATE_DIALING_TIMEOUT_ERROR:   state = handleDialingTimeout();        break;
    case STATE_NUMBER_DIALED:           state = handleNumberDialed();          break;
    case STATE_CONNECTING:              state = handleConnecting();            break;
    case STATE_UNLOCK:                  state = handleUnlock();                break;
    case STATE_LOCK:                    state = handleLock();                  break;
    case STATE_CONT_DIALING:            state = handleContDialing();           break;
    case STATE_VICTORY_SONG:            state = handleVictorySong();           break;
    case STATE_WRONG_NUMBER:            state = handleWrongNumber();           break;

  }
  dialer.update();

  if (state != STATE_ON_HOOK) {
    if (!isHandsetOffHook()) {
      tmrpcm.disable();
      state = STATE_ON_HOOK;
    }
  }
}


int handleVictorySong() {
  if (unlockEnable == 1) {
    return STATE_UNLOCK;
  } else {
    return STATE_LOCK;
  }
}


int handleUnlock() {

  delay (exDelay);//wait before switching
  
  if (exLockPIN == 5) {
    digitalWrite(ioPIN5, !ioPIN5HL);
  }
  if (exLockPIN == 7) {
    digitalWrite(ioPIN7, !ioPIN7HL);
  }
  if (exLockPIN == 8) {
    digitalWrite(ioPIN8, !ioPIN8HL);
  }
  if (exLockPIN == 14) {
    digitalWrite(ioPIN14, !ioPIN14HL);
  }
  if (exLockPIN == 15) {
    digitalWrite(ioPIN15, !ioPIN15HL);
  }
  if (exLockPIN == 16) {
    digitalWrite(ioPIN16, !ioPIN16HL);
  }
  if (exLockPIN == 17) {
    digitalWrite(ioPIN17, !ioPIN17HL);
  }
  if (exLockPIN == 18) {
    digitalWrite(ioPIN18, !ioPIN18HL);
  }
  if (exLockPIN == 19) {
    digitalWrite(ioPIN19, !ioPIN19HL);
  }

  delay (exHold);// wait before switching back
  
  if (exLockPIN == 5) {
    digitalWrite(ioPIN5, ioPIN5HL);
  }
  if (exLockPIN == 7) {
    digitalWrite(ioPIN7, ioPIN7HL);
  }
  if (exLockPIN == 8) {
    digitalWrite(ioPIN8, ioPIN8HL);
  }
  if (exLockPIN == 14) {
    digitalWrite(ioPIN14, ioPIN14HL);
  }
  if (exLockPIN == 15) {
    digitalWrite(ioPIN15, ioPIN15HL);
  }
  if (exLockPIN == 16) {
    digitalWrite(ioPIN16, ioPIN16HL);
  }
  if (exLockPIN == 17) {
    digitalWrite(ioPIN17, ioPIN17HL);
  }
  if (exLockPIN == 18) {
    digitalWrite(ioPIN18, ioPIN18HL);
  }
  if (exLockPIN == 19) {
    digitalWrite(ioPIN19, ioPIN19HL);
  }

  dialedNumberAcc = 0;
  digitLoop = 0;
  wrongNumLoop = 0;
  lastDialerToggle = 0;
  unlockEnable = 0;
  exLockPIN = 0;
  exDelay = 0;
  exHold = 0;
  //---reset stuff
  InititalizeArray();

  return STATE_OFF_HOOK;


}

int handleLock() {
  reconfigPins();
  unlockEnable = 0;
  return STATE_LOCK;
}



int handleOffHook() {
  dialedNumberAcc = 0;
  startDialingTStamp = millis();
  tmrpcm.play("stdsnds/dt10.wav");
  Serial.println(F("off hook"));
  return STATE_DIALING;
}


int handleOnHook() {
  InititalizeArray();  // Clear the dialed number array
  tmrpcm.disable();
  //Serial.println("on hook");
  digitLoop = 0;
  return isHandsetOffHook() ? STATE_OFF_HOOK : STATE_ON_HOOK;
}

int handleInitializeUnit() {

  return STATE_ON_HOOK;
}


boolean isHandsetOffHook() {
  return (safeDigitalRead(HANDSET_PIN) == HANDSET_UP);
}


int handleContDialing() {//-------------- Collect up dialed digits until we have enough --------

  // Serial.print("digitloop: "); //print # of digit collection loop we are on
  // Serial.println (digitLoop);

  //---OK, Lets try to store the dialed digit into an array
  numArray[digitLoop] = currentNumber ;// put the currently doaled digit into the array at location "digitLoop"
  digitLoop ++;
  // Serial.print("numArray[digitLoop] value:  "); //Print whats getting put into the array
  // Serial.println (currentNumber);


  if (digitLoop < NUMLENGTH )  // Do we have enough digits yet? We need NUMLENGTH
  {
    ShowArray();
    return STATE_DIALING;
  }
  else
  {
    ShowArray();
    return STATE_CONNECTING;  //Move on to another state to use the numbers we've collected
  }

}


void InititalizeArray()
{

  for (int i = 0; i < NUMLENGTH; i++) {
    numArray[i] = 0;
  }
}

int handleDialing() {

  if ((millis() - startDialingTStamp) > 30000) { //---If 30 seconds pass with no activity

    Serial.println(F("Dialing Timeout Error"));
    return STATE_DIALING_TIMEOUT_ERROR;
  }

  if (dialer.update()) {
    currentNumber = dialer.getNextNumber();
    return STATE_NUMBER_DIALED;
  } else {
    return STATE_DIALING;
  }
}

int handleDialingTimeout() { //Error tone when the phone is left too long without dialing activity.
  tmrpcm.play("stdsnds/howler.wav");
  delay(4000);
  return STATE_DIALING_TIMEOUT_ERROR; //just keep looping back to here until the phone is hung up
}

int handleNumberDialed() {
  tmrpcm.stopPlayback();
  // Serial.print("Number dialed: ");
  // Serial.println(currentNumber);
  return STATE_CONT_DIALING; //Return to continue dialing

}

void ShowArray()
{

  for (int i = 0; i < NUMLENGTH; i++) {
    //   Serial.print ("numArray["); Serial.print (i); Serial.print("]: ");
    //   Serial.println(numArray[i]);
  }

}

int handleConnecting() { // Here we will check the numbers dialed to see if they are valid or what we should do with them
  //Lets check to see if this number has an unlock file.
  sprintf(unlockFile, "%1d%1d%1d%1d%1d%1d%1d.loc", numArray[0], numArray[1], numArray[2], numArray[3], numArray[4], numArray[5], numArray[6], numArray[7]);
  if (SD.exists(unlockFile)) {
    //YES! The dialed number can switch the lock pin so we should do something here or jump to another function to handle the task
    unlockEnable = 1;
    Serial.print(F("UNLOCKFILE PRESENT!!"));
    readUnlockFile();
  } else {
    unlockEnable = 0;
    Serial.print (F("No unlock file for this #"));
  }

  char finalNumber[7];
  sprintf(finalNumber, "%1d%1d%1d%1d%1d%1d%1d.wav", numArray[0], numArray[1], numArray[2], numArray[3], numArray[4], numArray[5], numArray[6]);

  if (SD.exists(finalNumber)) {
    Serial.print(finalNumber);
    Serial.println(F(" exists."));
    tmrpcm.play("stdsnds/ring8.wav"); //add a little ringing before the recording is played
    delay(5000);
    tmrpcm.play(finalNumber);//Play the associated recording
    delay(5000);

  } else {
     //no custom .wav file so lets treat it as a generic bad destination number
    return STATE_WRONG_NUMBER;
  }
  if (unlockEnable == 1) {
    return STATE_VICTORY_SONG;

  }
}

int handleWrongNumber() {


  //Be sure to return STATE_LOCK after wrong number destinations.

  wrongNumLoop ++;

//NOTE: I would like to have this section randomize the selection of the error sound file to be selected. also it should skip to another if its been played within the last 5 call attempts.

  if (wrongNumLoop == 1) {
    tmrpcm.play("stdsnds/busy.wav");
  }
  if (wrongNumLoop == 2) {
    tmrpcm.play("stdsnds/dcerr.wav");
  }
  if (wrongNumLoop == 3) {
    tmrpcm.play("stdsnds/ring8.wav");
    delay(1000);
    tmrpcm.play("stdsnds/rnd3.wav");
  }
  if (wrongNumLoop == 4) {
    tmrpcm.play("stdsnds/ring8.wav");
  }
  if (wrongNumLoop == 5) {
    tmrpcm.play("stdsnds/nocall.wav");
  }
  if (wrongNumLoop == 6) {
    tmrpcm.play("stdsnds/rnd2.wav");
  }
  if (wrongNumLoop == 7) {
    tmrpcm.play("stdsnds/ring8.wav");
    delay(1000);
    tmrpcm.play("stdsnds/nis.wav");

    wrongNumLoop = 0;
  }
  return STATE_LOCK;
}


void readUnlockFile() {
  char character;
  String settingName;
  String settingValue;

  myFile = SD.open(unlockFile);
  if (myFile) {
    while (myFile.available()) {
      character = myFile.read();
      while ((myFile.available()) && (character != '[')) {
        character = myFile.read();
      }
      character = myFile.read();
      while ((myFile.available()) && (character != '=')) {
        settingName = settingName + character;
        character = myFile.read();
      }
      character = myFile.read();
      while ((myFile.available()) && (character != ']')) {
        settingValue = settingValue + character;
        character = myFile.read();
      }
      if (character == ']') {
              //Debuuging Printing
//                Serial.println(" ");
//                Serial.print("Name:");
//                Serial.println(settingName);
//                Serial.print("Value :");
//                Serial.println(settingValue);

        // Apply the value to the parameter
        applySetting(settingName, settingValue);
        // Reset Strings
        settingName = "";
        settingValue = "";
      }
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    //   Serial.println("error opening unlockFile");
  }
}


//this section is used to read the config.txt file and preconfigure the digital pins states
void readConfigFile() {
  char character;
  String settingName;
  String settingValue;
  //Serial.print("config.txt");
  myFile = SD.open("config.txt");
  if (myFile) {
    while (myFile.available()) {
      character = myFile.read();
      while ((myFile.available()) && (character != '[')) {
        character = myFile.read();
      }
      character = myFile.read();
      while ((myFile.available()) && (character != '=')) {
        settingName = settingName + character;
        character = myFile.read();
      }
      character = myFile.read();
      while ((myFile.available()) && (character != ']')) {
        settingValue = settingValue + character;
        character = myFile.read();
      }
      if (character == ']') {
        //Debuuging Printing
//                Serial.println(" ");
//                Serial.print("Name:");
//                Serial.println(settingName);
//                Serial.print("Value :");
//                Serial.println(settingValue);


        // Apply the value to the parameter
        applySetting(settingName, settingValue);
        // Reset Strings
        settingName = "";
        settingValue = "";
      }
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("error opening config.txt"));
  }
}

void applySetting(String settingName, String settingValue) {
  if (settingName == "exLockPIN") {
    exLockPIN = settingValue.toInt();
  }
  if (settingName == "exDelay") {
    exDelay = toFloat(settingValue);
  }
  if (settingName == "exHold") {
    exHold = toFloat(settingValue);
  }
  if (settingName == "ioPIN5HL") {
    ioPIN5HL = settingValue.toInt();
  }
  if (settingName == "ioPIN7HL") {
    ioPIN7HL = settingValue.toInt();
  }
  if (settingName == "ioPIN8HL") {
    ioPIN8HL = settingValue.toInt();
  }
  if (settingName == "ioPIN14HL") {
    ioPIN14HL = settingValue.toInt();
  }
  if (settingName == "ioPIN15HL") {
    ioPIN15HL = settingValue.toInt();
  }
  if (settingName == "ioPIN16HL") {
    ioPIN16HL = settingValue.toInt();
  }
  if (settingName == "ioPIN17HL") {
    ioPIN17HL = settingValue.toInt();
  }
  if (settingName == "ioPIN18HL") {
    ioPIN18HL = settingValue.toInt();
  }
  if (settingName == "ioPIN19HL") {
    ioPIN19HL = settingValue.toInt();
  }
}

// converting string to Float
float toFloat(String settingValue) {
  char floatbuf[settingValue.length() + 1];
  settingValue.toCharArray(floatbuf, sizeof(floatbuf));
  float f = atof(floatbuf);
  return f;
}

// Converting String to integer and then to boolean
// 1 = true
// 0 = false
boolean toBoolean(String settingValue) {
  if (settingValue.toInt() == 1) {
    return true;
  } else {
    return false;
  }
}
///////////////////////////////////
// Utility methods

boolean safeDigitalRead(byte pin) {
  boolean sample = digitalRead(pin);
  int consistentReadCount = 1;
  while (consistentReadCount < 3) {
    delayMicroseconds(500);
    if (digitalRead(pin) == sample) {
      consistentReadCount++;
    }
    else {
      sample = digitalRead(pin);
      consistentReadCount = 1;
    }
  }
  return sample;
}





