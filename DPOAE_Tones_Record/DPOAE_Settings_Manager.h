/*
 DPOAE_Settings_Manager.h

 Created: Chip Audette, Jan 2023
 Purpose: Choose what frequencies and amplitudes to use for each step
          in the DPOAE test.
 
 This class doesn't generate the ones; it simply decides what settings
     (frequency and amplitude) the tones should have.  The user must pass
     these settings to the tones themselves.

 MIT License, Use at your own risk.
*/

#ifndef _DPOAE_Settings_Manager_h
#define _DPOAE_Settings_Manager_h

#include "Tone_Manager.h"


//define DPOAE frequencies to be tested
/**********************************************************************************
F2_desired_Hz=[1000;1500;2000;3000;4000;6000;8000];
F1_desired_Hz= F2_desired_Hz/1.22;
FS=41667;   %Hz
BlockSize=1024;
F1_actual_Hz = floor(F1_desired_Hz * BlockSize / FS + 0.5) * FS / BlockSize;
F2_actual_Hz = floor(F2_desired_Hz * BlockSize / FS + 0.5) * FS / BlockSize;
**********************************************************************************/

#define N_F2 7   //number of F2 frequencies to choose from
class Test_Parameters {
  public:
    Test_Parameters(void) {};
    int n_freqs = N_F2;

    /**F1 Frequency**/
    float targ_freq1_Hz[N_F2] = {813.80859375, 1220.712890625, 1627.6171875, 2441.42578125, 3295.9248046875, 4923.5419921875, 6551.1591796875};
    //float targ_freq1_Hz[N_F2] = {820.3125f, 1242.1875f, 1604.625f, 2450.9375f, 3281.25f, 4921.875f, 6562.5f};  //ooops! two frequency were typed in wrong
    //float targ_freq1_Hz[N_F2] = {820.3125f, 1242.1875f, 1640.625f, 2460.9375f, 3281.25f, 4921.875f, 6562.5f};  //fixed two frequency values, Feb 24, 2023

    /**F2 Frequency**/
    float targ_freq2_Hz[N_F2] = {1017.2607421875, 1505.5458984375, 1993.8310546875, 3011.091796875, 3987.662109375, 5981.4931640625, 8016.0146484375};
    //float targ_freq2_Hz[N_F2] = {1000.f, 1500.f, 2000.f, 3000.f, 4000.f, 6000.f, 8000.f}; //define the target F2 frequencies
    //float targ_freq2_Hz[N_F2] = {1007.8125f, 1500.0f, 1992.1875f, 3000.0f, 4007.8125f, 6000.0f, 7992.1875f}; //define the target F2 frequencies
    
    float targ_f1_dBSPL = 65.0;
    float targ_f2_dBSPL = 55.0;
    float cal_f1_dBFS_at_94dBSPL[N_F2] = {0.6, 0.5, 1.8, 1.8, -2.2, -2.5, -4.2};
    float cal_f2_dBFS_at_94dBSPL[N_F2] = {1.2, 2.1, 2.8, -1.4, -4.2, -2.4, -14.7};
};

class DPOAE_Settings_Manager {
  public:
    DPOAE_Settings_Manager(Test_Parameters *params) : test_params(params) {};
    
    //define parameters relating to assumptions about the post-processing that will be performed
    int assumed_Nfft = 1024;
    bool flag__adjustToCenterOfFFTBin = true;  //if true, this adjusts f2 so that the DPOAE frequency ends up in the center of an FFT bin?
    
    //methods
    int nextTestStep(Tone_State *tone_state) {  testStep(cur_step_ind++, tone_state);  return cur_step_ind; }
    int testStep(int step_ind, Tone_State *tone_state);
    //void chooseToneFreqs_Hz(float targ_f2_Hz, float *out_f1_Hz, float *out_f2_Hz);
    float setCal_dB(int chan, int step_ind, float cal_dBFS_at_94dBSPL, Tone_State *tone_state) {
      if ( (step_ind < 0) || (step_ind >= test_params->n_freqs) ) return 0.0;
      if (chan == 0) test_params->cal_f1_dBFS_at_94dBSPL[step_ind] = cal_dBFS_at_94dBSPL;
      if (chan == 1) test_params->cal_f2_dBFS_at_94dBSPL[step_ind] = cal_dBFS_at_94dBSPL;  
      set_tone_state_amplitudes(step_ind,tone_state); //this sets some outputs via tone_state
      return cal_dBFS_at_94dBSPL;
    }
    float getCal_dBFS_at_95dBSPL(int chan, int step_ind) {
      if ( (step_ind < 0) || (step_ind >= test_params->n_freqs) ) return 0.0;
      if (chan == 0) return test_params->cal_f1_dBFS_at_94dBSPL[cur_step_ind];
      if (chan == 1) return test_params->cal_f2_dBFS_at_94dBSPL[cur_step_ind];       
      return 0.0f;
    }
    float incrementCal_dBFS(int chan, int step_ind, float incr_dB, Tone_State *tone_state) {
      if ( (step_ind < 0) || (step_ind >= test_params->n_freqs) ) return 0.0;
      float new_val = 0;
      if (chan == 0) new_val = (test_params->cal_f1_dBFS_at_94dBSPL[step_ind] += incr_dB);
      if (chan == 1) new_val = (test_params->cal_f2_dBFS_at_94dBSPL[step_ind] += incr_dB);       
      set_tone_state_amplitudes(step_ind,tone_state); //this sets some outputs via tone_state
      return new_val; 
    }
    void set_tone_state_amplitudes(int step_ind, Tone_State *tone_state) { //output is via tone_state
      if ( (step_ind < 0) || (step_ind >= test_params->n_freqs) ) return;
      tone_state->amp1_dBFS = test_params->targ_f1_dBSPL -94.0 + test_params->cal_f1_dBFS_at_94dBSPL[step_ind];
      tone_state->amp2_dBFS = test_params->targ_f2_dBSPL -94.0 + test_params->cal_f2_dBFS_at_94dBSPL[step_ind];
    }
    
    //test parameters
    //DPOAE_Parameters DPOAE_params;
    Test_Parameters *test_params;
    
  private:
    int cur_step_ind = 0;
    float sample_rate_Hz = 48000;
};


int DPOAE_Settings_Manager::testStep(int step_ind, Tone_State *tone_state) {   //output is via tone_state
  cur_step_ind = max(0,min(step_ind,test_params->n_freqs-1));


  tone_state->freq1_Hz = test_params->targ_freq1_Hz[cur_step_ind]; //this is an output
  tone_state->freq2_Hz = test_params->targ_freq2_Hz[cur_step_ind]; //this is an output
  
  set_tone_state_amplitudes(cur_step_ind,tone_state); //this sets more outputs
  return cur_step_ind;
}

#endif
