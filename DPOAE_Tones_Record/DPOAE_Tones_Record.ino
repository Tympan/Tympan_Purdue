/*
  DPOAE_Tones_Record
  Chip Audette, OpenAudio, Oct 2024

  Plays two tones for doing DPOAE Test.
    * Includes several preset combination of tone frequencies
    * Can manually step through the presets or can automatically step through
  Records audio line-in (as if from mic from DPOAE probe) to SD card.
  Control via BT App.

  This program has been expanded to allow for the Tympan to also make its SD card
  visible from your PC/Mac.  This support is very experimental and has several
  known issues.  But, it still can be much better than having to repeated remove
  and re-insert the SD card.

  The SD Reader behavior is called "MTP".  To enable MTP, you must tell the Arduino IDE
  to compile your code with different settings.  To see these settings and to see
  troubleshooting tips, see the comments at the top of the file "setup_MTP.h" that is
  here in this sketch.

  MTP Support is VERY EXPERIMENTAL!!  There are weird behaviors that come with the underlying
  MTP support provided by Teensy and its libraries.  

  MIT License, Use at your own risk.
*/

#include <Tympan_Library.h>   //requires V3.1.0 or later
#include "DPOAE_Settings_Manager.h"
#include "Tone_Manager.h"
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ;  //choose your sample rate (up to 96000)
const int audio_block_samples = 128;     //do not make bigger than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Create the audio library objects that we'll use
Tympan    myTympan(TympanRev::E, audio_settings);           //use TympanRev::D or E or F
SdFs      sd;                 //here is the sd card object, to be shared about AudioSDWriter and SDtoSerial
#include "AudioProcessing.h"  //here is where most of the audio stuff is created


// Create classes for controlling the system
#include   "SerialManager.h"
#include   "State.h"                            
BLE_UI& ble = myTympan.getBLE_UI();      //myTympan owns the ble object, but we grab a reference to it here
SerialManager   serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State           myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI
SDtoSerial      SD_to_serial(&sd, &Serial);  //transfers raw bytes of files on the sd over to Serial (part of Tympan Library)

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  //serialManager.add_UI_element(&myState);
  //serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}

/* Create the MTP servicing stuff so that one can access the SD card via USB */
/* If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental) */
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated

//create manager for the DPOAE protocol and for the test tones
DPOAE_Settings_Manager DPOAE_manager(&myState.test_params);
Tone_Manager tone_manager(&sine1, &sine2, sample_rate_Hz);
#include "DPOAE_test_logic.h"


// define setup()...this is run once when the hardware starts up
void setup(void)
{
  //Open serial link for debugging
  myTympan.beginBothSerial(); delay(300);      //both Serial (USB) and Serial1 (Bluetooth) are started here
  if (!Serial && (millis() < 2000)) delay(10); //stall to see if the USB serial comes alive
  myTympan.println("DPOAE_Tones_Record: starting setup()...");

  //allocate the audio memory
  AudioMemory_F32(100,audio_settings); //I can only seem to allocate 400 blocks
  
  //mute the sine waves
  muteOutput(true);  //true means to mute (false would tell it to unmute)
  
  //start the audio hardware
  myTympan.enable();

  //Choose the desired input
  setConfiguration(myState.input_source);  //see AudioProcessing.h

  //Set the desired volume levels
  myTympan.volume_dB(myState.output_gain_dB);          // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(myState.input_gain_dB);     // set input volume, 0-47.5dB in 0.5dB setps

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //Prime the tone generation system
  myState.max_step_ind = myState.test_params.n_freqs; 
  jumpToFreqStepAndPlayTones(0);  //start at step 0 (ie, start at the first step in the protocol)

  //setup level measurements
  setupLevelMeasurements();   //see AudioProcessing.h
  
  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define loop()...this is run over-and-over while the device is powered
void loop(void)
{
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(audio_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    //service the BLE advertising state...if not recording to SD
    if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
      ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    }

    //periodically print the CPU and Memory Usage
    if (myState.printCPUtoGUI) { myTympan.printCPUandMemory(millis(),3000); serviceUpdateCPUtoGUI(millis(),3000);}      //print every 3000 msec

    //service the state of the test
    serviceSteppedTest(millis());  //see DPOAE_test_logic.h

    //service the level measurements
    if (myState.printLevelsToGUI) serviceLevelMeasurements(millis(),1000);   //update every 1000msec
  }

}  //end loop()


