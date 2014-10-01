
/*******************************************************************************

 Bare Conductive Touch MP3 player
 ------------------------------

 Touch_MP3_player.ino - touch triggered MP3 playback


 This version implements the normal controls on an MP3 Player via touch pins:

   0  - start
   1  - stop
   2  - next Track
   3  - previous Track
   4  - pause
   5  - resume

   10 - decrease volume one step
   11 - increase volume one step

 The player plays TRACK0nn.mp3 files with consecutive track numbers nn in range 0 to 999

 In the absence of input, the player plays one track after another

 The SD card is read at FULL speed rather than half speed in an attempt to reduce jitter


 Adapted from the base Touch_MP3 code

 Based on code by Jim Lindblom and plenty of inspiration from the Freescale
 Semiconductor datasheets and application notes.

 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.


 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.


  CW - added volume control using Electrodes 10 (down) and 11 (up)
     - added code to skip on to the next track if a track finishes
     - refactored the code for readablity
     - imlement controls for start, stop, next and previous track.
*******************************************************************************/


// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
int lastPlayed = 0;
bool playing = false;
int vol = 30;
int finalTrack;
#define interTrackGap 1000
#define volStep 5

// touch behaviour definitions
#define startPin 0
#define stopPin 1
#define nextTrackPin 2
#define prevTrackPin 3
#define pausePin 4
#define resumePin 5
#define volDownPin 10
#define volUpPin 11

// sd card instantiation
SdFat sd;
SdFile sdfile;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

void setup() {
  Serial.begin(57600);
  pinMode(LED_BUILTIN, OUTPUT);
  while (!Serial)  {
    ;
  }
  Serial.println("Bare Conductive Touch MP3 player");

  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/", true)) sd.errorHalt("sd.chdir");

  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  result = MP3player.begin();
  if (result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }
  MP3player.setVolume(vol, vol);

  finalTrack  = getFinalTrack();
  Serial.print("Final Track Number ");
  Serial.println(finalTrack);

}

void loop() {
  if (MPR121.touchStatusChanged())
    readTouchInputs();
  else
    checkTrackFinished();
}

// touch dispatcher
void readTouchInputs() {
  MPR121.updateTouchData();
  if (MPR121.getNumTouches() == 1) {
    // only act if we have one pin touched
    switch (pinTouched()) {
      case startPin:
        startTrack(0); break;
      case  stopPin:
        stopTrack(); break;
      case nextTrackPin:
        nextTrack(); break;
      case prevTrackPin:
        prevTrack(); break;
      case pausePin:
        pauseTrack(); break;
      case resumePin:
        resumeTrack(); break;
      case volUpPin:
        upVolume(); break;
      case volDownPin:
        downVolume(); break;
        // ignore unused pin
    }
  }
}

int pinTouched() {
  int i = 0;
  while (i < 12) {
    if (MPR121.isNewTouch(i))
      return i;
    i++;
  }
  return -1;
}


//  actionn functions

void startTrack (int i) {
  if (playing)
    stopTrack();
  char * name = filename(i);
  MP3player.playMP3(name);
  Serial.print("playing track ");
  Serial.println(i);
  lastPlayed = i;
  playing = true;
}

void stopTrack() {
  MP3player.stopTrack();
  Serial.print("stopping track ");
  Serial.println(lastPlayed);
  playing = false;
}

void nextTrack() {
  int i = lastPlayed + 1 ;
  if (i > finalTrack)
    i = 0;
  startTrack(i);
}

void prevTrack() {
  int i = lastPlayed - 1 ;
  if (i < 0) {
    i = finalTrack;
  }
  startTrack(i);
}

void pauseTrack() {
  MP3player.pauseMusic();
  Serial.print("paused track ");
  Serial.println(lastPlayed);
  playing = false;
}

void resumeTrack() {
  MP3player.resumeMusic();
  Serial.print("resuming track ");
  Serial.println(lastPlayed);
  playing = true;
}

void upVolume() {
  vol =  max(0, vol - volStep);
  Serial.print("Volume up ");
  Serial.println(vol);
  flash(100);
  MP3player.setVolume(vol, vol);
}

void downVolume() {
  vol =  min(254, vol + volStep);
  Serial.print("Volume down ");
  Serial.println(vol);
  flash(100);
  MP3player.setVolume(vol, vol);
}

//other functions
void checkTrackFinished() {
  if (playing &&  MP3player.isPlaying() == 0) {
    delay(interTrackGap);
    nextTrack();
  }
}

void flash(int ms) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ms);
  digitalWrite(LED_BUILTIN, LOW);
}

int getFinalTrack() {
  int i = 0;
  while (true) {
    if (! sd.vwd()->exists(filename(i)))
      return i - 1;
    i++;
  }
}

char * filename(int i) {
  static char fn[13];
  sprintf(fn,"TRACK%03d.mp3",i);
  return fn;
}
