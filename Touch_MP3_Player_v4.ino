
/*******************************************************************************

 Bare Conductive Touch MP3 player
 ------------------------------

 Touch_MP3_player.ino - touch triggered MP3 playback


 This version implements the normal controls on an MP3 Player via touch pins:

  ###### 

 The player plays TRACKnnn.mp3 files with consecutive track numbers nnn in range 0 to 999

 In the absence of input, the player plays one track after another

 The SD card is read at FULL speed rather than half speed in an attempt to reduce jitter

 Chris Wallace  4 Oct 2014
 
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
int currentTrack = -1;
int currentAlbum = 0;
bool playing = false;
bool paused = false;
int vol = 30;
int trackCount; 
int albumCount;
#define interTrackGap 500
#define volStep 5

// touch behaviour definitions
#define controlPin 11
#define nextTrackPin 0
#define prevTrackPin 2
#define nextAlbumPin 4
#define prevAlbumPin 6
#define pausePin 7

#define volDownPin 8
#define volUpPin 9

// sd card instantiation
SdFat sd;
SdFile sdfile;

// define LED_BUILTIN for older versions of Arduino
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(57600); 
/*  include if using the Serial connection
 while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
*/
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
  albumCount = getAlbumCount();
  Serial.print("Number of Albums ");
  Serial.println(albumCount);
  openAlbum(currentAlbum);
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
  if ( MPR121.getTouchData(controlPin)) {
    switch (pinTouched()) {
      case nextTrackPin:
        nextTrack(); break;
      case prevTrackPin:
        prevTrack(); break;
      case nextAlbumPin:
        nextAlbum(); break;
      case prevAlbumPin:
        prevAlbum(); break;
      case pausePin:
         if (paused) {
             resumeTrack(); 
             paused= false;
         }
         else {
             pauseTrack();
             paused=true;
         }
         break;
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
  while (i < 11) {
    if (MPR121.isNewTouch(i))
      return i;
    i++;
  }
  return -1;
}


//  action functions

void startTrack (int i) {
  if (playing)
    stopTrack();
  char * name = filename(i);
  MP3player.playMP3(name);
  Serial.print("playing ");
  Serial.println(name);
  flash(200);
  currentTrack = i;
  playing = true;
}

void stopTrack() {
  flash(100);
  MP3player.stopTrack();
  Serial.print("stopping ");
  Serial.println(filename(currentTrack));
  playing = false;
}

void openAlbum(int j) {
   
  if (playing) stopTrack();
  char * name = albumname(j);
  sd.chdir("/", true);
  sd.chdir(name,true);
  trackCount  = getTrackCount();
  currentAlbum=j;
  Serial.print("Opened ");
  Serial.println(albumname(currentAlbum));
  Serial.print("Number of Tracks ");
  Serial.println(trackCount);
  flash(200);
  startTrack(0);
}

void nextAlbum() {
  int j = mod(currentAlbum + 1,albumCount);
  openAlbum(j);
}

void prevAlbum() {
  int j = mod(currentAlbum - 1,albumCount);
  openAlbum(j);
}

void nextTrack() {
  int i = mod(currentTrack + 1,trackCount);
  startTrack(i);
}

void prevTrack() {
  int i = mod(currentTrack - 1,trackCount);
  startTrack(i);
}

void pauseTrack() {
  MP3player.pauseMusic();
  Serial.print("paused track ");
  Serial.println(filename(currentTrack));
  playing = false;
}

void resumeTrack() {
  MP3player.resumeMusic();
  flash(50);
  Serial.print("resuming track ");
  Serial.println(filename(currentTrack));
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
  if (playing && !paused && MP3player.isPlaying() == 0) {
    delay(interTrackGap);
    nextTrack();
  }
}

void flash(int ms) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(ms);
  digitalWrite(LED_BUILTIN, LOW);
}

int getTrackCount() {
  int i = 0;
  while (true) {
    if (! sd.vwd()->exists(filename(i)))
      return i;
    i++;
  }
}
int getAlbumCount() {
  int i = 0;
  while (true) {
    if (! sd.vwd()->exists(albumname(i)))
      return i;
    i++;
  }
}

char * filename(int i) {
  static char fn[13];
  sprintf(fn,"TRACK%03d.mp3",i);
  return fn;
}

char * albumname(int i) {
  static char fn[13];
  sprintf(fn,"ALBUM%03d",i);
  return fn;
}

int mod(int i,int n) {
  return (i + n) % n;
}
