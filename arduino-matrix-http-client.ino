//MD_PAROLA version 3.5.6
//MD_MAX72XX version 3.3.0

#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12
#define CLK_PIN   7
#define DATA_PIN  6
#define CS_PIN    5

MD_Parola Parola = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


const char kHostname[] = "192.168.88.186";
int kPort = 8080;
const char kPath[] = "/";

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;

IPAddress dnServer(192, 168, 88, 1);
IPAddress gateway(192, 168, 88, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ip(192, 168, 88, 190);

EthernetClient c;
HttpClient http(c);
  
void setup()
{
  Serial.begin(9600); 
  
  Parola.begin();
  Ethernet.begin(mac, ip, dnServer, gateway, subnet);

  Parola.displayText("Ethernet ok", PA_CENTER, 25, 2000, PA_SCROLL_DOWN, PA_SCROLL_UP);
  while (Parola.displayAnimate() == false) {
    Parola.displayAnimate();
  }
}

void loop()
{
  //Ethernet.maintain();
  
  char data[512];
  int pos = 0;
  memset(&data, 0, sizeof(data));
  
  int err =0;
  
  err = http.get(kHostname, kPort, kPath);
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

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
                // Print out this character
                data[pos++] = c;
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
  }
  else
  {
    Parola.displayText("Connect failed", PA_CENTER, 25, 2000, PA_SCROLL_DOWN, PA_SCROLL_UP);
    while (Parola.displayAnimate() == false) {
      Parola.displayAnimate();
    }
    //Serial.print("Connect failed: ");
    //Serial.println(err);
  }
  http.stop();

  // And just stop, now that we've tried a download

  Parola.displayText(data, PA_LEFT, 25, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (Parola.displayAnimate() == false) {
    Parola.displayAnimate();
  }
  
  delay(3000);
}
