/*

* LAST VERSION UPLOADED: ~/Desktop/TRSmartbox/TRSmartbox.ino Aug 21 2017 20:09:31  IDE 10803
  * Finally fully converted over to u8x8 and sorted out temperature detecting code:
    *               -32 falls in this range so this is logic floor
    * 
    * if (procVal <  0 ) u8x8.setCursor(u8x8colTwo, 5); 
    * if (procVal >= 0 ) u8x8.setCursor(u8x8colTwo + 2, 5);
    * if (procVal >= 10 ) u8x8.setCursor(u8x8colTwo + 1, 5);
    * if (procVal >= 100 ) u8x8.setCursor(u8x8colTwo, 5);
    * if (procVal >= 1000 ) u8x8.setCursor(u8x8colTwo - 1, 5); 
    *                 
    *                1000 ceiling, max op 1200 is ok

==============================================================================================================
ThinkingRoad.org Smartbox   ALPHA 

/Users/eanwell/Desktop/TRSmartbox-u8x8-embiggen/TRSmartbox-u8x8-embiggen.ino Aug 28 2017 01:09:29  IDE 10803

  testing out some big font code.

==============================================================================================================
ThinkingRoad.org Smartbox   909 

/Users/eanwell/Desktop/TRSmartbox909-u8x8-noled/TRSmartbox909-u8x8-noled.ino Aug 28 2017 10:44:01  IDE 10803

  Realized I have been staring at the thinkingroad Smartbox 909 POC and have been using it as my daily driver
  for a very long time now. It's time to do the basic block diagram to see if I can get to an UL/CSA-listed
  status and be insurable/bondable.


==============================================================================================================

---------------- 
ThinkingRoad.org 
---------------- 
SmartBox 999 
/Users/eanwell/Desktop/TRSmartBox/TRSmartBox.ino Aug 28 2017 23:51:22  IDE 10803
 
  Some perhaps simpler temperature code:

    if (setpointVal >= 1000) {
      u8x8.draw2x2String(6, 2, varToString);
  } else if (setpointVal >= 100)  { 
      u8x8.draw2x2String(6, 2, " "); 
      u8x8.draw2x2String(8, 2, varToString);
  } else if (setpointVal < 10) {  
      u8x8.draw2x2String(6, 2, " "); 
      u8x8.draw2x2String(8, 2, "OFF");
  } else {  
    u8x8.draw2x2String(8, 2, " "); 
    u8x8.draw2x2String(10, 2, varToString);
  }



              
*/
