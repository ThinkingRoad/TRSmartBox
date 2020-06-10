/*
  THINKING ROAD - SSR RELAY PID FOR A HOT RUNNER HEATER COIL EQUIPPED WITH A K-TYPE THERMOCOUPLE

  LAST VERSION UPLOADED: ~/Desktop/TRSmartbox/TRSmartbox.ino Aug 21 2017 20:09:31  IDE 10803

  by Eanna Anwell June 2020

  License: Creative-Commons Attribution-ShareAlike 4.0 International

  Thanks to:

                     .
  neighbours                   | Put up with all the mad science and magic smoke
  Superior Smoke                | Ordered lots of quartz we destroyed with thermal breaks
  Chris Rouse                   | "OLEd Thermocouple Display using U8GLIB Library" used for training purposes
  Brett Beauregard              ! PID Simple Example (Augmented with Processing.org Communication)
  Mr. Shroomz                  | Code ideas, chats and technical advice; Guinea pig
  Ladyada.net                   | https://learn.adafruit.com/thermocouple/
  Matthew L Beckler             | My LED handling with arrays is inspired by "RGB LED - Automatic Color Cycling."
                                | Great RGBHSV lib. Adds too much delay. I have an RTC goal and hope to slave everything to a single clock. :)
  Athadawoosh (RIP)             | Would have been the first dispensary we gifted a four-brain unit for compassionate use and/or vapour lounge.
  Thompson Caribou Concentrates | Thompson Caribou Region - We love their shatter and use it almost exclusively
  The Giving Plant              | Closest dispensary in driving distance
  

*/
#include <Arduino.h>
#include <SPI.h>
#include <PID_v1.h> // PID Contol Algorithm

#define smartboxVersion Bingo
#define RelayPin 8 // SSR vcc++ via logic high
#define PotPin   0 // select the procVal pin for the potentiometer
//#define u8x8Font u8x8_font_saikyosansbold8_u
#define u8x8Font u8x8_font_artosserif8_r //quite nice
//u8x8_font_artossans8_r //is ok
//u8x8_font_5x7_f
//u8x8_font_amstrad_cpc_extended_f
//u8x8_font_pxplustandynewtv_r //is ok but boring

#define max6675nano
#define debug
#define serialSplash
#define u8x8lib
#define newtimer // used for 200ms sample temp

#ifdef u8x8lib
#include <U8x8lib.h>
/* Constructor */
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* SCL Connect A5 */ 5, /* SDA Connect A4 */ 4);
int u8x8Row[] = {6, 16, 26, 40, 54, 64};
//int u8x8Col = 70;
int u8x8colOne = 1;
int u8x8colTwo = u8x8colOne + 3;
int u8x8col3 = u8x8colOne + 10;
#endif

#ifdef max6675nano
#include <max6675.h> // interface thermocoupler
int thermoDO = 2, thermoCS = 3, thermoCLK = 4, vccPin = 5, gndPin = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
#endif

int controlMode; // set this to 0 to turn off the unit
int TestsetpointVal = 0;
int potValue, potLastValue, potSlopeValue, potPin = 0;  // select the procVal pin for the potentiometer

// PID Setup
double setpointVal, lastprocVal, procVal, Output;

//Specify the links and initial tuning parameters

float kPID[][3] = {     // 0 IS THE DEFAULT RUN MODE
  {20,  0.5,    100} ,  // 0 slow climb very stable at setpointVal (default conservative 16 Aug 2017)
  {24, 8.3, 20} ,  // Experimental [This was XMT-7100 from reddit; behaves like a good nail. :) ]
};

int kPIDTuning = 1;

PID myPID(&procVal, &Output, &setpointVal, kPID[kPIDTuning][0], kPID[kPIDTuning][1], kPID[kPIDTuning][2], P_ON_M, DIRECT);

// /* RTC Needed */ // not really needed with good pid tunings

unsigned long previousMillis = 0;        // will store last time counter was updated
unsigned long windowStartTime;

const unsigned long WindowSize = 2400; // stops working if this falls below 1000.

//I am assuming this is some sort of partial millisecond timing thing but 1000 seems solid.

/* the variables are unsigned long because the time, measured in miliseconds will quickly become a bigger number than can be stored in an int.
  Note this windowsize also happens to be the minimaum interValue at which to capture tepmerature samples (milliseconds) but any faster than 200
  and the max chip can't keep up. this is the best way to avoid realtime delay and slice a window or burn down a house.

  This should actually be an RTC.

  https://groups.google.com/d/msg/diy-pid-control/fkY11BFG0LI/efeF7kWuAQAJ       */

