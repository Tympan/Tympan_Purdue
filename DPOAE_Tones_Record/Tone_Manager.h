/*
 Tone_Manager.h

 Created: Chip Audette, Jan 2023
 Purpose: Control the two sine wave tones used in the DPOAE test
 
 This class controls the frequencies and amplitudes of the two tones

 MIT License, Use at your own risk.
*/

#ifndef _Tone_Manager_h
#define _Tone_Manager_h

class Tone_State {
  public:
    Tone_State(void) {};
    float freq1_Hz = 0;
    float freq2_Hz = 0;
    float amp1_dBFS = -60.0f;
    float amp2_dBFS = -60.0f;
    bool is_muted = false;
};

class Tone_Manager {
  public:
    Tone_Manager(AudioSynthWaveform_F32 *_f1, AudioSynthWaveform_F32 *_f2, float fs_Hz) : 
                f1_tone(_f1), f2_tone(_f2), sample_rate_Hz(fs_Hz) {};

    void setTones(const Tone_State &tone_state) {
      if (tone_state.is_muted) {
        f1_tone->amplitude(0.0); f2_tone->amplitude(0.0);
      }
      
      //set the sine wave parameters                               
      f1_tone->frequency(tone_state.freq1_Hz);   
      f2_tone->frequency(tone_state.freq2_Hz);

       if (!tone_state.is_muted) {
          f1_tone->amplitude(dB_to_amp(tone_state.amp1_dBFS));
          f2_tone->amplitude(dB_to_amp(tone_state.amp2_dBFS));   
      }     
    }

    //utility functions
    float dB_to_amp(float val_dB) { return sqrtf(powf(10.0, val_dB/10.0)); }
    void printFrequencyValues() { 
        Serial.println("Tone_Manager: f1 = " + String(f1_tone->getFrequency_Hz()) 
                          + "Hz, f2 = " + String(f2_tone->getFrequency_Hz()) + "Hz"); 
    }    
  private:
    AudioSynthWaveform_F32 *f1_tone, *f2_tone;
    float sample_rate_Hz = 48000;
};

#endif
