
#ifndef _TestController_h
#define _TestController_h

//header files
#include "Measurement.h"
#include <vector>

#define TEST_CONTROLLER_DEFAULT_current_test_mode         TEST_MODE_MUTE
#define TEST_CONTROLLER_DEFAULT_stepped_test_step_dur_sec 0.5 //Lowest speed 
#define TEST_CONTROLLER_DEFAULT_default_sine_freq_Hz      1000.0
#define TEST_CONTROLLER_DEFAULT_default_sine_amplitude    (sqrt(2.0)*sqrt(pow(10.0,0.1*-20.0)))    //(-20dBFS converted to linear and then converted from RMS to amplitude)

class TestController {
  public:
    TestController(AudioSynthWaveform_F32 *sine, Measurement *measurement) : sineWave(sine), inputMeasurement(measurement) { resetToDefaults(); };

    void resetToDefaults(void) {
      Serial.println("TestController: reseting to default settings for stepped tone test");
      switchTestToneMode(TEST_MODE_MUTE);
      stepped_test_step_dur_sec = TEST_CONTROLLER_DEFAULT_stepped_test_step_dur_sec;
      setFrequency_Hz(TEST_CONTROLLER_DEFAULT_default_sine_freq_Hz);
      setAmplitude(TEST_CONTROLLER_DEFAULT_default_sine_amplitude);
      current_step = -1;
      switchTestToneMode(TEST_CONTROLLER_DEFAULT_current_test_mode);
    }

    // set the frequency of the stepped tone (and note when the tone was changed)
    float setSteppedTone(float freq_Hz) {
      //Serial.println("TestToneController: setSteppedTone: step = " + String(current_step) + ", frequency = " + String(freq_Hz) + " Hz");
      setFrequency_Hz(freq_Hz);                    //setFrequency is in "AudioProcessing.h"
      setAmplitude(default_sine_amplitude);  //setAmplitude is in "AudioProcessing.h"
      stepped_test_next_change_millis = millis() + (unsigned long)(1000.0*stepped_test_step_dur_sec);
      return getFrequency_Hz();
    }

    //return a negative frequency if we're at an invalid step in the test
    float getToneFrequencyForCurrentStep(void) {
      //check what step were on in our test protocol
      if ((current_step < 0) || (current_step >= number_steps)) return -1.0;  // invalid test step.  return early.

      //compute the frequency, including for edge cases
      if (number_steps <= 0) {
        //invalid request
        Serial.println("getToneFrequencyForCurrentStep: *** ERROR ***: number of steps must be > 0.  Returning.");
        return -1.0;
      } else if (number_steps==1) {
        //weird request
        return stepped_test_start_freq_Hz;
      } else {
        //normal request
        float step_factor = log( stepped_test_end_freq_Hz/stepped_test_start_freq_Hz ) / ((float)(number_steps-1));
        float freq_Hz = stepped_test_start_freq_Hz * exp( step_factor * ( (float)current_step) );
        return freq_Hz;
      }

      //we should never get here, so if we do, return an error
      return -1.0;
    }


    //return true when incrementing past the last step in the test
    bool incrementToNextStep(void) {
      current_step = current_step + 1;
      if (current_step >= number_steps) {
        //we're now incrementing past our last step, so the test is done!
        current_step = -1; //indicates an invalid test step (ie, test should be stopped)
        return true;  //return true, indicating tha the test is complete
      }

      //test not yet complete.  change the frequency
      setSteppedTone(getToneFrequencyForCurrentStep());
      if (inputMeasurement != nullptr) inputMeasurement->resetLevelCalculators();  //clear the averaging so that we're starting fresh for this new frequency
      return false; //this is the normal return path
    }

