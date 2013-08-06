#include <Wire.h>                       // For clock card
#include <RTClib.h>                     // For clock card
#include <SD.h>                         // For SD card logging
#include <math.h>

//  Define statements for hardware pins and timeing constraints

#define FOREVER 1                       // For while FOREVER loop in generate data file name function

#define BEAM_LATENCY 3000               // Minimum bounce milliseconds on IR beams
#define RESET_TIMEOUT 10000             // Maximum time allowed between inside beam and outside beam breakage
#define LOGGING_INTERVAL 55000          // Millisecond logging interval (ie once every 30 seconds)

#define STATUS_LED_PIN 46               // LED light pin
#define LED_N_SIDE 50                   // For light sensor
#define LED_P_SIDE 51                   // For light sensor
#define IR_INSIDE_BEAM 6                // Pin of outside IR beam
#define IR_OUTSIDE_BEAM 7               // Pin of inside IR beam

// Global constants and variables for IR beams to count movement of bats
   int inCount = 0;                     // Count of bats entering house
   int outCount = 0;                    // Count of bats leaving house
   int timeoutCount = 0;                // Count of timeouts of IR sensors
   int errCount = 0;                    // Count of invalid IR states - should always be zero
   long unsigned insideMillis = 0;      // Millisecond time of last inside IR trip
   long unsigned outsideMillis = 0;     // Millisecond time of last outside IR trip
   int state = 0;                       // IR beam states - 0 --> No trip, 1 --> Inside beam tripped; 2 --> Ouside beam tripped

// Global constants and structures for real time clock/calandar card
   RTC_DS1307 RTC;                      // For RTC 1307 Real Time Clock structure

// Global constants and variables for SD logging card
#define MAXRECORDSPERFILE 60            // Maximum records per SD card logging file  
   long unsigned logMillis = 0;         // Millisecond time of last SD logging
   int gNumRecordsThisFile = 0 ;        // Count of number of records written thus far to current file.
   boolean gSdCardInited = false;       // Boolean variable to indicate if SD card initialized okay
   boolean gFileOpened = false;         // Boolean variable to indicate if an SD card file is currently opened
   String gCurrentFname = NULL;         // Name of currently opened SD card file
   const int chipSelect = 10;
   const String logFileNamePrefix = "Log";
   const String logFileNameSuffix = ".dat" ;
   File gDataFile;                      // Handle to our SD card file(s)
   char gInternalFnameChar [50] = { '\0' } ;
   String gFileNameBase = "" ;

// Global consts and vars for thermistor readings
   const int thermistorAnalogIn = 8 ;

// Global consts for IBM light sensor readings
   const int lightSensorNarrowPin = 9 ;
   const int lightSensorWidePin = 10 ;

// Global consts and vars for PIR motion detector here
   const int pirDigitalInPin = -1 ;


// ********************************************** Arduino program setup function *****************************************//

void setup() {
  
//  Open serial communications and wait for port to open:
    Serial.begin(9600);

    Serial.println("\nsetup - function starting");
    Serial.flush();
    
//  Set led pin as output only
    pinMode ( STATUS_LED_PIN, OUTPUT ) ;
       
//  Setup beam detectors inputs, turn on internal resister pullups per
//  http://arduino.cc/en/Tutorial/DigitalPins
    Serial.println("setup - About to initialize IR beams");
    Serial.flush();
    pinMode ( IR_INSIDE_BEAM, INPUT ) ;
    digitalWrite ( IR_INSIDE_BEAM, HIGH ) ;
    pinMode ( IR_OUTSIDE_BEAM, INPUT ) ;
    digitalWrite ( IR_OUTSIDE_BEAM, HIGH ) ;

//  Initialize first ir beam break times and sd card logging time
    logMillis = insideMillis = outsideMillis = millis();
  
// Initialize the RTC real time clock card
    Serial.println("setup - About to initialize RTC");
    Serial.flush();
    Wire.begin();
    RTC.begin();

//  Initialize and test the SD card for logging  ability
    Serial.println("setup - About to initialize SD card");
    Serial.flush();
    gSdCardInited = initSdCard();
    
//  Get next SD card file name
    Serial.println("setup - About to generate next file name");
    Serial.flush();
    gCurrentFname = generateNextFileName (  ) ;

//  Set up to log data
    Serial.println("setup - About to start logging data");
    Serial.flush();
    int result = startLoggingData (  ) ;

//  Let user know SD logging state
    if ( result == 1 ) {
       Serial.println ( "Starting to log data to '" + gCurrentFname + "'") ;
    }  // End of if result clause
    else {
       Serial.println ( "Unable to start logging data to '" + gCurrentFname + "'") ;
    }  // End of else clause

    Serial.println("setup - function ending\n");
    Serial.flush();
    
}   // End of setup function