//end of pid stuff

/* FILTER procVal */

// the variables are unsigned long because the time, measured in miliseconds will quickly become a bigger number than can be stored in an int.
// Note this windowsize also happens to be the minimaum interValue at which to capture tepmerature samples (milliseconds) but any faster than 200
// and the max chip shits the bed. this is the best way to avoid realtime delay and slice a window or burn down a house.

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the Output will respond to
// the procVal. Using a constant rather than a normal variable lets us use this
// Valueue to determine the size of the readings array.

const int numReadings = 3;

int readings[numReadings];      // the readings from the procVal
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

void setup() {

#ifdef newtimer
  OCR0A = 0xAF;            // use the same timer as the millis() function
  TIMSK0 |= _BV(OCIE0A);
#endif

#ifdef u8x8lib
  //  u8x8.setFlipMode(1); not working with current board
  u8x8.setFont(u8x8Font);
  u8x8.begin();

  u8x8.setCursor(0, 0);

  u8x8.drawString(u8x8colOne, 0, "Thinking Road");
  u8x8.drawString(u8x8colOne - 1, 1, "Smart Box Bravo");
#endif

#ifdef serialSplash
  Serial.begin(9600);        // init serial
  Serial.print(__FILE__ " " __DATE__ " " __TIME__);
  Serial.print("  IDE "); Serial.println(ARDUINO);
  Serial.println(" ");
  Serial.print("----------------");
  Serial.println(" ");
  Serial.print("ThinkingRoad.org");
  Serial.println(" ");
  Serial.print("----------------");
  Serial.println(" ");
  Serial.print("Smartbox   BRAVO");
  Serial.println(" ");
#endif

  // use Arduino pins
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  pinMode(RelayPin, OUTPUT); // Relay pin

  windowStartTime = millis();

  //tell the PID to range between 0 and 1.2 times full window size (considere this drives 0-255 max pwm) and bump window scale appropriately
  myPID.SetOutputLimits(0, WindowSize * 1.2);

}  // void setup()

void debugControls() {  // enable this in void loop for an emergency override
  //potValue = 10; // set this to 10 or less to turn off the unit
  //setpointVal = 850; // rating: harsh
  //setpointVal = 800; //
}