    int switchTestToneMode(int new_mode) {
      switch (new_mode) {
        case TEST_MODE_MUTE:
          current_test_mode = new_mode;
          Serial.print("TestToneController: switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
          setFrequency_Hz(default_sine_freq_Hz);  //setFrequency is in "AudioProcessing.h"
          setAmplitude(0.0);  //mute   (setAmplitude is in "AudioProcessing.h")
          break;
        case TEST_MODE_STEADY:
          current_test_mode = new_mode;
          Serial.print("TestToneController: switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
          setFrequency_Hz(default_sine_freq_Hz);  //setFrequency is in "AudioProcessing.h"
          setAmplitude(default_sine_amplitude);   //setAmplitude is in "AudioProcessing.h"
          break;
        case TEST_MODE_STEPPED_FREQUENCY:
          current_test_mode = new_mode;
          current_step = -1;
          Serial.print("TestToneController: switchTestToneMode: Switching to "); printTestToneMode(); Serial.println();
          incrementToNextStep();
          break;   
        default:
          Serial.println("TestToneController: switchTestToneMode: *** WARNING ***: mode = " + String(new_mode) + " not recognized.  Ignoring.");
          break;
      }
      return current_test_mode;
    }


    void serviceSteppedToneTest(unsigned long current_millis) {
      if (current_test_mode != TEST_MODE_STEPPED_FREQUENCY) return;
      if (current_millis >= stepped_test_next_change_millis) {
      
        //we're about to make a change, so print the current levels
        if (inputMeasurement != nullptr) inputMeasurement->takeMeasurement(current_step, getFrequency_Hz());
        
        //go to the next test tone frequency
        bool is_done = incrementToNextStep();
    
        //are we done?
        if (is_done) {
          Serial.println("TestToneController: serviceStppedToneTest: stepped-tone test completed!");
          switchTestToneMode(TEST_MODE_MUTE); //mutes the tone and sets the freuqency to a default value
        } 
      }
    }

    void printTestToneMode(void) {
      switch (current_test_mode) {
        case TEST_MODE_MUTE:
          Serial.print("Muted"); break;
        case TEST_MODE_STEADY:
          Serial.print("Steady Tone"); break;
        case TEST_MODE_STEPPED_FREQUENCY:
          Serial.print("Stepped Tones"); break;
      }
    }

    float setFrequency_Hz(const float freq_Hz) { return sineWave->setFrequency_Hz(constrain(freq_Hz, 125.0/8, 20000.0)); }  //constrain the frequency
    float getFrequency_Hz(void) { return sineWave->getFrequency_Hz(); }
    float setAmplitude(const float amplitude) { return sineWave->setAmplitude(constrain(amplitude, 0.0, 1.0)); } //constrain the amplitude
    float getAmplitude(void) { return sineWave->getAmplitude(); }

    //data members
    enum Test_Mode { TEST_MODE_MUTE, TEST_MODE_STEADY, TEST_MODE_STEPPED_FREQUENCY};  //different test modes allowed here

    //user settings
    const float stepped_test_start_freq_Hz = 20.0;    //starting frequency for stepped tones
    const float stepped_test_end_freq_Hz = 20000.0;   //ending frequency for stepped tones
    const int number_steps = 501;                     //how many steps should we use to span the range of frequencies?
    int current_test_mode = TEST_CONTROLLER_DEFAULT_current_test_mode;   //what test mode should we default to upon startup?
    float stepped_test_step_dur_sec = TEST_CONTROLLER_DEFAULT_stepped_test_step_dur_sec;      //duration at each step
    float default_sine_freq_Hz   = TEST_CONTROLLER_DEFAULT_default_sine_freq_Hz;      //used whenever switching the TEST_MODE
    float default_sine_amplitude = TEST_CONTROLLER_DEFAULT_default_sine_amplitude;   


  private:
    AudioSynthWaveform_F32 *sineWave = nullptr;
    Measurement *inputMeasurement = nullptr;
    int current_step = -1;                                    //which step are we in the stepped test?
    unsigned long stepped_test_next_change_millis = 0UL;      //when to switch to the next step

};

#endif