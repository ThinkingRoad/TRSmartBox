/*
  THINKING ROAD - SSR RELAY PID FOR A HOT RUNNER HEATER COIL EQUIPPED WITH A K-TYPE THERMOCOUPLE

  LAST VERSION UPLOADED: /Users/eanwell/Desktop/TRSmartbox/TRSmartbox.ino Aug 22 2017 23:38:06  IDE 10803

  VERSION 2.0

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
/*
  Changelog:

  Major revison 2.0   `         | Complete overhaul of display code and addition of timers for sampling a max chip more reliably than if we did not
                                | use some sort of RTC. Simplified display data and moved most reporting over to serial debug.


*/
#include <Arduino.h>
#include <SPI.h>
#include <PID_v1.h> // PID Contol Algorithm

#define RelayPin 8 // SSR vcc++ via logic high
#define PotPin   0 // select the input pin for the potentiometer
#define u8g2Font u8g2_font_helvB08_tf

#define max6675nano
//#define debug
#define serialSplash
#define u8g2lib
#define newtimer // used for 200ms sample temp


#ifdef u8g2lib
#include <U8g2lib.h>
//#include <U8x8lib.h>
/* Constructor */
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//SDA  connect A4
//SCL  Connect A5

int u8g2Row[] = {6,14,26,40,54};

//int u8g2Row[] = {0,15,30,45,50};
#endif

#ifdef max6675nano
#include <max6675.h> // interface thermocoupler
int thermoDO = 2, thermoCS = 3, thermoCLK = 4, vccPin = 5, gndPin = 6;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
#endif

int controlMode; // set this to 0 to turn off the unit
int TestSetpoint = 0;
int potPin = 0;  // select the input pin for the potentiometer
int potValue, potLastValue, potSlopeValue;

// PID Setup
double Setpoint, pInput, Input, Output;
//
//Specify the links and initial tuning parameters
//
float kPID[][3] = {     // 0 IS THE DEFAULT RUN MODE
  {20,  0.5,    100} ,  // 0 slow climb very stable at setpoint (default conservative 16 Aug 2017)
  {24, 8.3, 20} ,  // Experimental [This was XMT-7100 from reddit; behaves like a good nail. :) ]
  // {48,  5,      5} ,    // 1 XMT-7100 Kp, Ki, Kd (PID) parameters (default aggressive 17 Aug 2017) - probably overshoots at 1000K/hr, so break it
  // {125, 60,     10}, // auberins default
  // {20,  0.1,    100} ,  // 2 also slow climb
  // {10,  0.1,    50} ,   // 3 fairly solid stepup ramp that is visibly rising (faster than above 2 still slow)
  // {5,   0.1,    50} ,   // 4 fairly solid stepup ramp that is visibly rising (faster than above 2 still slow)
  // {4,   0.02,   5} ,    // conservative Kp, Ki, Kd (PID) parameters
  // {40,  0.025,  20} ,   // PRE-HEAT
  // {100, 0.025,  25} ,   // SOAK Kp, Ki, Kd (PID) parameters
  // {200, 15,     50} ,   // REFLOW Kp, Ki, Kd (PID) parameters
};

int kPIDTuning = 1;

PID myPID(&Input, &Output, &Setpoint, kPID[kPIDTuning][0], kPID[kPIDTuning][1], kPID[kPIDTuning][2], P_ON_M, DIRECT);

// /* RTC Needed */ // not really needed with good pid tunings

unsigned long previousMillis = 0;        // will store last time counter was updated
unsigned long windowStartTime;

const unsigned long WindowSize = 2400; // stops working if this falls below 1000.

//I am assuming this is some sort of partial millisecond timing thing but 1000 seems solid.

