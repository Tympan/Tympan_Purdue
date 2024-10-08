/*
  DPOAE_Tones_Record
  Chip Audette, OpenAudio, Oct 2024

  Plays two tones for doing DPOAE Test.
    * Includes several preset combination of tone frequencies
    * Can manually step through the presets or can automatically step through
  Records audio line-in (as if from mic from DPOAE probe) to SD card.
  Control via BT App.

  MIT License, Use at your own risk.
*/

#include <Tympan_Library.h>
#include "DPOAE_Settings_Manager.h"
#include "Tone_Manager.h"
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ;  //choose your sample rate (up to 96000)
const int audio_block_samples = 128;     //do not make bigger than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// Create the audio library objects that we'll use
Tympan                    myTympan(TympanRev::E, audio_settings);           //use TympanRev::D or E or F
AudioInputI2S_F32         audio_in(audio_settings);                         //from the Tympan_Library
AudioSDWriter_F32_UI      audioSDWriter(audio_settings);                    //record audio to SD card.  This is stereo by default
AudioSynthWaveform_F32    sine1(audio_settings),sine2(audio_settings);      //from the Tympan_Library...for generating tones
AudioEffectFade_F32       fade1(audio_settings), fade2(audio_settings);     //For smoohting start/stop of the tones
AudioFilterBiquad_F32     highpass1(audio_settings), highpass2(audio_settings);     //for limiting bandwidth prior to measuring loudness
AudioFilterBiquad_F32     lowpass1(audio_settings), lowpass2(audio_settings);       //for limiting bandwidth prior to measuring loudness
AudioCalcLeq_F32          measureLEQ1(audio_settings), measureLEQ2(audio_settings); //for measuring loudness
AudioOutputI2S_F32        audio_out(audio_settings);   //from the Tympan_Library

// Create the audio connections from the sine1 object to the audio output object
AudioConnection_F32     patchCord10(sine1, 0, fade1, 0);  //connect to left output
AudioConnection_F32     patchCord11(sine2, 0, fade2, 0);  //connect to right output
AudioConnection_F32     patchCord12(fade1, 0, audio_out, 0);  //connect to left output
AudioConnection_F32     patchCord13(fade2, 0, audio_out, 1);  //connect to right output
AudioConnection_F32     patchcord20(audio_in, 0, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32     patchcord21(audio_in, 1, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer
AudioConnection_F32     patchcord30(audio_in, 0, highpass1, 0);   //connect Raw audio to a highpass filter
AudioConnection_F32     patchcord31(audio_in, 1, highpass2, 0);   //connect Raw audio to a highpass filter
AudioConnection_F32     patchcord32(highpass1, 0, lowpass1, 0);   //more filtering
AudioConnection_F32     patchcord33(highpass2, 0, lowpass2, 0);   //more filtering
AudioConnection_F32     patchcord34(lowpass1, 0, measureLEQ1, 0);   //filtered audio to level measurement
AudioConnection_F32     patchcord35(lowpass2, 0, measureLEQ2, 0);   //filtered audio to level measurement

// Create classes for controlling the system
#include   "SerialManager.h"
#include   "State.h"                            
BLE_UI& ble = myTympan.getBLE_UI();      //myTympan owns the ble object, but we grab a reference to it here
SerialManager   serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State           myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  //serialManager.add_UI_element(&myState);
  //serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}
          
//create manager for the DPOAE protocol and for the test tones
DPOAE_Settings_Manager DPOAE_manager(&myState.test_params);
Tone_Manager tone_manager(&sine1, &sine2, sample_rate_Hz);
const int sd_start_millis = 2000; //dead period after starting SD recording prior to tones starting
const int tone_dur_millis = 3000; //duration of tone
const int silence_dur_millis = 1000; //duration of silence between tones
const float fade_msec = 50.0; //length of fade in and fade out of tones
#include "DPOAE_test_logic.h"

//settings for level measurement
float hp_Hz = 100.0;     //cutoff for highpass filter
float lp_Hz = 10000.0;   //cutoff for lowpass filter
float LEQ_ave_sec = 0.5; //averaging time
void setupLevelMeasurements(void) {
  highpass1.setHighpass(0,hp_Hz);  highpass2.setHighpass(0,hp_Hz);
  lowpass1.setLowpass(0,lp_Hz);  lowpass2.setLowpass(0,lp_Hz);
  measureLEQ1.setTimeWindow_sec(LEQ_ave_sec);measureLEQ2.setTimeWindow_sec(LEQ_ave_sec);
}

void setConfiguration(int config) {
 
  switch (config) {
    case State::INPUT_PCBMICS:
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      break;

    case State::INPUT_JACK_MIC:
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      break;
   
    case State::INPUT_JACK_LINE:
      Serial.println("setConfiguration: changing to INPUT JACK as LINE-IN...");
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      break;
      
    default:
        return;
  }
  myState.input_source = config;
}


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
  setConfiguration(myState.input_source);

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
  setupLevelMeasurements();
  
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

  //service the BLE advertising state...if not recording to SD
  if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
    ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
  }

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(audio_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //periodically print the CPU and Memory Usage
  if (myState.printCPUtoGUI) { myTympan.printCPUandMemory(millis(),3000); serviceUpdateCPUtoGUI(millis(),3000);}      //print every 3000 msec

  //service the state of the test
  serviceSteppedTest(millis());  //see DPOAE_test_logic.h

  //service the level measurements
  if (myState.printLevelsToGUI) serviceLevelMeasurements(millis(),1000);   //update every 1000msec

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
 
