/* Include the serial library for communicating with the COM port */
#include <SPI.h>
 
/* Include the standard Ethernet library */
#include <Ethernet.h>

/*include libraries for BME280 */
#include <BME280I2C.h>
#include <Wire.h>

#define SERIAL_BAUD 9600

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
// Set up pressure sensor 
float temp(NAN), hum(NAN), pres(NAN);
 
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);
/* MAC address of the Ethernet shield. If you are using this on your 
own network, then the MAC address below will be fine, but remember if 
you use more than one shield on your network they will need to be assigned
unique MAC addresses */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
 
/* The IP address of the shield. Make sure this matches the IP 
   address range of your network and is not in use by any other 
   device on it */    
IPAddress ip(192, 168, 1, 91 );
 
/* The port number the shield will respond to. Use port 80 
   for standard HTTP requests */
EthernetServer server(80);
 
 
 
void setup()
{  
  //pinMode(53, OUTPUT); //Uncomment this line if using a Mega
  /* Start the Ethernet interface */
  Ethernet.begin(mac, ip);
  server.begin();
  
  while(!Serial) {} // Wait

  Wire.begin();
  
  /* Initialise the serial port */
  Serial.begin(9600);

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
 
}
 
 
/* MAIN PROGRAM LOOP */
void loop()
{
   
  /* All client requests are terminated with a blank line. This flag will 
    signify if the current line received from this client is a blank line */
  boolean bBlankLineFlag = true;
  
  /* Used to hold the current byre received from the client */
  char cCurrentByte;
  
  /* Used to hold the current byte read from the test.txt file located on the SD card */
  char cCurrentSDByte;
    
  
  /* Wait for a request from a client */
  EthernetClient ethernet = server.available();
  
  if (ethernet) 
  {
    /* Continue to read data from the client one byte at a time until 
       there is no more data */ 
    while (ethernet.connected()) 
    {
      /* Is there still data available to be read? ethernet class 
         ethernet.connected() returns the number of bytes available */
      if (ethernet.available()) 
      {
        /* If data is available read the next byte */ 
        cCurrentByte = ethernet.read();
 
        /* Is the next byte read is a new line termination ? */      
        if (cCurrentByte == '\n')
        { 
          /* If so was it a blank line? */
          if (bBlankLineFlag) 
          {
            bme.read(pres, temp, hum, tempUnit, presUnit);
            Serial.println("Returning data");
            Serial.println(temp);
            Serial.println(pres);
            
            /* If it was then we can now send a response to the client’s http request... */
            ethernet.println("HTTP/1.1 200 OK");
            ethernet.println("Content-Type: application/json");
            ethernet.println();
 
            ethernet.print("{");
            //Print temp
            ethernet.print("\"temp\":");
            ethernet.print(temp);
            ethernet.print(",");

            //Print pres
            ethernet.print("\"pres\":");
            ethernet.print(pres);
            ethernet.print(",");
            
            //Print hum
            ethernet.print("\"hum\":");
            ethernet.print(hum);
           
            ethernet.print("}");

            
 
            /* Disconnect from the client */
            ethernet.stop();
          }
        
          /* The last byte received from the client was the start of a 
             new line so flag as no data received for this line yet */
          bBlankLineFlag = true;
        
        /* If the last byte received wasn't a new line then it must be data... */  
        } else if (cCurrentByte != '\r')
        {  
          /* ...and so flag this as not a blank line. */
          bBlankLineFlag = false;
        }
      }
    }
  }
}