void loop() {

  // MAIN APPLICATION

  if (potValue < 5) {
    controlMode = 0;
    myPID.SetMode(MANUAL); // USER CONTROLS OUTPUT VALUE
    Output = 0; // OUTPUT OFF. LAST KNOWN VALUE COULD BE between 0 and full window size (drives 0-255 max pwm)
  } else {
    controlMode = 1;
    myPID.SetMode(AUTOMATIC);  // PID CONTROLS OUTPUT VALUE between 0 and full window size (drives 0-255 max pwm)
  }

  potLastValue = potValue;

  potValue = analogRead(PotPin);

  potSlopeValue = potLastValue - potValue;

  //debugControls(); // set this to override the knob values for testingpotIsBad

  setpointVal =  map(potValue, 0, 1023, 109, 888); // convert 0-1023 potValue to a setpointVal of 109-1075

  setpointVal  = roundf(setpointVal / 10) * 10; // step trimmer

  if (setpointVal < 109) setpointVal = 109; // chop that data value off at the knees
  if (setpointVal > 888) setpointVal = 888; // chop that data value off at the knees

  //  if (potValue > 1020) {//for (int i = 0; i <= 300000; i++){
  //  setpointVal=888;
  //  u8x8.setCursor(u8x8colOne, 3);
  //  u8x8.print("Cleaning");
  //  //          thinkingroad.org
  //  u8x8.setCursor(u8x8colOne+9.8, 3);
  //  u8x8.print(setpointVal);
  ////  if (potValue < 1020) i = 300000;
  //  u8x8.setCursor(u8x8colOne, 5);
  //  if (controlMode == 1) u8x8.print("Heat ON");
  //  //                                thinkingroad.org
  //  else u8x8.print("Heat OFF");
  //  //               thinkingroad.org
  //  u8x8.setCursor(u8x8colOne+9.8, 5);
  //  if (procVal > 100) u8x8.setCursor(u8x8colOne+9.8, 5);
  //  else u8x8.setCursor(u8x8colOne+10.8, 5);
  //  u8x8.print(fixFloat);
  //  }

  //setpointVal = map(setpointVal, 160, 380, 260, 480); // convert quartz offsets to factor dabbing product LEIDENFROST points
  //setpointVal = map(setpointVal, 100, 800, 250, 900); // convert quartz offsets to factor dabbing product LEIDENFROST points
  //setpointVal = map(setpointVal, 160, 800, 260, 900); // convert quartz offsets to factor dabbing product LEIDENFROST points

  /*  This is actually a challenge of phase angle decay when you think about what is really at issue here. Calculating lag time '
      decay should be trivial for a little box like this.This wasn't accomplished very scientifically at the moment because the
      best thermometer I have is a barbeque thermometer fielding the coil decay off a calculateable mass with a low thermal sink.

       In hindsight I should have used an IR therrmometer to take several readings to average them out. In the future a compensatory
       calibration tool using a clip-on IR sensor might be an option. Aftermarket nails have different thermal sinking capabilities
       and this might change the corrective calibration later. Interestingly this might make room temp detection better but will
       change the heat factor slightly for safety.

       I solved the temperature set point error calculation that has been the bane of my existence ever since the project started
       so now I can actually predict the leidenfrost point of bho and essentially volatilze the contents in a predictably
       lab-repeatable fashion as of a few moments agoi. the map Valueue for 850 is actually 938 at the point of the coil heat due
       to thermal transfer, sink and decay.

       So in order to heat the nail to the temperature I //really// intended I had to add a map to scale the heat Valueue the user
       selects internally. I’ve just created the local bubble pan star effect that means I’ve guaranteed a leidenfrost thermal
       reaction occurred rather than combustion or vaporization. It’s like this plasma state transfer that actually causes
       near-perfect evaporation into molecular constituents.

       http://www.physics.emory.edu/faculty/jburton/papers/PhysRevFluids.2.031602.pdf

       http://www.d-nail.com/analysis/Q-Halo.jpg

  */

  myPID.SetTunings(kPID[kPIDTuning][0], kPID[kPIDTuning][1], kPID[kPIDTuning][2]);

  myPID.Compute(); // compute the current pid Value

  /************************************************
     turn the output pin on/off based on pid output
   ************************************************/
  unsigned long now = millis();

  if (now - windowStartTime > WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if (Output > now - windowStartTime)digitalWrite(8, HIGH);
  else {
    digitalWrite(8, LOW);
    if (potValue < 3) Serial.println("!!!TR:Message::MANUAL-OFF-NAIL_NOT_ENERGIZED!!!");
  }


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > WindowSize) {

    previousMillis = currentMillis;

    /* /procVal = thermocouple.readFahrenheit(); // well-tuned PIDs don't need averaging.

       NOPE! DON'T DO THIS!
       This is a tempting opportunity to sample as long as your window time isn't too small.

       You can't try to take more data than the amp can feed you.

       You just get last known if you go too fast and that is often statically too low creating a dangerous ramp.
       This could kill someone if the window time falls below 200ms on a max-6675 chip due to overheating of the
       heaters.

        Since this can actually happen I used a hardware interrupt for a timer instead and slave the read to a
        200ms clock.

        Eventually the entire pid will be clocked to this timing source using this method:



    */


    // tempSlope = (procVal - lastprocVal);  // calculate the slope if you need it for some reasom



  }

  if (procVal > setpointVal * .981) kPIDTuning = 0;   // shift to less aggressive pid tuning and crush overshoot
  else kPIDTuning = 1; // leverage hysteresis and heat "rocket launching"

  // MAIN APPLICATION


#ifdef debug
  /************************************************
     populate all serial procVal output
   ************************************************/

  // MOVED ALL CONTROL OUT OF THIS DISPLAY so it is now flexible!

  Serial.print("setpointVal ");
  Serial.print(setpointVal);
  Serial.print(" ");

  Serial.print("PIDBump ");
  Serial.print(setpointVal * .981);
  Serial.print(" ");

  Serial.print("procVal ");
  Serial.print(procVal);
  Serial.print(" ");

  Serial.print("Output ");
  Serial.print(Output);
  Serial.print(" ");

  Serial.print("potValue ");
  Serial.print(potValue);
  Serial.print(" ");

  Serial.print("controlMode ");
  Serial.print(controlMode);
  Serial.print(" ");

  Serial.print("relayON ");
  Serial.print(digitalRead(RelayPin));
  Serial.print(" ");

  Serial.print("potSlopeValue ");
  Serial.print(potSlopeValue);
  Serial.print(" ");

  Serial.println(" ");

  /************************************************
     populate all serial procVal output end
   ************************************************/

#endif

#ifdef u8x8lib
  u8x8.draw2x2String(u8x8colOne-1, 2, "SP  ");
  u8x8.setCursor(u8x8colTwo, 2);

  if (setpointVal > 999 ) u8x8.setCursor(u8x8colTwo - 1, 2);

  u8x8.print(setpointVal);

  u8x8.draw2x2String(u8x8colOne-1, 4, "PV   ");

  u8x8.setCursor(u8x8col3, 4);

  if (controlMode == 1) u8x8.print("ON ");
  else u8x8.print("OFF");

  if (procVal < 0) u8x8.setCursor(u8x8colTwo, 4); //-32 falls in this range so this is logic floor
  if (procVal >= 0 ) u8x8.setCursor(u8x8colTwo + 2, 4);

  if (procVal >= 10 ) u8x8.setCursor(u8x8colTwo + 1, 4);
  if (procVal >= 100 ) u8x8.setCursor(u8x8colTwo, 4);

  if (procVal >= 1000 ) u8x8.setCursor(u8x8colTwo - 1, 4); //ceiling, max op 1200

  u8x8.print(procVal);

  u8x8.setCursor(u8x8colOne-1, 7);
  if (procVal >= 109) u8x8.print("DANGER: HOT COIL");
                 else u8x8.print("SAFE: COIL COOL ");

// continue on with display
/*
THINKINGROad.org
warning hot nail
safe 



*/
  ////
  ////    if (controlMode == 1) {
  ////
  ////      if (abs(setpointVal - procVal)) < 2) u8x8.drawStr(1, u8x8Row[5], "at setpointVal");
  ////
  ////      if (abs(procVal - setpointVal < 11)) u8x8.drawStr(1, u8x8Row[5], "resting - please wait");
  ////
  ////      if (abs(procVal - setpointVal > 11)) u8x8.drawStr(1, u8x8Row[5], "heating - please wait");
  ////      if (abs(setpointVal - procVal)) < 10) u8x8.drawStr(1, u8x8Row[5], "ready to use");
  ////
  ////        } else {
  ////
  ////      if (abs(procVal - setpointVal > 11)) u8x8.drawStr(1, u8x8Row[5], "cooling - please wait");
  ////      else if ((procVal < 451) && (procVal > 109)) u8x8.drawStr(1, u8x8Row[5], "cooling - swab ok");
  ////      else u8x8.drawStr(1, u8x8Row[5], "at safe temperatures");
  ////
  ////    }
  //
  ////  if (procVal >= setpointVal + 11.00) u8x8.drawStr(u8x8colOne, u8x8Row[5], "resting - please wait");
  ////  if ((procVal <= setpointVal + 11.00)&&(procVal >= setpointVal + 10.00))  u8x8.drawStr(u8x8colOne, u8x8Row[5], "ready to use");
  ////  if ((procVal <= setpointVal - 11.00) && (procVal >= 109.00)) u8x8.drawStr(u8x8colOne, u8x8Row[5], "heating - please wait");
  //  //else u8x8.drawStr(u8x8colOne, u8x8Row[5], "at safe temperatures");

  //    /************************************************
  //       populate all OLED procVal output ends
  //     ************************************************/

  //  if (potValue > 1020) {//for (int i = 0; i <= 300000; i++){
  //  setpointVal=888;
  //  u8x8.setCursor(u8x8colOne, 3);
  //  u8x8.print("        ");
  //  u8x8.setCursor(u8x8colOne, 3);
  //  u8x8.print("Cleaning");
  //  //          thinkingroad.org
  //  u8x8.setCursor(u8x8colOne+9.8, 3);
  //  u8x8.print(setpointVal);
  ////  if (potValue < 1020) i = 300000;
  //  u8x8.setCursor(u8x8colOne, 5);
  //  if (controlMode == 1) u8x8.print("Heat ON");
  //  //                                thinkingroad.org
  //  else u8x8.print("Heat OFF");
  //  //               thinkingroad.org
  //  u8x8.setCursor(u8x8colOne+9.8, 5);
  //  if (procVal > 100) u8x8.setCursor(u8x8colOne+9.8, 5);
  //  else u8x8.setCursor(u8x8colOne+10.8, 5);
  //  u8x8.print();
  //  }

  //
  //    // MOVED ALL CONTROL OUT OF THIS DISPLAY so it is now flexible!
  //
  //    u8x8.setFont(u8x8_font_saikyosansbold8_u);
  ////    u8x8.drawStr(u8x8colOne, u8x8Row[0], "ThinkingRoad.org");
  ////    u8x8.drawStr(u8x8colOne, u8x8Row[1], "Smartbox Pro");
  ////    u8x8.setFont(u8x8_font_courB10_tr);
  //
  //    if (controlMode == 1) { //
  ////      u8x8.drawStr(u8x8colOne,  u8x8Row[2], "Heat:");
  //      u8x8.setCursor(u8x8Col, u8x8Row[2]);
  //      u8x8.print(setpointVal);
  //    } else { //
  ////      u8x8.drawStr(u8x8colOne,  u8x8Row[2], "Heat:");
  ////      u8x8.drawStr(u8x8Col + 19, u8x8Row[2], "OFF");
  //    }
  //
  ////    u8x8.drawStr(u8x8colOne,  u8x8Row[3], "Coil:");
  //    u8x8.setCursor(u8x8Col, u8x8Row[3]);
  //    u8x8.print(procVal);
  //
  ////    u8x8.setFont(u8x8_font_courB08_tr);

  ////  } while {( /* ( u8x8.nextPage() */);
  ////
#endif

}

