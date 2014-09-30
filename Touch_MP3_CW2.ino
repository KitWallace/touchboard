/*******************************************************************************

 Bare Conductive Touch MP3 player
 ------------------------------

 Touch_MP3.ino - touch triggered MP3 playback

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
     - 
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
int lastPlayed = -1;
int vol = 30;

// touch behaviour definitions
#define firstPin 0
#define lastPin 9
#define volDown 10
#define volUp 11
#define volStep 5
// sd card instantiation
SdFat sd;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

void setup() {
  Serial.begin(57600);

  pinMode(LED_BUILTIN, OUTPUT);

  //while (!Serial) ; {} //uncomment when using the serial monitor
  Serial.println("Bare Conductive Touch MP3 player");

  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  result = MP3player.begin();
  MP3player.setVolume(vol, vol);

  if (result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }

}

void loop() {
  if (MPR121.touchStatusChanged())
    readTouchInputs();
  else if (lastPlayed > -1 &&  MP3player.isPlaying() == 0)
    nextTrack();
}


void flash(int ms) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ms);
  digitalWrite(LED_BUILTIN, LOW);
}

void startTrack (int i) {
  MP3player.playTrack(i);
  Serial.print("playing track ");
  Serial.println(i);
  // don't forget to update lastPlayed - without it we don't
  // have a history
  lastPlayed = i;
}

void stopTrack() {
  MP3player.stopTrack();
  Serial.print("stopping track ");
  Serial.println(lastPlayed);
  // remember that no track is playing
  lastPlayed = -1;
}

void nextTrack() {
  int i = (lastPlayed + 1 );
  if (i > lastPin) {
    i = firstPin;
  }
  startTrack(i);
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

void readTouchInputs() {
  MPR121.updateTouchData();
  if (MPR121.getNumTouches() == 1) {
    // only make an action if we have one pin touched
    // ignore multiple touches
    if (MPR121.isNewTouch(volUp))
      upVolume();
    else if (MPR121.isNewTouch(volDown))
      downVolume();
    else
      for (int i = firstPin; i <= lastPin; i++) { // Check which electrodes were pressed
        if (MPR121.isNewTouch(i)) {
          //pin i was just touched
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" was just touched");
          flash(200);
          if (MP3player.isPlaying()) {
            if (lastPlayed == i) {
              // if we're already playing the requested track, stop it
              stopTrack();
            } else {
              // if we're already playing a different track, stop that
              // one and play the newly requested one
              stopTrack();
              startTrack(i);
            }
          }
          else {
            // if we're playing nothing, play the requested track
            startTrack(i);
          }
        }
      }
  }
}