/* the variables are unsigned long because the time, measured in miliseconds will quickly become a bigger number than can be stored in an int.
  Note this windowsize also happens to be the minimaum interValue at which to capture tepmerature samples (milliseconds) but any faster than 200
  and the max chip shits the bed. this is the best way to avoid realtime delay and slice a window or burn down a house.

  This is super fucking dumb and should actually be an RTC.

  https://groups.google.com/d/msg/diy-pid-control/fkY11BFG0LI/efeF7kWuAQAJ       */

//end of pid stuff

/* FILTER Input */

// not really needed with good pid tunings other than for aesthetics

// the variables are unsigned long because the time, measured in miliseconds will quickly become a bigger number than can be stored in an int.
// Note this windowsize also happens to be the minimaum interValue at which to capture tepmerature samples (milliseconds) but any faster than 200
// and the max chip shits the bed. this is the best way to avoid realtime delay and slice a window or burn down a house.

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the Output will respond to
// the Input. Using a constant rather than a normal variable lets us use this
// Valueue to determine the size of the readings array.

const int numReadings = 3;

int readings[numReadings];      // the readings from the Input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average



void setup() {

#ifdef newtimer
  OCR0A = 0xAF;            // use the same timer as the millis() function
  TIMSK0 |= _BV(OCIE0A);


#endif

#ifdef u8g2lib
  //u8gs.setRot180();
  u8g2.setFont(u8g2Font);
  u8g2.begin();

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
  Serial.print("Smartbox   ALPHA");
  Serial.println(" ");
  Serial.println(" ");

#endif

  // use Arduino pins
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  pinMode(RelayPin, OUTPUT); // Relay pin

  windowStartTime = millis();

  //tell the PID to range between 0 and 1.2 times full window size (considere this drives 0-255 max pwm) and bump window scale appropriately
  myPID.SetOutputLimits(0, WindowSize * 1.2);

  // void setup()
}

void debugControls() {  // enable this in void loop for an emergency override
  //potValue = 10; // set this to 10 or less to turn off the unit
  //Setpoint = 850; // rating: harsh
  //Setpoint = 800; //
}

void loop() {
  u8g2.clearBuffer();


  // MAIN APPLICATION

  if ((potValue < 10) || (potValue > 1100)) {
#ifdef debug
    Serial.println("TR:Message:: MANUAL    - OFF - NAIL NOT ENERGIZED");
#endif
    controlMode = 0;
    myPID.SetMode(MANUAL); // USER CONTROLS OUTPUT VALUE
    Output = 0; // OUTPUT OFF. LAST KNOWN VALUE COULD BE between 0 and full window size (drives 0-255 max pwm)
  } else {
#ifdef debug
    Serial.println("TR:Message:: AUTOMATIC - ON  - HEATING TO SETPOINT");
#endif
    controlMode = 1;
    myPID.SetMode(AUTOMATIC);  // PID CONTROLS OUTPUT VALUE between 0 and full window size (drives 0-255 max pwm)
  }
  potLastValue = potValue;

  potValue = analogRead(PotPin);

  potSlopeValue = potLastValue - potValue;

  //debugControls(); // set this to override the knob values for testingpotIsBad

  Setpoint =  map(potValue, 0, 1023, 0, 850); // convert 0-1023 potValue to a setpoint of 600-1000


  Setpoint  = roundf(Setpoint / 10) * 10; // step trimmer

  if (Setpoint < 300) Setpoint = 300; // chop that data value off at the knees
  if (Setpoint > 850) Setpoint = 850; // chop that data value off at the knees






  //Setpoint = map(Setpoint, 160, 380, 260, 480); // convert quartz offsets to factor dabbing product LEIDENFROST points
  //Setpoint = map(Setpoint, 100, 800, 250, 900); // convert quartz offsets to factor dabbing product LEIDENFROST points
  //Setpoint = map(Setpoint, 160, 800, 260, 900); // convert quartz offsets to factor dabbing product LEIDENFROST points

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
  else digitalWrite(8, LOW);

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > WindowSize) {

    previousMillis = currentMillis;

    /* /Input = thermocouple.readFahrenheit(); // well-tuned PIDs don't need averaging.

       NOPE! DON'T DO THIS!
       This is a tempting opportunity to sample as long as your window time isn't too small. You can't try to take more data than the amp can feed you.

        You just get last known if you go too fast and that is often statically too low creating a dangerous ramp. This could kill someone if the window
        time falls below 200ms on a max-6675 chip due to overheating of the heaters.

        Since this can actually happen I used a hardware interrupt for a timer instead and slave the read to a 200ms clock.

    */


    // tempSlope = (Input - pInput);  // calculate the slope if you need it for some reasom



  }

  if (Input > Setpoint * .981) kPIDTuning = 0;   // shift to less aggressive pid tuning and crush overshoot
  else kPIDTuning = 1; // leverage hysteresis and heat "rocket launching"

  // MAIN APPLICATION

