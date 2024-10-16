
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;                    //created in the main *.ino file
extern State myState;                      //created in the main *.ino file
extern AudioSettings_F32 audio_settings;   //created in the main *.ino file  
extern AudioSDWriter_F32_UI audioSDWriter; //created in AudioProcessing.h
extern SdFileTransfer sdFileTransfer;        //created in the main *.ino file

//functions in the main sketch that I want to call from here
extern void setConfiguration(int);
extern float changeCal(int, float);
extern void printGainLevels(void);
extern int incrementFreqStep(int);
extern int jumpToFreqStepAndPlayTones(int);
extern bool muteOutput(bool);
extern void start_DPOAE_test(void);
extern void stop_DPOAE_test(void);
extern bool enablePrintLevelsToGUI(bool);


//externals for MTP
extern void start_MTP(void);
//extern void stop_MTP(void);

//
// The purpose of this class is to be a central place to handle all of the interactions
// to and from the SerialMonitor or TympanRemote App.  Therefore, this class does things like:
//
//    * Define what buttons and whatnot are in the TympanRemote App GUI
//    * Define what commands that your system will respond to, whether from the SerialMonitor or from the GUI
//    * Send updates to the GUI based on the changing state of the system
//
// You could achieve all of these goals without creating a dedicated class.  But, it 
// is good practice to try to encapsulate some of these functions so that, when the
// rest of your code calls functions related to the serial communciation (USB or App),
// you have a better idea of where to look for the code and what other code it relates to.
//

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};
      
    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    int receiveFilename(String &filename,const unsigned long timeout_millis);

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateCalDisplay(void);
    void updateDPOAEStatus(void);
    void updateDPOAEDisplay(void);
    void updateCpuDisplayOnOff(void);
    void updateCpuDisplayUsage(void);
    void updateMuteDisplay(void);
    void updateLevelStartStop(void);
    void updateLevelDisplays(void);
    void updateGUI_inputGain(bool activeButtonsOnly = false);
    void updateGUI_inputSelect(bool activeButtonsOnly = false);    

    //factors by which to raise or lower the parameters when receiving commands from TympanRemote App
    float gainIncrement_dB = 1.0f;            //raise or lower by x dB
  private:

    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

