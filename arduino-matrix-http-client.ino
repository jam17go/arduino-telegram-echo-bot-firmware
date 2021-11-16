//MD_PAROLA version 3.5.6
//MD_MAX72XX version 3.3.0
//Arduino Http Client version 0.0.4
//https://arduinoplusplus.wordpress.com/2018/09/23/parola-a-to-z-optimizing-flash-memory/

/*#define ENA_MISC    0   ///< Enable miscellaneous animations
#define ENA_WIPE    0   ///< Enable wipe type animations
#define ENA_SCAN    0   ///< Enable scanning animations
#define ENA_SCR_DIA 0   ///< Enable diagonal scrolling animation
#define ENA_OPNCLS  0   ///< Enable open and close scan effects
#define ENA_GROW    0   ///< Enable grow effects
#define ENA_SPRITE  0   ///< Enable sprite effects
#define ENA_GRAPHICS  0 ///< Enable graphics functionality*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <HttpClient.h>

#include "arduino_secrets.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12
#define CLK_PIN   7
#define DATA_PIN  6
#define CS_PIN    5

char kHostname[] = SERVER_HOSTNAME;
char kPath[] = SERVER_PATH;
int kPort = SERVER_PORT;

int kNetworkTimeout = 30*1000;
int kNetworkDelay = 1000;
int failsInaRow = 0;
int resetPin = 8;

byte mac[] = BOARD_MAC;

MD_Parola Parola = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void reset() {
  digitalWrite(resetPin, LOW);
}

void setupResetPin()
{
  digitalWrite(resetPin, HIGH);
  delay(200);
  pinMode(resetPin, OUTPUT);
  delay(200);
}

void animateDisplay()
{
  while (Parola.displayAnimate() == false) {
    Parola.displayAnimate();
  }
}

String setupEthernet()
{
  Serial.println("Network setup started");
  
  while (Ethernet.begin(mac) != 1)
  { 
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      reset();
    }

    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }

    Parola.displayText("Ethernet hardware error. Check console output for details", PA_LEFT, 25, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    
    animateDisplay();

    delay(10000);
  }

  Serial.print("  DHCP assigned IP ");
  Serial.println(Ethernet.localIP());

  delay(2000);

  String ipAddress = ConvertIpAddress(Ethernet.localIP());

  return ipAddress;
}

void setup()
{
  setupResetPin();
    
  Serial.begin(9600);
  Serial.println("=== Arduino Echo Bot Firmware ===");

  Parola.begin();

  String ipAddress = setupEthernet();

  String iPStr = "My IP: " + ipAddress;
  
  char *pIpStr = iPStr.c_str();

  Parola.displayText(pIpStr, PA_LEFT, 25, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  
  animateDisplay();
}

String ConvertIpAddress(IPAddress address)
{
  return String(address[0]) + "." + 
         String(address[1]) + "." + 
         String(address[2]) + "." + 
         String(address[3]);
}

void loop()
{
  if (failsInaRow >= 10) {
    reset();
  }

  Ethernet.maintain();
  EthernetClient ethClient;
  HttpClient http(ethClient);
  
  int err = 0;
  int pos = 0;
  char data[256];

  memset(&data, 0, sizeof(data));
  
  err = http.get(kHostname, kPort, kPath);
  if (err == 0)
  {
    Serial.println("Request started ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
      
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) )
        {
            if (http.available())
            {
                c = http.read();
                
                // Save char to data
                data[pos++] = c;

                // Print out this character
                Serial.print(c);
                
                bodyLen--;
                // We read something, reset the timeout counter
                timeoutStart = millis();
            }
            else
            {
                // We haven't got any data, so let's pause to allow some to
                // arrive
                delay(kNetworkDelay);
            }
        }
        Serial.println();
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }

  failsInaRow = 0;
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
    
    failsInaRow = failsInaRow + 1;
  }

  http.stop();

  Parola.displayText(data, PA_LEFT, 25, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  
  animateDisplay();

  delay(5000);
}
