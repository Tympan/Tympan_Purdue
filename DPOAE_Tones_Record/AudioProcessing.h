


// Create the audio library objects that we'll use
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

//settings for level measurement
float hp_Hz = 100.0;     //cutoff for highpass filter
float lp_Hz = 10000.0;   //cutoff for lowpass filter
float LEQ_ave_sec = 0.5; //averaging time
void setupLevelMeasurements(void) {
  highpass1.setHighpass(0,hp_Hz);  highpass2.setHighpass(0,hp_Hz);
  lowpass1.setLowpass(0,lp_Hz);  lowpass2.setLowpass(0,lp_Hz);
  measureLEQ1.setTimeWindow_sec(LEQ_ave_sec);measureLEQ2.setTimeWindow_sec(LEQ_ave_sec);
}


//code to switch between the different analog inputs
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