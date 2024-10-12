
#ifndef _Measurement_h
#define _Measurement_h

#include <AudioCalcLeq_F32.h>  //from Tympan_Library.h
#include <vector>

class Measurement { 
  public: 
    Measurement(AudioCalcLeq_F32 *left, AudioCalcLeq_F32 *right) : measureLevel_L(left), measureLevel_R(right) {};
        
    void takeMeasurement(const int test_step, const float freq_Hz) {
      //Serial.println("Measurement: takeMeasurement: test_step " + String(test_step) + ", freq = " + String(freq_Hz));
      if ((measureLevel_L != nullptr) && (measureLevel_R != nullptr)) {
        //confirm that there is space
        if (test_step >= all_freq_Hz.size()) setSizeOfVectors(test_step);
        
        //save the values
        all_freq_Hz[test_step] = freq_Hz;
        all_left_dB[test_step] = measureLevel_L->getCurrentLevel();
        all_right_dB[test_step] = measureLevel_R->getCurrentLevel();
        printMeasurement(test_step);
      } else {
        Serial.println("Measurement: takeMeasurement: *** ERROR ***: no pointers to level measuring blocks have been provided!");
      }
    }

    void printMeasurement(const int test_step) {
      Serial.print("Measurement: (step, Tone Hz, Left dBFS, Right dBFS): " + String(test_step));
      Serial.print(", " + String(all_freq_Hz[test_step],2));
      Serial.print(", " + String(all_left_dB[test_step],8));
      Serial.print(", " + String(all_right_dB[test_step],8));
      Serial.println();
    }

    void printAllMeasurements(void) {
      Serial.println("Measurement: printing all measurements: " + String(all_freq_Hz.size()));
      for (int i=0; i < all_freq_Hz.size(); i++) {
        printMeasurement(i);
      }
    }

    void clearAllMeasurements(void)  {
      int n = getMinimumNumberOfMeasurments();
      if (all_freq_Hz.size() < n) setSizeOfVectors(n);
      for (int i=0; i < getMinimumNumberOfMeasurments(); i++) {
        all_freq_Hz.at(i) = 0.0; all_left_dB.at(i) = 0.0; all_right_dB.at(i);
      }
    }
    
    void resetLevelCalculators(void) { measureLevel_L->clearStates();  measureLevel_R->clearStates(); }
    

    int setSizeOfVectors(const int n) { 
      Serial.println("Measurement: setSizeOfVectors: n = " + String(n));
      all_freq_Hz.resize(n); 
      all_left_dB.resize(n); 
      all_right_dB.resize(n); 
      return n; 
    }
    int setMinimumNumberOfMeasurements(const int n) {
      Serial.println("Measurement: setMinimumNumberOfMeasurements: n = " + String(n));
      if (n > all_freq_Hz.size()) setSizeOfVectors(n);
      return minimumNumberOfMeasurements = n; 
    }
    int getMinimumNumberOfMeasurments(void) { return minimumNumberOfMeasurements; }

    //data members
    AudioCalcLeq_F32 *measureLevel_L = nullptr;
    AudioCalcLeq_F32 *measureLevel_R = nullptr;
    std::vector<float> all_freq_Hz;
    std::vector<float> all_left_dB;
    std::vector<float> all_right_dB;  //use these to hold data

  private:
    int minimumNumberOfMeasurements = 501;
};

#endif