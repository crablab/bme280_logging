# Temperature, pressure and humidity - Arduino Grafana Project
### Code samples and guide for a BME280 sensor project, connecting to InfluxDB and Grafana

## Hardware

This was tested on an [Arduino Uno](https://store.arduino.cc/arduino-uno-rev3) but other 3rd party designs or models should work, just as long as the sheild fits on top! 

The sensor used is a [BME280](https://www.adafruit.com/product/2652) - an i2c/SPI calibrated temperature, humidity and pressure sensor. It is pretty plug and play - the relevant code in this guide is from the example by Adafruit [here](https://github.com/adafruit/Adafruit_BME280_Library/blob/master/examples/bme280test/bme280test.ino). You'll need to install their library to use the sensor. 

This also uses an ethernet sheield, compatible with the standard ethernet library (installed by default). You can find a version of their example code [here](https://www.arduino.cc/en/Tutorial/WebServer) from which this is loosely adapted from. An official one can be found [here](https://store.arduino.cc/arduino-ethernet-shield-2) but as with most Arduino stuff, a compatible one works just as well! 

## Arduino wiring

The ethernet shield should just stick right on top - simple! (you don't need an SD card)

The standard wiring for the BME280 is:

| BME280 Pin | Arduino Pin |
|------------|-------------|
| VIN        | 3.3v        |
| GND        | GND         |
| SCL/SCK    | A5          |
| SDA/SDI    | A4          |

Note, we are using the i2c interface here. 

You'll need to wire this to the relevant pins on at the top of your Arduino + shield stack. 

## Arduino code

Awesome! Now that's wired up it's best to check out a quick test script - before we do anything more complicated - so we can check that the sensor is working. The Adafruit example [here](https://github.com/adafruit/Adafruit_BME280_Library/blob/master/examples/bme280test/bme280test.ino) works! 

You should get a nice table of updating values on the serial monitor - if not, check your wiring. 

We can now start working with the ethernet library with a super simple script, to check everything is working. 

```
/* Include the serial library for communicating with the COM port */
#include <SPI.h>
 
/* Include the standard Ethernet library */
#include <Ethernet.h>

#include <Wire.h>

#define SERIAL_BAUD 9600

/* MAC address of the Ethernet shield. If you are using this on your 
own network, then the MAC address below will be fine, but remember if 
you use more than one shield on your network they will need to be assigned
unique MAC addresses */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
 
/* The IP address of the shield. Make sure this matches the IP 
   address range of your network and is not in use by any other 
   device on it */    
IPAddress ip(192, 168, 1, XX);
 
/* The port number the shield will respond to. Use port 80 
   for standard HTTP requests */
EthernetServer server(80);
 
void setup()
{  
  //pinMode(53, OUTPUT); //Uncomment this line if using a Mega
  /* Start the Ethernet interface */
  Ethernet.begin(mac, ip);
  server.begin();
  
  /* Initialise the serial port */
  Serial.begin(9600);
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
            
            /* If it was then we can now send a response to the client’s http request... */
            ethernet.println("HTTP/1.1 200 OK");
            ethernet.println("Content-Type: text/html");
            ethernet.println();
 
            ethernet.println("<body>");
            ethernet.println("Hello world!");
            ethernet.println("</body>");
 
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
```

You need to define an IP address - make sure it's not an IP already allocated to something on the network! 

If you run that and navigate to the IP you have specified above, you should be greeted in your browser with `Hello World`. 

Now that's neat, but we actually want sensor values output there. 

The first thing we need to do is include the BME280 library, and the Wire library which is used for serial communication, just bellow the `Ethernet.h` import: 
```
/*include libraries for BME280 */
#include <BME280I2C.h>
#include <Wire.h>
``` 

We can create the `bme` object so we can start accessing the data. It has some default configuration parameters which you can find more detail on changing [here](https://github.com/adafruit/Adafruit_BME280_Library/blob/master/examples/advancedsettings/advancedsettings.ino) 

We then create the float variables we'll stick the data in and apply some configuration parameters to the BME280 to set the units to Degrees Celsius and Pascals (which are commonly used in the UK).

```
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

// Set up pressure sensor 
float temp(NAN), hum(NAN), pres(NAN);
 
BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);
```

In the `setup()` method, we now need to initlise and check we have the BME280 connected correctly. 

Just below the `Serial.begin` call (which sets up an interface for us to use the serial monitor), we need to get the i2c library going: 

```
while(!Serial) {} // Wait

Wire.begin();
```

We can then try to connect to the BME280: 
```
while(!bme.begin())
{
  Serial.println("Could not find BME280 sensor!");
  delay(1000);
}
```

If that fails, we'll get a helpful error message rather than outputting garbage. 

We then have another helpful (and optional) just to confirm the type of sensor we have:

```
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
 ```
 
 Now that's set up, we only need one simple function call to grab the data from the sensor:
 
 ```
 bme.read(pres, temp, hum, tempUnit, presUnit);
 ``` 
 
 And we stick that at the top of the if statement body where we were returning the response to the browser. So, when we get a valid request we check the sensor values. 
 
Then, it's a simple matter of returning the data. 
 
In this case, I've decided to return JSON (JavaScript Object Notation) which is prettty ubiquitus. It's going to be in the format: 

```
{
  "temp": 22.0,
  "pres": 1013.5,
  "hum": 54
}
```

This will then allow us to easily process it with the Python script. 

First, we need to change the content type from `text/html` to `application/json` so the client knows what we are sending back. 

We can then replace our previous printed data (including the body tags) with JSON: 

```
ethernet.print("{");

//Print temp
ethernet.print("\"temp\":");
ethernet.print(temp);
ethernet.print(",");

ethernet.print("}");
``` 

It should be clear how to add humidity and pressure :) 

If we upload that and go to the IP adress, you should be greeted with a string looking a bit like the template JSON above! (but maybe not prettified!)

## InfluxDB

Influx is a database time based database engine - it's designed for storing metrics (like this) and works really well with a graphing display program called Grafana. 

Installing Influx on an Ubuntu server is really easy. The below is an edited (for brevity) version of [Andre Miller's guide](http://www.andremiller.net/content/grafana-and-influxdb-quickstart-on-ubuntu). 

You can either install from a `.deb` [here](https://portal.influxdata.com/downloads/) or it is in the `apt` packagae manager with instructions on [here](https://docs.influxdata.com/influxdb/v1.7/introduction/installation/). 

Once installed you can start Influx with `sudo service influxdb start`. 

If you then type `influx` you end up in the Influx shell. 

We can easily create a database for our project with `CREATE DATABASE weather` and `SHOW DATABASES` should output showing the new database. Unlike MySQL or the like, you don't need a trailing semicolon. 

We haven't yet added any data... 

## Python Code

The Python script is really simple. We're going to use the Requests library to grab the data, and then, pipe it to Influx.

We'll do this with a continuous while loop, with a delay to prevent any accidental DDoSing! 

```
while(1):
    r = requests.get("http://192.168.1.XX")
    data = r.json()
    print(json.dumps(data))
    time.sleep(60)
```

That should print out your stats every minute! Be sure to change the IP address :) 

Influx has a really nice packagae for python we can install with a quick command `pip install influxdb`. If you don't have pip, then it's on aptitude `sudo apt install python-pip`. The client has it's GitHub [here](https://github.com/influxdata/influxdb-python) with loads more examples. I've adapted the super simple example on their readme. 

Just below the imports we can then set up our Influx connection `client = InfluxDBClient('192.168.1.XX', 8086, 'username', 'password', 'database')`. You'll need to enter the correct IP for the Influx server, the database name and if you've set them, a username a password. By default Influx doesn't have any - if you're running this on a public facing server you'll want to change that! 

For each of the bits of data we're recording (temperature, pressure, humidity) we now need to create a JSON blob which Influx understands. This consists of the measurement name, any tags (you can leave that blank by default), a timestamp and the actual data point we recorded. 

It should look like this: 

```
temp = [
        {
            "measurement": "temp",
            "tags": {
            },
            "time": datetime.utcfromtimestamp(time.time()),
            "fields": {
                "value": data['temp']
            }
        }
    ]
``` 

You can now push this to Influx with a really simple function call `client.write_points(temp)`. Easy! 

If you want to verify that the data is coming through, go back to your Influx shell and type `USE database_name` and then `SELECT * FROM temp` to grab all of the values for that measurement. 

You can then add the other values in the same way. 

## Grafana

Grafana is a fancy application for drawing graphs. It's typically used to show metrics for servers and applications, to identify issues and trends. You can even connect it with stuff like PagerDuty to spam you with calls when your server dies in the middle of the night! 

A dashboard might look like this:

![grafana example](https://raw.githubusercontent.com/crablab/bme280_logging/master/Selection_390.png)

This is again based off Andre's excellent guide. You can find basic installation instructions [here](http://docs.grafana.org/installation/debian/). 

Once installed, you'll need to start the Grafana server `sudo service grafana-server start` and visit `http://IP_ADDRR:3000/`, at which ever IP the server has. Username `admin` and password `admin` are the default credentials. 

After logging in. click on Data Sources in the left menu, and then on Add New in the top menu to add a new datasource.

Click on InfluxDB and then add the appropriate information. The port should be, by default, 8086. There is a Save and Test button which will tell you if all is well! 

Once created, you can then add a dashboard! I've included the JSON for one [here](https://github.com/crablab/bme280_logging/blob/master/grafana.json), but Grafana is really use to use so you can create your own if you like. 

---

And there you have it! An Arduino datalogger piping into Influx and with some pretty graphs in Grafana. You can play around with Grafana and even add more data sources to Influx for more fun. You may find that you can easily use Telegraf to intergrate existing applications into Influx/Grafana - Nginx for example has a plugin for this. 

A quick note: Influx does have a default retention policy of around 7 days unless you specify otherwise. This has the advantage of protecting you from filling up your harddisk with data acidentally, but if you want the data for longer you will need to change that! 
