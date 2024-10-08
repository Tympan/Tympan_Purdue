
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"
#include "Measurement.h"
#include "TestController.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern State myState;
extern TestController testController;
extern Measurement inputMeasurement;

//Extern Functions (that live in a file other than this file here)
extern void setConfiguration(int);
extern float setInputGain_dB(float val_dB);
extern int setOutputChan(int chan);
extern bool enablePrintMemoryAndCPU(bool _enable);
extern void printInputConfiguration(void);
extern void printOutputChannel(void);
extern float setCalcLevelTimeWindow(float time_window_sec);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void) : SerialManagerBase() {};

    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    float inputGainIncrement_dB = 5.0;  //changes the input gain of the AIC
    float frequencyIncrement_factor = pow(2.0,1.0/3.0);  //third octave steps
    float amplitudeIcrement_dB = 1.0;  //changes the amplitude of the synthetic sine wave
  private:
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("General: No Prefix");
  Serial.println("  h:     Print this help");
  Serial.print(  "  w/W/E: Input: Use PCB mics (w), jack as mic (W), jack as line-in (e) (current = ");  printInputConfiguration(); Serial.println(")");
  Serial.println("  i/I:   Input: Increase or decrease input gain (current = " + String(myState.input_gain_dB,1) + " dB)");
  Serial.println("  f/F:   Sine: Increase or decrease steady-tone frequency (current = " + String(testController.getFrequency_Hz(),1) + " Hz)");
  Serial.println("  a/A:   Sine: Increase or decrease sine amplitude (current = " + String(20*log10(testController.getAmplitude())-3.0,1) + " dB re: output FS = " + String(testController.getAmplitude(),3) + " amplitude)");
  Serial.print(  "  1/2/3: Sine: Output to left (1), right (2), or both (3) (current = "); printOutputChannel(); Serial.println(")");
  Serial.println("  q  :   TestMode: Reset all test parameters to the defaults.");
  Serial.print(  "  m/t/T: TestMode: Switch between muted (m), steady tone (t) or stepped-tone (T) modes (current = "); testController.printTestToneMode(); Serial.println(")");
  Serial.println("  d/D:   TestMode: Increase or decrease stepped-tone duration (current = " + String(testController.stepped_test_step_dur_sec,2) + " sec)");
  Serial.println("  v  :   TestMode: Print all resuls from stepped-tone test");
  Serial.print(  "  p/P:   Printing: start/Stop printing the current input signal levels"); if (myState.flag_printInputLevelToUSB)   {Serial.println(" (active)");} else { Serial.println(" (off)"); }
  Serial.print(  "  o/O:   Printing: start/Stop printing the current output signal levels"); if (myState.flag_printOutputLevelToUSB)   {Serial.println(" (active)");} else { Serial.println(" (off)"); }
  Serial.println("  r/s:   SD: Start recording (r) or stop (s) audio to SD card");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'c':
      Serial.println("SerialManager: enabling printing of memory and CPU usage.");
      myState.enable_printCpuToUSB = true;
      break;
    case 'C':
      Serial.println("SerialManager: disabling printing of memory and CPU usage.");
      myState.enable_printCpuToUSB = false;
      break;
    case 'w':
      Serial.println("SerialManager: Switch input to PCB Mics");
      setConfiguration(State::INPUT_PCBMICS);
      break;
    case 'W':
      Serial.println("SerialManager: Switch input to Mics on Jack.");
      setConfiguration(State::INPUT_JACK_MIC);
      break;
    case 'e':
      myTympan.println("SerialManager: Switch input to Line-In on Jack");
      setConfiguration(State::INPUT_JACK_LINE);
      break;
    case 'i':
      setInputGain_dB(myState.input_gain_dB + inputGainIncrement_dB);
      Serial.println("SerialManager: Increased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'I':   //which is "shift i"
      setInputGain_dB(myState.input_gain_dB - inputGainIncrement_dB);
      Serial.println("SerialManager: Decreased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'f':
      testController.setFrequency_Hz(max(125.0/4,min(16000.0, testController.getFrequency_Hz()*frequencyIncrement_factor)));  //octave-based incrementing. Limit freuqencies to 31.25 Hz -> 16 kHz
      Serial.println("SerialManager: increased tone frequency to " + String(testController.getFrequency_Hz()));
      break;
    case 'F':
      testController.setFrequency_Hz(max(125.0/4,min(16000.0, testController.getFrequency_Hz()/frequencyIncrement_factor)));  //octave-based incrementing. Limit freuqencies to 31.25 Hz -> 16 kHz
      Serial.println("SerialManager: decreased tone frequency to " + String(testController.getFrequency_Hz()));
      break;
    case 'a':
      testController.setAmplitude(testController.getAmplitude()*sqrt(pow(10.0,0.1*amplitudeIcrement_dB))); //converting dB back to linear units
      Serial.println("SerialManager: increased tone amplitude to " + String(20.f*log10f(testController.getAmplitude())-3.0,1) + " dB re: FS (output)");
      break;
    case 'A':
      testController.setAmplitude(testController.getAmplitude()/sqrt(pow(10.0,0.1*amplitudeIcrement_dB))); //converting dB back to linear units
      Serial.println("SerialManager: decreased tone amplitude to " + String(20.f*log10f(testController.getAmplitude())-3.0,1) + " dB re: FS (output)");
      break;
    case '1':
      Serial.println("SerialManager: outputing sine to left only");
      setOutputChan(State::OUT_LEFT);
      break;
    case '2':
      Serial.println("SerialManager: outputing sine to right only");
      setOutputChan(State::OUT_RIGHT);
      break;
    case '3':
      Serial.println("SerialManager: outputing sine to both left and right");
      setOutputChan(State::OUT_BOTH);
      break;
    case 'q':
      Serial.println("SerialManager: reseting test controller to its defaults...");
      testController.resetToDefaults();
      inputMeasurement.clearAllMeasurements(); Serial.println("SerialManager: clearing any previous input measurements.");
      break;
    case 'm':
      testController.switchTestToneMode(TestController::TEST_MODE_MUTE);
      break;
    case 't':
      testController.switchTestToneMode(TestController::TEST_MODE_STEADY);
      break;
    case 'T':
      inputMeasurement.clearAllMeasurements(); Serial.println("SerialManager: clearing any previous input measurements.");
      testController.switchTestToneMode(TestController::TEST_MODE_STEPPED_FREQUENCY);
      break;
    case 'd':
      testController.stepped_test_step_dur_sec = max(0.05,testController.stepped_test_step_dur_sec + 0.1);
      Serial.println("SerialManager: increased step duration to " + String(testController.stepped_test_step_dur_sec) + " sec");
      if (myState.calcLevel_timeWindow_sec < myState.target_calcLevel_timeWindow_sec) {
        //averaging window is shorter than we'd like given how short the test tones are.  Could we lengthen the window?
        float candidate_timeWindow_sec = 2.0*testController.stepped_test_step_dur_sec;
        if (candidate_timeWindow_sec> myState.calcLevel_timeWindow_sec) {
          //yes!  we can lengthen the averaging window.
          myState.calcLevel_timeWindow_sec=setCalcLevelTimeWindow(max(myState.calcLevel_timeWindow_sec,myState.target_calcLevel_timeWindow_sec));
          Serial.println("SerialManager: changing averaging window to " + String(myState.calcLevel_timeWindow_sec,3) + " sec");
        }
      }
      break;
    case 'D':
      testController.stepped_test_step_dur_sec = max(0.1,testController.stepped_test_step_dur_sec - 0.1);
      Serial.println("SerialManager: decreased step duration to " + String(testController.stepped_test_step_dur_sec) + " sec");
      {
        //having shortened the test tone, is our averaging window too long?
        float maxAllowedTimeWindow_sec = max(0.05, 0.5*testController.stepped_test_step_dur_sec);
        //Serial.println("SerialManager: shorten duration, maxAllowedTimeWindow_sec = " + String(maxAllowedTimeWindow_sec,4) + ", myState.calcLevel_timeWindow_sec = " + String(myState.calcLevel_timeWindow_sec,4));
        if (myState.calcLevel_timeWindow_sec > maxAllowedTimeWindow_sec) {
          //yes, our averaging is too long.  Make it shorter.
          myState.calcLevel_timeWindow_sec=setCalcLevelTimeWindow(maxAllowedTimeWindow_sec);
          Serial.println("SerialManager: changing averaging window to " + String(myState.calcLevel_timeWindow_sec,3) + " sec");         
        }
      }
      break;
    case 'v':
      inputMeasurement.printAllMeasurements();
      break;
    case 'p':
      myState.flag_printInputLevelToUSB = true;
      Serial.println("SerialManager: enabled printing of the input signal levels");
      break;
    case 'P':
      myState.flag_printInputLevelToUSB = false;
      Serial.println("SerialManager: disabled printing of the input signal levels");
      break;
    case 'o':
      myState.flag_printOutputLevelToUSB = true;
      Serial.println("SerialManager: enabled printing of the output signal level");
      break;
    case 'O':
      myState.flag_printOutputLevelToUSB = false;
      Serial.println("SerialManager: disabled printing of the output signal level");
      break;
    case 'r':
      Serial.println("SerialManager: starting recording of input sginals to the SD card...");
      audioSDWriter.startRecording();
      break;
    case 's':
      Serial.println("SerialManager: stopping recording of input signals to the SD card...");
      audioSDWriter.stopRecording();
      break;
    default:
      Serial.println("SerialManager: command " + String(c) + " not recognized");
      break;
  }
  return ret_val;
}


#endif