// ************************************** Arduino main looping function *****************************************//

void loop() {
  
// Initialize a string for assembling the data to the SD card log
   String dataString = "";
   long lightLevelLed = 0 ;
  
//   Serial.println("\nLoop function entered - About to enter readLedLightSensor");
//   Serial.flush();
  
// Read the LED light sensor
   for ( int i = 0 ; i < 2 ; i++ ) {
//     lightLevelLed = readLedLightSensor ( 5 ) ;
   }  // End of for i loop
    
//   Serial.println("Returned from readLedLightSensor");
//   Serial.flush();
  
// Get our new IR beam state to count entrances and exits of bats
   int insideSignal = digitalRead(IR_INSIDE_BEAM);
   int outsideSignal = digitalRead(IR_OUTSIDE_BEAM);
   
// Get current milliTime to determine accruacy of IR beams and the need to log
   long unsigned milliTime = millis();

// Invoke our IR analysis function to make sense of IR hardware signals
   if  (insideSignal == LOW || outsideSignal == LOW) {
//     Serial.println("loop - invoking irAnalysis - state=" + String(state) + "   insideSignal=" + 
//         String(insideSignal) + "   outsideSignal=" + String(outsideSignal) + "   milliTime=" + String(milliTime));
       state = irAnalysis( state, insideSignal, outsideSignal, milliTime);  
//     Serial.println("loop - returned from irAnalysis - state=" +
//         String(state) + "\n");
   }   // End of if insideSignal clause

// Get the current date/time from the ds1307 RTC to be put into the logging line
//   Serial.println("About to invoke RTC now function");
     DateTime now = RTC.now();
//   Serial.println("Returned from RTC now function");
//   Serial.flush();

// Format the current date/time as YY/MM/DD, HH:MM:SS (DEC means decimal, not December)
   dataString =  String(now.year(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.day(), DEC) + "," +
       String(now.hour(), DEC) + ":" + String(now.minute(), DEC) + ":" + String(now.second(), DEC) +
       "," + lightLevelLed ;

// Read and calculate thermistor temperature
   int rawADC = analogRead ( thermistorAnalogIn ) ;
   float scaledResult = Thermistor ( rawADC ) ;
  
// Convert the floating variable to ascii for logging
//   Serial.println("Loop about to invoke ftoa");
//   Serial.flush();
   char temp [50] ;
   ftoa ( temp, (double) scaledResult, 2 ) ;
// sprintf ( temp, "%lf", scaledResult ) ;
   dataString += "," ;
   dataString += temp ;
  
// Read the first IBM light sensor
   int ibmLightLevel = readLight2 ( lightSensorNarrowPin ) ;
   dataString += "," ;
   dataString += ibmLightLevel ;
  
// Read the second IBM light sensor
   ibmLightLevel = readLight2 ( lightSensorWidePin ) ;
   dataString += ","  ;
   dataString += ibmLightLevel ;

// Log data if logging interval has expired
   if  ((milliTime - logMillis) >=LOGGING_INTERVAL ) {

//     Include movement count and reset counts to zero     
       dataString = dataString + String(inCount) + "," + String(outCount) + "," + String(timeoutCount) + "," + String(errCount);
       inCount = outCount = timeoutCount = errCount = 0;
       
//     Reset previous logging operation     
       digitalWrite ( STATUS_LED_PIN, LOW ) ;

//     Serial.println("Loop about to invoke logNextDataSample");
       Serial.flush();
       logNextDataSample ( dataString ) ;
  
       Serial.print ( "dataString = " ) ;
       Serial.println ( dataString ) ;
       digitalWrite ( STATUS_LED_PIN, HIGH ) ;
//     delay ( 55000 ) ;            // We can no longer use any delays withoud losing count of bats entering/exiting

//     If we have hit our maximum records per SD card logging file, get a new file name  
       if ( gNumRecordsThisFile > MAXRECORDSPERFILE ) {
          openLogFile ( generateNextFileName ( ) ) ;
//        Serial.println ( "Opening file " + gCurrentFname ) ;
        }  // End of if NumRecords clause
   }    // End of if LOGGING_INTERVAL clause
   
//   int lightSensor2Pin=0;  //feel like this doesn't actually go here.. 
//   int lightSensor2=readLight2(lightSensor2Pin);

//   Serial.println("loop - function ending\n");
}  // End of loop function


// ************************************ SD Card support functions ***********************************//

// Initialize the SD card, and open the file named fname.  Return 1 if Ok,
// return 0 if not, print error messages to serial port.
//
// Based on: http://arduino.cc/en/Tutorial/Datalogger
//

//  Function to open SD card
boolean initSdCard(  ) {
    Serial.println("\ninitSdCard - function started - gSdCardInited=" + String(gSdCardInited));
    Serial.flush();

//  If card has already been initialized, we MUST NOT not do it again
    if  (gSdCardInited)
        return true;
      
    Serial.println("Initializing SD card...");
    Serial.flush();

//  Make sure that the default chip select pin is set to output, even if we don't use it:
    pinMode(chipSelect, OUTPUT);

//  If the SD card is not present, tell user and remember it
    if ( !SD.begin ( chipSelect ) ) {
       Serial.println("Card failed, or not present");
       Serial.flush();
       gSdCardInited = false;    // don't do anything more:
    }  // End of if SD.begin clause

//  Otherwise, remember that it has been inited
    else {
       Serial.println("Card initialized.");
       Serial.flush();
       gSdCardInited = true;
    }  // End of else clause
    
//    gSdCardInited = true;
  
    Serial.println("initSdCard - function ended - gSdCardInited=" + String(gSdCardInited) + "\n");
    Serial.flush();
    return gSdCardInited;                // Return init status - true if inited; false if NOT inited

}   // End of initSdCard function


// Open new data log file
boolean openLogFile ( String fname ) {

//  Close any previous log file that might be opened
    Serial.println("openLogFile started");
    closeLogFile();       // Close any previously opened log files - Only one opened file allowed per SD card
    
//  Make sure SD card is initialized
    initSdCard();
    
//  If SD card is inited, open the data logging file
    if (gSdCardInited) {
       fname.toCharArray ( gInternalFnameChar, sizeof ( gInternalFnameChar ) - 1 ) ;
       gDataFile = SD.open ( gInternalFnameChar, FILE_WRITE);

//     If open was not successful, tell user
       if (!gDataFile )
          Serial.println ( "Error opening '" + fname + "'" ) ;
   
//     Otherwise, if successful, tell user and remember file status
       else {
          Serial.println ( "File " + fname + " opened" ) ;
          gCurrentFname = fname ;
          gFileOpened = true;
          gNumRecordsThisFile = 0 ;
       }  // End of else clause
    }  // End of if gSdCardInited clause

//  Tell calling function - true if successful; false otherwise
    return gFileOpened;

}   // End of openLogFile function


// Close current data log file
void closeLogFile() {

    Serial.println("closeLogFile started");

//  If a file is currently, flush its data and close
    if ( gFileOpened ) {
       gDataFile.flush ( ) ;
       gDataFile.close ( ) ;
       gFileOpened = false;
    }  // End of if gDataFile clause
    
    Serial.println("closeLogFile ended");

    return;

}  // End of closeLogFile function


// Generate the next filename, by searching for the first available 
// name by including a sequence number (1, 2, 3, ...) into our log file base ("Logxxxxx.dat").
// If successful, the next file name will be returnd; otherwise, the string will be null
String generateNextFileName (  ) {

   int i=10000;                      // Start at 10000 so our converted string will be five digits long
   String sequence = "";
   String fileName = NULL;
   char   BufName[15] = "";          // Character array for SD.exists function

   Serial.println("generateNextFileName started");

// Initialize SD card. If successful, fine an unused file name
   initSdCard();
   if (gSdCardInited) {
     
//    Loop until we have an unused file name
      while (FOREVER) {
         i++;
         sequence = String(i);
         sequence.setCharAt(0, sequence.charAt(0) - 1);                  // Subtract one from the leading digit so we actually start at '00001'
         fileName = logFileNamePrefix + sequence + logFileNameSuffix;    // Convert String to a character array
         fileName.toCharArray(BufName, sizeof(BufName)-1);
         
         if (!SD.exists(BufName))       // If file does not exist,
            break;                      // Time to exit loop - we have a winner
      }  // End of while FOREVER loop
   }  // End of if SdCardInited clause

   Serial.println( "Logging File Name is '" + fileName + "'" );
   Serial.flush();
  
   return fileName;                   // Don't be shy - tell them the name of the file

}   // End of generateNextFileName function


// Data logging functions
int startLoggingData (  ) {

    initSdCard( );  // Initialize SD card. Success or failure is set in gSdCardInited global variable

//  Close any previously opened log file
    closeLogFile();
  
//  If SD card was initialized okay, get new file name
    if (gSdCardInited)
       gCurrentFname = generateNextFileName();      // If successful, new file name will be returned.
                                                    // If not successful, a null value will be returned
  
//  Open the new log file
    gFileOpened = openLogFile(gCurrentFname);
  
    return gFileOpened;                             // Return file opened status
    
}   // End of startLoggingDate function


// Write the next (pre-formatted) value to the file
boolean logNextDataSample ( String dataString ) {
  
   Serial.println("logNextDataSample started - gFileOpened=" + String(gFileOpened));
   Serial.flush();
  
// If we have a data file opened, print a data line to it 
   if ( gDataFile ) {
      Serial.println("logNextDataSample printing '" + dataString + "'");
      Serial.flush();
      gDataFile.println( dataString );
      gNumRecordsThisFile ++ ;    // Increment log file record count
      return 1 ;
   }  // End of if gDataFile clause
   
   return 0 ;
    
}  // End of logNextDataSample function

// ******************************************* LED Light Sensor functions ***********************************//

// Need to make this into a moving average using a static value
long averageLight(int numSamples, long sample){
  int i;
  long sampleRead ;


  for (i=0 ; i < numSamples ; i++){
    sampleRead += sample;
  }
  sampleRead = sampleRead / numSamples;
  return sampleRead;
}

long readLedLightSensor ( int numSamples )
{
  long sample = 0 ;

  // Apply reverse voltage, charge up the pin and led capacitance
  pinMode(LED_N_SIDE,OUTPUT);
  pinMode(LED_P_SIDE,OUTPUT);
  digitalWrite(LED_N_SIDE,HIGH);
  digitalWrite(LED_P_SIDE,LOW);

  // Isolate the pin 2 end of the diode
  pinMode(LED_N_SIDE,INPUT);
  digitalWrite(LED_N_SIDE,LOW);  // turn off internal pull-up resistor

  // Count how long it takes the diode to bleed back down to a logic zero
  for ( int j = 0L; j < 120000; j++) {
    if ( digitalRead(LED_N_SIDE)==0) break;
    sample = j;
  }

  // You could use 'j' for something useful, but here we are just using the
  // delay of the counting.  In the dark it counts higher and takes longer, 
  // increasing the portion of the loop where the LED is off compared to 
  // the 1000 microseconds where we turn it on.

  // Turn the light on for 1000 microseconds
  digitalWrite(LED_P_SIDE,HIGH);
  digitalWrite(LED_N_SIDE,LOW);
  pinMode(LED_P_SIDE,OUTPUT);
  pinMode(LED_N_SIDE,OUTPUT);
  delayMicroseconds(1000);
  // we could turn it off, but we know that is about to happen at the loop() start
  return sample ;
}
//By: Ashley Mitchell
int readLight2 (int lightSensor2Pin) {
  // read the input on analog pin:
  int sensorValue = analogRead(lightSensor2Pin);  //Don't know that you want to name my pin

    //Serial.println(sensorValue);
  delay(1);        // delay in between reads for stability

  return sensorValue;
}

// *********************************************** Thermistor functions *****************************************//

float Thermistor(int RawADC) {
// Call function like...
// temp=Thermistor(analogRead(ThermistorPIN)); 
// ... from main function
  float pad = 10000;                       // balance/pad resistor value, set this to
                                           // the measured resistance of your pad resistor
  float thermr = 10000;                   // thermistor nominal resistance
 
  long Resistance;  
  float Temp;  // Dual-Purpose variable to save space.
  Resistance=((1024 * pad / RawADC) - pad); 
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius                      
  Temp = (Temp * 9.0)/ 5.0 + 32.0;                  // Convert to Fahrenheit
  return Temp;                                      // Return the Temperature
}
// End of function defintions

// Helper function from
// http://arduino.cc/forum/index.php/topic,44262.0.html
char *ftoa(char *a, double f, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}  // End of ftoa function


int  irAnalysis(int curState, int inSignal, int outSignal, long unsigned milliTime) {

     int newState = curState;

//   Serial.println("irAnalysis - entered - curState=" + String(curState) + "   insideSignal=" +
//       String(inSignal) + "   outsideSignal=" + String(outSignal) + "   milliTime=" + String(milliTime));
//   Serial.println("irAnalysis - insideMilli=" + String(insideMillis) +
//       "   insideDiff=" + String(milliTime - insideMillis));
//   Serial.println("irAnalysis - outsideMillis=" + String(outsideMillis) +
//       "   outsideDiff=" + String(milliTime - outsideMillis));
         
//   If we have an inside signal timeout, reset state
     if  ((curState == 1) && ((milliTime - insideMillis) > timeoutCount)) {
         curState = 0;
         timeoutCount++;
         insideMillis = milliTime;
     }   // End of insideMillis clause

//   If we have an outside signal timeout, reset state
     if  ((curState == 2) && ((milliTime - outsideMillis) > timeoutCount)) {
         curState = 0;
         timeoutCount++;
         outsideMillis = milliTime;
     }   // End of outside Millis clause

//   If inside signal has tripped, handle it
     if  (inSignal) {

//       If inside beam has settled down, analyze state
         if  ((milliTime - insideMillis) >= BEAM_LATENCY) {
//           Serial.println("irAnalysis - inSignal - milliTime=" + String(milliTime) + 
//              "   insideMillis=" + String(insideMillis) + "   difference=" + String(milliTime - insideMillis));
             
//           Case 0 - No movement in progress
//           Case 1 - Inside signal has tripped
//           Case 2 - Outside signal has tripped
             switch(curState) {
                    
                case 0:
                    newState = 1;
                    insideMillis = milliTime;
                    break;
                    
                case 1:
                    timeoutCount++;
                    newState = 1;
                    insideMillis = milliTime;
                    break;
         
                case 2:
                    inCount++;
                    newState = 0;
                    insideMillis = milliTime;
                    break;
                    
                default:
                    newState = 0;
                    errCount++;
                    break;
             }  // End of switch block
         }   // End of if inMillis clause
     }   // End of if inSignal clause
 
 //  If outside signal has tripped, handle it
     if  (outSignal) {

//       If outside beam has settled down, analyze state
         if  ((milliTime - outsideMillis) >= BEAM_LATENCY) {
//           Serial.println("irAnalysis - outSignal - milliTime=" + String(milliTime) + 
//              "   outsideMillis=" + String(outsideMillis) + "   difference=" + String(milliTime - outsideMillis));
                
//           Case 0 - No movement in progress
//           Case 1 - Inside signal has tripped
//           Case 2 - Outside signal has tripped
             switch(curState) {
                    
                case 0:
                    newState = 2;
                    outsideMillis = milliTime;
                    break;
                
                case 1:
                    outCount++;
                    newState = 0;
                    outsideMillis = milliTime;
                    break;
               
                case 2:
                    timeoutCount++;
                    newState = 2;
                    outsideMillis = milliTime;
                    break;
                    
                default:
                    newState = 0;
                    errCount++;
                    break;
                    
             }  // End of switch block
         }   // End of if outMillis clause
     }   // End of if outSignal  clause
     
     Serial.println("irAnalysis - ended - newState=" + String(newState) + "   inCount=" + String(inCount) +
         "   outCount=" + outCount + "   timeoutCount=" + String(timeoutCount) + "   errCount=" + String(errCount));
     
     return newState;
     
}  // End of irAnalysis function

