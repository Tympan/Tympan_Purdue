
const int sd_start_millis = 2000; //dead period after starting SD recording prior to tones starting
const int tone_dur_millis = 3000; //duration of tone
const int silence_dur_millis = 1000; //duration of silence between tones
const float fade_msec = 50.0; //length of fade in and fade out of tones

//update the state of the stepped DPOAE test
int serviceSteppedTest(unsigned long curTime_millis) {
  static unsigned long lastTransition_millis = 0;
  if (curTime_millis < lastTransition_millis) lastTransition_millis = curTime_millis; //prevent wrap-around problems
  unsigned long delta_millis = curTime_millis - lastTransition_millis;

  bool update_gui = false;
  switch (myState.cur_test_state) {
    case (State::TEST_OFF):
      //do nothing
      break;
    case (State::TEST_STARTING):
      muteOutput(true); //this mutes any tones
      audioSDWriter.startRecording();audioSDWriter.setSDRecordingButtons(); //start SD recording
      audioSDWriter.setSDRecordingButtons();
      myState.cur_test_state = State::TEST_SDSTART;
      lastTransition_millis = curTime_millis;
      update_gui = true;
      break;
   case (State::TEST_SDSTART):
      if (delta_millis >= sd_start_millis) {
        //transition to silence
        muteOutput(false); //this unmutes the tones
        jumpToFreqStepAndPlayTones(0);  //start the test
        fade1.fadeIn_msec(fade_msec); fade2.fadeIn_msec(fade_msec);
        myState.cur_test_state = State::TEST_TONE;
        lastTransition_millis = curTime_millis;
        update_gui = true;
      }
      break;
    case (State::TEST_SILENCE):
      if (delta_millis >= silence_dur_millis) {
        //increment to the next tone
        if (myState.cur_step_ind >= (myState.test_params.n_freqs - 1)) {
          stop_DPOAE_test();
        } else {
          incrementFreqStep(+1);
          fade1.fadeIn_msec(fade_msec); fade2.fadeIn_msec(fade_msec);
          //muteOutput(false); //this unmutes the tones
          myState.cur_test_state = State::TEST_TONE;
          update_gui = true;
        }
        lastTransition_millis = curTime_millis;
      }
      break;
    case (State::TEST_TONE):
      if (delta_millis >= tone_dur_millis) {
        //muteOutput(true); //this mutes the tones (ie, goes to silence)
        fade1.fadeOut_msec(fade_msec); fade2.fadeOut_msec(fade_msec);
        //update_gui = true;
        myState.cur_test_state = State::TEST_SILENCE;
        lastTransition_millis = curTime_millis;
      }
      break;
    case (State::TEST_STOPPING):
      muteOutput(true);
      fade1.fadeIn_msec(0.0); fade2.fadeIn_msec(0.0);  //snap the faders back open
      audioSDWriter.stopRecording();audioSDWriter.setSDRecordingButtons();   //stop SD recording
      myState.cur_test_state = State::TEST_OFF;
      lastTransition_millis = curTime_millis;
      update_gui = true;
      break;
  }

  //do we need to update the GUI for the new state?
  if (update_gui) {
    serialManager.updateDPOAEStatus();
    //serialManager.updateCalDisplay();
    //serialManager.updateDPOAEDisplay();
    //serialManager.updateMuteDisplay();
  }

  return myState.cur_test_state;
}
