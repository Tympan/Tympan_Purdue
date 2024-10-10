/*
   To compile for MTP, you need to:
     * Arduino IDE 1.8.15 or later
     * Teensyduino 1.58 or later
     * MTP_Teensy library from https://github.com/KurtE/MTP_Teensy
     * In the Arduino IDE, under the "Tools" menu, choose "USB Type" and then "Serial + MTP Disk (Experimental)"

   After compiling and pushing to the Tympan, you can use MTP.  To use MTP, you need to:
     * Connect the Tympan to a PC via USB and turn on the Tympan
     * Open a SerialMonitor (such as through the Arduino IDE)
     * [Optional: send an 'h' (without quotes) to see the Tympan's help menu]
     * Send the character '>' (without quotes) to the Tympan to start MTP
     * After a few seconds, the Tympan should appear in Windows as a drive named "Teensy"
     * After using MTP to transfer files, you must cycle the power on the Tympan to get out of MTP mode.

   MTP Support is VERY EXPERIMENTAL!!  There are weird behaviors that come with the underlying
   MTP support provided by Teensy and its libraries.  

   TROUBESHOOTING:
  
   Symptom: Tympan does not appear as a drive in your operating system.
     * Be aware that you cannot record (or play) from SD and then activate MTP. If you've used the SD card
       at all, the MTP doesn't seem to work. Sorry.  The MTP only works if nothing else has used the SD card.  
     * But of course you need to use the SD card to record or play!  Why else would you want MTP? You want
	   MTP so that you can get files that you've recorded (or are going to play)!  What do you do??
	 * The easiest solution is to record and play from the SD as you desire.  Then, when you go to connect
	   to the PC, simply cycle the power on teh Tympan.  
	 * That way, it starts fresh.  The "first" time it goes to access the SD card will be for MTP.  Easy.
	 
   Symptom: The Tympan still does not appear as a drive in your operating system. 
     * This is most likely to happen immediately after you've first programmed a non-MTP Tympan
	   to use MTP.  I think that the PC (Windows, at least) gets confused as to what kind of device
	   is attached.  The way to get around this problem is to give Windows a chance to forget the device.
     * A solution seems to be to (1) turn off the Tympan, (2) unplug from USB, (3) wait 5 seconds,
	   (4) reconnect to USB, (5) power up the Tympan.
     * When the Tympan boots up and appears in the SerialMonitor again, go ahead and send the '>' character.
       It should show up as a drive on your PC!

   Symptom: the Arduino IDE cannot program the Tympan when the Tympan is running an MTP-enabled sketch like this
      one.  Similarly, you might find that the SerialMonitor doesn't seem to work with your Tympan.  These are
      similar problems.
     * Be aware that when your Tympan is running MTP-aware code, the generic serial link back to the PC is
       a little bit different, which sometimes gives the Arduino IDE trouble.  The Arduino IDE seems to be
       confused about the serial port and so reprogramming or the SerialMonitor don't work right.
     * If that happens, cycle the Tympan's power and once it has rebooted, go under the "Tools" menu,
       and click on the "Port" submenu.  You'll see (on Windows, at least) that your Tympan seems to have
       two "Teensy" entries in this menu.  Choose whichever entry wasn't already selected.  Then, try uploading
       your Arduino code again.  It should now work.
     * If it still doesn't work, cycle the Tympan power, and switch back to the original port.  It should really
       work now!
*/

uint32_t MTP_store_num = 0; //state variable used by code below.  Don't change this.
bool is_MTP_setup = false;  //state variable used by code below.  Don't change this.
bool use_MTP = false;       //state variable used by code below.  Don't change this.

#if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
 
  #include <SD.h>
  #include <MTP_Teensy.h>  //from https://github.com/KurtE/MTP_Teensy, use Teensyduino 1.58 or later
  
  void start_MTP(void) {
    use_MTP = true;
  }
  
  void setup_MTP(void) {

    //audioSDWriter.end();  //stops any recordings (hopefully) and closes SD card object (hopefully)
    MTP.begin();  //required
    SD.begin(BUILTIN_SDCARD);  // Start SD Card (hopefully this fails gracefully if it is already started by by audio code)
    MTP_store_num = MTP.addFilesystem(SD, "SD Card"); // add the file system (hopefully this fails gracefully if it is already started)
    is_MTP_setup=true;
  }
  
  void service_MTP(void) {
    // has the MTP already been setup?
    if (is_MTP_setup == false) { setup_MTP(); }

    // now we can service the MTP
    MTP.loop();
  }

  //Feb 7, 2024: Stopping the MTP doesn't really seem to work.  Sorry
  void stop_MTP(void) {
    //SD.stop();
    if (is_MTP_setup) {
      (MTP.storage())->removeFilesystem(MTP_store_num); 
      is_MTP_setup = false;
    }
    use_MTP = false;
  }
   
#else

  #warning "You need to select USB Type: 'MTP Disk (Experimental)' or 'Serial + MTP Disk (Experimental)'"


  //here are place-holder definitions for when "MTP Disk" isn't selected in the IDE's "USB Type" menu
  void start_MTP(void) {};
  void setup_MTP(void) {
    Serial.println("setup_MTP: *** ERROR ***: ");
    Serial.println("   : When compiling, you must set Tools -> USB Type -> Serial + MTP Disk (Experimental)");
  }
  void service_MTP(void) {};
  void stop_MTP(void) { };

#endif
