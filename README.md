
# RotaryLockSD3

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
Flash the HEX file to your UNO using the supplied XLoader flasher.
Copy the /stdsnds folder and its contents as well as the example config.txt, .wav, and .loc files in the repo to the root of the SD card.
Do not copy the RotaryDialer or TMRPCM libraries to your SD card those are for reference only.


The pins used by hardware is as follows:

  | Pin/Name |Description|
  |--------------|---------|
  | 2 Handset_PIN   | Used to tell MCU when handset is On-Hook|
  | 3 Pulse_PIN| Pulse contacts of Rotary Dial|
  | 4  SD Chip Select | Needed for SD card support|
  | 5  ioPIN5| in service as digital toggle pin|
  | 6  DIALRDY_PIN    | Rotary dial at rest contacts|
  | 7  ~ioPIN7~|~in service as digital toggle pin~|
  | 8  ~ioPIN8~|~in service as digital toggle pin~|
  | 9  Speaker_PIN| Generated audio played via this pin 
  | 10 SPI SS         | Needed for SD card support|
  | 11 SPI MOSI |Needed for SD card support|
  | 12 SPI MISO     |   Needed for SD card support|
  | 13 SPI SCK |Needed for SD card support|
  | A0-A5|ioPIN14-ioPIN19 in service as digital toggle pin|

  
  
 Users may now use custom sound files which can be triggered by any 7 digit number dialed.
 The same dialed numbers may also be used to control any one of the ~9~ 7 available digital pins states.
 (working on a bug that casues the code to misbehave when pins 7-8 are enabled)

  |Available Pins on UNO|
  |------------------------|
  |Pin5,~Pin7~,~Pin8~,A0,A1,A2,A3,A4,A5|
                         
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
  |  [exLockPIN=x] "x" being the pin # to use (5,~7,8~ or 14-19 for A0-A5)                                                                                                               |
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
 |~[ioPIN7HL=0] digital pin 7  1 = HIGH / 0 = LOW~|
 |~[ioPIN8HL=1] digital pin 8  1 = HIGH / 0 = LOW~|
 |[ioPIN14HL=0] analog A0     1 = HIGH / 0 = LOW|
 |[ioPIN15HL=1] analog A1     1 = HIGH / 0 = LOW|
 |[ioPIN16HL=0] analog A2     1 = HIGH / 0 = LOW|
 |[ioPIN17HL=1] analog A3     1 = HIGH / 0 = LOW|
 |[ioPIN18HL=0] analog A4     1 = HIGH / 0 = LOW|
 |[ioPIN19HL=1] analog A5     1 = HIGH / 0 = LOW|

You may change the default state of any or all of the pins, the settings presented here are just an example.
NOTE: A recommended best practice would be to set any unused pins low.

