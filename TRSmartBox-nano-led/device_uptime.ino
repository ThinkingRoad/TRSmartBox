///*
//* DEVICE UPTIME
//* -------------
//* Returns the total time in days hours minutes and seconds that arduino running *
//* Limitations apply:
//* Arduino UNO autoresets the board on new serial connection so logs are reseted too.
//* millis overflow every 50 days or so
//* The stats are not aqurate for very long period of time but its good for diagnostic 
//* purposes when arduino is used for final (standalone) project 
//* I use it on my relay board in order to get info if something went wrong
//* Good to know that a power failure caused syste to reset.
//*
//*/
//
//
////  Creator Nikos Georgousis
////  JAN 2011
//
//int incomingByte =0;
//long currentmillis=0;
////############################################ SETUP vvvv  ####################################
//void setup () {
// Serial.begin(9600); // open serial port
// Serial.println("'Project Uptime"); //print something to notify user on restart 
//}
////############################################ SETUP ^^^^ #####################################
////############################################ LOOP vvvv  #####################################
//void loop () {
// SerialCheck (); //redirect ro serial check
//}
////############################################ LOOP ^^^^  #####################################
////########################################### SERIALCHECK vvvv  ###############################
//void SerialCheck () {
// if (Serial.available() > 0) {
//   incomingByte = Serial.read();
//   {    
//     if (incomingByte==63) // if ? received then answer with data
//     {
//       currentmillis=millis(); // get the  current milliseconds from arduino
//       // report milliseconds
//       Serial.print("Total milliseconds running: "); 
//       Serial.println(currentmillis);
//       uptime(); //call conversion function to display human readable time
//     }
//   }
// }
//}
////######################################## SERIALCHECK ^^^^  #################################
////############################################ UPTIME vvvvv  #################################
void uptime()
{
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
 secs = currentmillis/1000; //convect milliseconds to seconds
 mins=secs/60; //convert seconds to minutes
 hours=mins/60; //convert minutes to hours
 days=hours/24; //convert hours to days
 secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
 mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
 hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
 //Display results
 Serial.println("Running Time");
 Serial.println("------------");
   if (days>0) // days will displayed only if value is greater than zero
 {
   Serial.print(days);
   Serial.print(" days and :");
 }
 Serial.print(hours);
 Serial.print(":");
 Serial.print(mins);
 Serial.print(":");
 Serial.println(secs);
}
////############################################ UPTIME ^^^^  #################################
