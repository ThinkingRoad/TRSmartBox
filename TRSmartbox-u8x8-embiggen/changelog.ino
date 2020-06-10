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

---------------- 
ThinkingRoad.org 
---------------- 
Smartbox   ALPHA 

/Users/eanwell/Desktop/TRSmartbox-u8x8-embiggen/TRSmartbox-u8x8-embiggen.ino Aug 28 2017 01:09:29  IDE 10803

  testing out some big font code.


              
*/