int ticks = 0;
ISR(TIMER0_COMPA_vect) {
  ticks++;
  if (ticks >= 200) { // clock max chip to 200ms sample time to increase stability

    /* VVV smooth and average thermocouple readings VVV */ //

    // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = thermocouple.readFahrenheit(); // well-tuned PIDs don't need averaging.
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;

    // if we're at the end of the array...
    if (readIndex >= numReadings) {
      // ...wrap around to the beginning:
      readIndex = 0;
    }

    lastprocVal = procVal ;            //save previous reading

    // calculate the average:
    procVal = total / numReadings;

    /* ^^^ smooth and average thermocouple readings ^^^ */ //

    ticks = 0;
  }
}

/* https://www.thickassglass.com/products/electric-e-nail-heater-box-and-coil

    When using E-Nail do NOT use nail directly on your Rigs Joint!!!!
  - Use a Drop Down or Glass Adapter to preserve your rig, if the adapter joint breaks then you can just replace the adapter, not the rig!!

  COMES PRE-PROGRAMMED (READY TO USE)

  Included:

  1 x Electric Nail Heater Unit

  Pin1= AC power
  Pin2= AC power
  Pin3= TC+
  Pin4= TC-
  Pin5= Ground wire

  1 x Power Cord

  Max Temperature: 2000F+ <== this is somewhat insane

  Dimensions: 5.75" (Depth) x 4" (Width) x 4" (Height)

  Weight of Unit: 1.75lb

  Coil Length: 5ft.

  2 Year Warranty (Box ONLY) - If there is anything wrong with it over the course of 2 years we will replace it, no exceptions, hassle free.

  3 Month Warranty (Coil ONLY) - Keep temperatures under 850 for longest life expectancy. (Using at highest temperatures will void warranty)

  IF YOU ALTER THE SETTINGS PLEASE REVERT BACK TO THESE VALUES FOR PROPER FUNCTION.

  ﻿Your settings should be as follows

  Hold down 1st button for 4 seconds
  - Press 2nd button to cycle through
  - Use up and down arrows to select value

  CN-t = 5
  d-U = F
  SL-H = 1300
  SL-L = -200
  CNTL = Pid
  S-HC = Stnd
  St = ON
  CP = 2
  GREV = GR-R
  ALt1 = 2
  ALH1 = 0.2
  ALt2 = 2
  ALH2 = 0.3

  Hold Down 1st button for 3 seconds to exit/save

  From Main screen
  Press 2nd button to see option

  R-5 = RUN
  AL-1 = 0
  AL-2 = 0

  From Main Screen
  Press 1st button
  At = -2 (setting will go back to off when you turn the unit off, this is okay because it is part of the calibration process, it only needs to calibrate once)

*/