#ifdef u8g2lib
  u8g2.firstPage();
  do {
    /* all graphics commands have to appear within the loop body. */
    u8g2.setFont(u8g2Font);
#endif

#ifdef debug
    /************************************************
       populate all serial input output
     ************************************************/
    // MOVED ALL CONTROL OUT OF THIS DISPLAY so it is now flexible!

    Serial.print("Setpoint ");
    Serial.print(Setpoint);
    Serial.print(" ");

    Serial.print("PIDBump ");
    Serial.print(Setpoint * .981);
    Serial.print(" ");

    Serial.print("Input ");
    Serial.print(Input);
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
       populate all serial input output end
     ************************************************/
#endif

#ifdef u8g2lib

    u8g2.setFont(u8g2_font_courB08_tf);
    // MOVED ALL CONTROL OUT OF THIS DISPLAY so it is now flexible!
    u8g2.drawStr(0, u8g2Row[0], "ThinkingRoad.org ");
    u8g2.drawStr(0, u8g2Row[1], "Smartbox Pro");
    u8g2.setFont(u8g2_font_courB10_tf);
    if (controlMode == 1) { //
      u8g2.drawStr(0,  u8g2Row[2], "Heater");
      u8g2.setCursor(65, u8g2Row[2]);
      u8g2.print(Setpoint);
      u8g2.drawStr(0,  u8g2Row[3], "Coil");
      u8g2.setCursor(65, u8g2Row[3]);
      u8g2.print(Input);
    } else { //
      u8g2.drawStr(0,  u8g2Row[2], "Heater");
      u8g2.drawStr(70, u8g2Row[2], "OFF");
      u8g2.drawStr(0,  u8g2Row[3], "Coil");
      u8g2.setCursor(65, u8g2Row[3]);
      u8g2.print(Input);
    }


    u8g2.setFont(u8g2_font_courB08_tf);
    if (Input > 109) u8g2.drawStr(0, u8g2Row[4], "DO NOT TOUCH COIL!");
    else u8g2.drawStr(0, u8g2Row[4], "SAFE TO TOUCH COIL!");



    // continue on with display

    // if (abs(Setpoint - Input) < 2) u8g2.drawStr(0, u8g2Row[8], "At setpoint! :)");
    //
    //    if (abs(Input - Setpoint > 11))  u8g2.drawStr(0, u8g2Row[8], "Cooling - Wait");
    //    else if (abs(Input - Setpoint < 11)) u8g2.drawStr(0, u8g2Row[8], "Heating - Wait");
    //    else if ((abs(Setpoint - Input) < 10) || controlMode == 1) u8g2.drawStr(0, u8g2Row[8], "Ready to use! :)");
    //
    //    }



    /************************************************
       populate all OLED input output ends
     ************************************************/
  } while ( u8g2.nextPage() );
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

    pInput = Input ;            //save previous reading

    // calculate the average:
    Input = total / numReadings;


    /* ^^^ smooth and average thermocouple readings ^^^ */ //

    ticks = 0;
  }
}