void SerialManager::printHelp(void) {  
 Serial.println(" General: No Prefix");
  Serial.println("  h : Print this help");
  Serial.println(" c/C: Enable/Disable printing of CPU and Memory usage");
  Serial.println(" f/F: Incr/Decrease DPOAE Test Step");
  Serial.println(" 1-7: Jump to DPOAE Test Step"); 
  Serial.println(" o/O: Incr/Decrease F1 Loudness");
  Serial.println(" p/P: Incr/Decrease F2 Loudness");
  Serial.println("  g  : Print all gain levels.");
  Serial.println(" m/M: Mute/Unmute the audio output.");
  Serial.println(" q/Q: Start/Stop the Stepped DPOAE Test.");
  //Serial.println(" w/e: Switch Input to PCB Mics (w) or Line In (e)");
  Serial.println(" l/L: Start/Stop printing measured mic levels.");
  Serial.println(" z  : SD Transfer: Get file names at root of SD.");
  Serial.println(" x    : Transfer file from Tympan SD to PC via Serial ('send' interactive)");
  Serial.println(" X    : Transfer file from PC to Tympan SD via Serial ('receive' interactive)");
  
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
    Serial.println("  > : SDUtil : Start MTP mode to read SD from PC (Tympan must be freshly restarted)");
  #endif

  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  
  
  Serial.println();
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    // case 'w':
    //   Serial.println("Received: Switch input to PCB Mics");
    //   setConfiguration(State::INPUT_PCBMICS);
    //   updateGUI_inputSelect();
    //   updateGUI_inputGain(); //changing inputs changes the input gain, too
    //   break;
    // case 'W':
    //   Serial.println("Recevied: Switch input to Mics on Jack.");
    //   setConfiguration(State::INPUT_JACK_MIC);
    //   updateGUI_inputSelect();
    //   updateGUI_inputGain(); //changing inputs changes the input gain, too
    //   break;
    // case 'e':
    //   myTympan.println("Received: Switch input to Line-In on Jack");
    //   setConfiguration(State::INPUT_JACK_LINE);
    //   updateGUI_inputSelect();
    //   updateGUI_inputGain(); //changing inputs changes the input gain, too
    //   break;      
    case 'g': case 'G':
      printGainLevels();
      break;
    case 'o':
      changeCal(0,gainIncrement_dB);   //raise
      updateCalDisplay();    //update the App
      break;
    case 'O':
      changeCal(0,-gainIncrement_dB);  //lower
      updateCalDisplay();   //update the App
      break;
    case 'p':
      changeCal(1,gainIncrement_dB);   //raise
      updateCalDisplay();    //update the App
      break;
    case 'P':
      changeCal(1,-gainIncrement_dB);  //lower
      updateCalDisplay();   //update the App
      break;   
    case 'q':
      Serial.println("Starting DPOAE Test...");
      start_DPOAE_test();
      break;
    case 'Q':
      Serial.println("Stopping DPOAE Test...");
      stop_DPOAE_test();
      break;
    case 'l':  //lowercase 'L'
      Serial.println("Start printing measured mic levels...");
      enablePrintLevelsToGUI(true);
      updateLevelStartStop();
      break;
    case 'L':
      Serial.println("Stop printing measured mic levels...");
      enablePrintLevelsToGUI(false);    
      updateLevelStartStop();
      break;
    case 'c':
      Serial.println("Starting CPU reporting...");
      myState.printCPUtoGUI = true;
      updateCpuDisplayOnOff();
      break;
    case 'C':
      Serial.println("Stopping CPU reporting...");
      myState.printCPUtoGUI = false;
      updateCpuDisplayOnOff();
      break;   
    case 'f':
      incrementFreqStep(+1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case 'F':
      incrementFreqStep(-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '1':  //one
      jumpToFreqStepAndPlayTones(1-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '2':  //two
      jumpToFreqStepAndPlayTones(2-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;  
    case '3':
      jumpToFreqStepAndPlayTones(3-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '4':
      jumpToFreqStepAndPlayTones(4-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '5':
      jumpToFreqStepAndPlayTones(5-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '6':
      jumpToFreqStepAndPlayTones(6-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case '7':
      jumpToFreqStepAndPlayTones(7-1);
      updateDPOAEDisplay();updateCalDisplay();
      break;
    case 'm':
      muteOutput(true);
      updateMuteDisplay();
      break;
    case 'M':
      muteOutput(false);
      updateMuteDisplay();
      break;
    case 'z':
      Serial.print("SerialMonitor: Listing Files on SD: "); //purposely don't include end-of-line
      sdFileTransfer.sendFilenames(','); //send file names seperated by commas
      break;
    case 'x':
      if ((Serial.peek() == '\n') || (Serial.peek() == '\r')) Serial.read();  //remove any trailing EOL character
      sdFileTransfer.sendFile_interactive();
      break;
    case 'X':
      if ((Serial.peek() == '\n') || (Serial.peek() == '\r')) Serial.read();  //remove any trailing EOL character
      sdFileTransfer.receiveFile_interactive();
      break;
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA  
    case '>':
      Serial.println("SerialMonitor: Received command to start MTP service..."); Serial.flush();delay(10);
      if (audioSDWriter.getState() != AudioSDWriter::STATE::UNPREPARED) {  //anything other than UNPREPARED means that the SD has been used before
        Serial.println("SerialManager: *** ERROR ***: Cannot run MTP if you have recorded to SD.");
        Serial.println("    : You must restart your Tympan to clear out any previous SD activity.");
        Serial.println("    : Once you re-start the Tympan, send the command to activate MTP mode.");
      } else {
        Serial.println("Starting MTP service to access SD card (everything else will stop)");
        start_MTP();
      }
      break; 
  #endif   
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;
  }
  return ret_val;
}

int SerialManager::receiveFilename(String &filename,const unsigned long timeout_millis) {
  Serial.setTimeout(timeout_millis);  //set timeout in milliseconds
  filename.remove(0); //clear the string
  filename += Serial.readStringUntil('\n');  //append the string
  if (filename.length() == 0) filename += Serial.readStringUntil('\n');  //append the string
  Serial.setTimeout(1000);  //return the timeout to the default
  return 0;
}

// //////////////////////////////////  Methods for defining and transmitting the GUI to the App

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("DPOAE Automated Testing");  
      card_h = page_h->addCard("Stepped-Frequency Test");
          card_h->addButton("Start", "q" , "start",        6);
          card_h->addButton("Stop",  "Q" , "",             6);
          card_h->addButton("",      "",   "status",         12);
          
      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      card_h = audioSDWriter.addCard_sdRecord(page_h);
  
  page_h = myGUI.addPage("DPOAE Manual Control");

       card_h = page_h->addCard("Mute Output");   
          card_h->addButton("Mute",   "m" , "mute",         6);
          card_h->addButton("Unmute", "M" , "",             6);

      //Add another card to this page
//      card_h = page_h->addCard("DPOAE Frequencies (Hz)");
//          card_h->addButton("-",    "F", "",      2);  //displayed string, command, button ID, button width (out of 12)
//          card_h->addButton("",     "",  "step",  8);  
//          card_h->addButton("+",    "f", "",      2);  //displayed string, command, button ID, button width (out of 12)
//          card_h->addButton("-",    "",  "",      2);  //placeholder
//          card_h->addButton("F1",   "",  "",      4);  //displayed string, command (blank), button ID, button width (out of 12)
//          card_h->addButton("",     "",  "f1",    4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
//          card_h->addButton("+",    "",  "",      2);  //placeholder
//          card_h->addButton("-",    "",  "",      2);  //placeholder
//          card_h->addButton("F2",   "",  "",      4);  //displayed string, command (blank), button ID, button width (out of 12)
//          card_h->addButton("",     "",  "f2",    4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
//          card_h->addButton("+",    "",  "",      2);  //placeholder

      card_h = page_h->addCard("DPOAE Settings");
          card_h->addButton("-",    "F", "",      2);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("",     "",  "step",  8);  
          card_h->addButton("+",    "f", "",      2);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("F1",   "",  "",      2);  //placeholder
          card_h->addButton("",     "",  "f1",    5);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("",    "",  "spl1",   5);  //placeholder
          card_h->addButton("F2",   "",  "",      2);  //placeholder
          card_h->addButton("",     "",  "f2",    5);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("",    "",  "spl2",   5);  //placeholder
    
      card_h = page_h->addCard("Speaker Cal (dBFS at 94dB SPL)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-",  "O",  "",       2);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("F1", "",  "",        4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("",   "",   "cF1",    4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+",  "o",  "",       2);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("-",  "P",  "",       2);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("F2", "",   "",       4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("",   "",   "cF2",    4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+",  "p",  "",       2);  //displayed string, command, button ID, button width (out of 12)
/*
      card_h = page_h->addCard("Freq 2 Loudness (dBFS)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-",      "P",  "",             4);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("",       "",   "cF2",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+",      "p",  "",             4);  //displayed string, command, button ID, button width (out of 12)
  */
  
      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      card_h = audioSDWriter.addCard_sdRecord(page_h);


  //Add a page for showing the levels
  page_h = myGUI.addPage("Level Measurements");
      // card_h = page_h->addCard("Select Audio Input");
      //   card_h->addButton("PCB Mics",      "w", "configPCB",   12);  //displayed string, command, button ID, button width (out of 12)
      //   //card_h->addButton("Jack as Mic",   "W", "configMIC",   12);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      //   card_h->addButton("Jack as Line-In","e", "configLINE",  12);  //displayed string, command, button ID, button width (out of 12) 
  
    card_h = page_h->addCard(String("Analog Input Gain (dB)"));
      card_h->addButton("",      "", "inpGain",   12); //label, command, id, width (out of 12)...THIS ISFULL WIDTH!

    card_h = page_h->addCard(String("Measured Levels (dBFS)"));
      card_h->addButton("Start", "l", "sLev", 6);      //label, command, id, width (out of 12)
      card_h->addButton("Stop",  "L", "",     6);      //label, command, id, width (out of 12)
      card_h->addButton("LEQ1 (dBFS)", "", "",     6); //label, command, id, width (out of 12)
      card_h->addButton("",            "", "L1", 6); //label, command, id, width (out of 12)
      card_h->addButton("LEQ2 (dBFS)", "", "",     6); //label, command, id, width (out of 12)
      card_h->addButton("",            "", "L2", 6); //label, command, id, width (out of 12)

  //Add another page to the GUI
  page_h = myGUI.addPage("Globals");

    //Add an example card that just displays a value...no interactive elements
    //card_h = page_h->addCard(String("Analog Input Gain (dB)"));
    //  card_h->addButton("",      "", "inpGain",   12); //label, command, id, width (out of 12)...THIS ISFULL WIDTH!
        
    //Add an example card where one of the buttons will indicate "on" or "off"
    card_h = page_h->addCard(String("CPU Usage (%)"));
      card_h->addButton("Start", "c", "cpuStart", 4);  //label, command, id, width...we'll light this one up if we're showing CPU usage
      card_h->addButton(""     , "",  "cpuValue", 4);  //label, command, id, width...this one will display the actual CPU value
      card_h->addButton("Stop",  "C", "",         4);  //label, command, id, width...this one will turn off the CPU display
  
        
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();
    Serial.println(s);
    ble->sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  updateDPOAEStatus();
  updateDPOAEDisplay();
  updateCalDisplay();
  updateMuteDisplay();
  
  updateGUI_inputGain(activeButtonsOnly);
  updateGUI_inputSelect(activeButtonsOnly);
  updateLevelStartStop();
  updateLevelDisplays();
  
  //updateCpuDisplayOnOff();

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

void SerialManager::updateCalDisplay(void) {
  int test_ind = myState.cur_step_ind;
  //float amp, sp1, 
  float cal;

  //amp = myState.tone_state.amp1_dBFS; 
  //spl = myState.test_params.targ_f1_dBSPL;

  cal = myState.test_params.cal_f1_dBFS_at_94dBSPL[test_ind];
  setButtonText("cF1", String(cal,1));
  cal = myState.test_params.cal_f2_dBFS_at_94dBSPL[test_ind];
  setButtonText("cF2", String(cal,1));
}

void SerialManager::updateDPOAEStatus(void) {
  String text;
  if (myState.cur_test_state == State::TEST_OFF) {
      setButtonText("status", "Stopped");
      setButtonState("start",false);
  } else {
      //setButtonText("status", "Step " + String(myState.cur_step_ind + 1));
      setButtonText("status", String("Step ") + String(myState.cur_step_ind + 1) + String(" of ") + String(myState.max_step_ind));
      setButtonState("start",true);
  }  
}
void SerialManager::updateDPOAEDisplay(void) {
  setButtonText("step", String("Step ") + String(myState.cur_step_ind + 1));
  setButtonText("f1", String(myState.tone_state.freq1_Hz,0) + String(" Hz"));
  setButtonText("f2", String(myState.tone_state.freq2_Hz,0) + String(" Hz"));
  setButtonText("spl1", String(myState.test_params.targ_f1_dBSPL,0) + String(" dB SPL"));
  setButtonText("spl2", String(myState.test_params.targ_f2_dBSPL,0) + String(" dB SPL"));
  
}

void SerialManager::updateCpuDisplayOnOff(void) {
  setButtonState("cpuStart",myState.printCPUtoGUI);  //illuminate the button if we will be sending the CPU value
}

void SerialManager::updateCpuDisplayUsage(void) {
   setButtonText("cpuValue",String(audio_settings.processorUsage(),1)); //one decimal places
}

void SerialManager::updateMuteDisplay(void) {
  setButtonState("mute",myState.tone_state.is_muted);  //illuminate the button if we will be sending the CPU value
}


void SerialManager::updateGUI_inputGain(bool activeButtonsOnly) {
  setButtonText("inpGain",String(myState.input_gain_dB,1));
}


void SerialManager::updateGUI_inputSelect(bool activeButtonsOnly) {
  if (!activeButtonsOnly) {
    setButtonState("configPCB",false);
    //setButtonState("configMIC",false);
    setButtonState("configLINE",false);
  }
  switch (myState.input_source) {
    case (State::INPUT_PCBMICS):
      setButtonState("configPCB",true);
      break;
    case (State::INPUT_JACK_MIC): 
      setButtonState("configMIC",true);
      break;
    case (State::INPUT_JACK_LINE): 
      setButtonState("configLINE",true);
      break;
  }
}

void SerialManager::updateLevelStartStop(void) {
    setButtonState("sLev",myState.printLevelsToGUI);
}

void SerialManager::updateLevelDisplays(void) {
  float val;
  String str1,str2;

  val = myState.measuredLEQ_dB[0]; str1 = String("L1");  //measurement for the first channel
  if (val > -200.0) { str2 = String(val,1); } else { String("-"); };  //if a valid value, send the numbers.  If not, set a dash.
  setButtonText(str1,str2);  //actually transmit the new string

  val = myState.measuredLEQ_dB[1]; str1 = String("L2");  //measurement for the second channel
  if (val > -200.0) { str2 = String(val,1); } else { String("-"); }; //if a valid value, send the numbers.  If not, set a dash.
  setButtonText(str1,str2);  //actually transmit the new string
}

#endif