// ///////////////// Servicing routines

//Test to see if enough time has passed to send up updated CPU value to the App
void serviceUpdateCPUtoGUI(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    //send the latest value to the GUI!
    serialManager.updateCpuDisplayUsage();
    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end serviceUpdateCPUtoGUI();


//void serviceSteppedTest(millis());  //see DPOAE_test_logic.h

void serviceLevelMeasurements(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    //send the latest value to the GUI!
    myState.measuredLEQ_dB[0] = measureLEQ1.getCurrentLevel_dB();
    myState.measuredLEQ_dB[1] = measureLEQ2.getCurrentLevel_dB();
    serialManager.updateLevelDisplays();
    
    lastUpdate_millis = curTime_millis;
  } 
} 

// ///////////////// functions used to respond to the commands

void start_DPOAE_test(void) {
  myState.cur_test_state = State::TEST_STARTING;
}

void stop_DPOAE_test(void) {
  myState.cur_test_state = State::TEST_STOPPING;     
}

//change the gain from the App
float changeCal(int chan, float change_in_cal_dB) {
  float new_val = DPOAE_manager.incrementCal_dBFS(chan, myState.cur_step_ind, change_in_cal_dB, &myState.tone_state);
  jumpToFreqStepAndPlayTones(myState.cur_step_ind);  //play the tones at the new settings
  return new_val;
}

//Print gain levels 
void printGainLevels(void) {
  Serial.print("Analog Input Gain (dB) = "); 
  Serial.println(myState.input_gain_dB); //print text to Serial port for debugging
  Serial.println("Overall Output (dB SPL): F1 = " + String(myState.test_params.targ_f1_dBSPL) + " dBFS" 
                +", F2 = " + String(myState.test_params.targ_f2_dBSPL) + "dBFS");
  Serial.println("Per-Frequency Cal (dBFS at 94dB SPL) = "); 
  int n = myState.test_params.n_freqs;
  float *f2_Hz = myState.test_params.targ_freq2_Hz;
  float *amp1_dB = myState.test_params.cal_f1_dBFS_at_94dBSPL;
  float *amp2_dB = myState.test_params.cal_f2_dBFS_at_94dBSPL;
  for (int i=0; i<n; i++) {
    Serial.println("    F2 = " + String(f2_Hz[i],0) + " Hz, " 
                     + "F1 gain = " + String(amp1_dB[i],1) + " dB, "
                     + "F2 gain = " + String(amp2_dB[i],1) + " dB");
  }
  //Serial.println(myState.digital_gain_dB); //print text to Serial port for debugging
}

int incrementFreqStep(int ind) {
  return jumpToFreqStepAndPlayTones(myState.cur_step_ind + ind);
}

int jumpToFreqStepAndPlayTones(int ind) {
  myState.cur_step_ind = DPOAE_manager.testStep(ind,&(myState.tone_state));  //start test at step zero. output is through tone_state
  tone_manager.setTones(myState.tone_state);      //play the tones selected by the DPOAE manager
  tone_manager.printFrequencyValues();
  return myState.cur_step_ind;
}

bool muteOutput(bool please_mute) {
  myState.tone_state.is_muted = please_mute;      //tell myState.tone_state whether we want to be muted (or not)
  tone_manager.setTones(myState.tone_state);      //here's where we actually set the frequency and amplitudes to the values in myState.tone_state
  return myState.tone_state.is_muted;             //return the value of whether we are muted or not
}

bool enablePrintLevelsToGUI(bool please_print) { 
  return myState.printLevelsToGUI = please_print; 
}
 
