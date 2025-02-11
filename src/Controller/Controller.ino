
#include <ESP8266WiFi.h>

#include "WiFiCredentials.h"
#include "LMSDetails.h"
#include "433_control.h"

#include <ArduinoOTA.h>

const int PIN_LED = D4;
const int PIN_PLAY = D1;
const int PIN_SKIP = D2;
const int PIN_LED_PLAY = D7;
const int PIN_LED_SKIP = D6;

const int DEBOUNCE = 20;
const int LONG_PRESS = 2000;

unsigned long playPressed = 0;
unsigned long skipPressed = 0;

bool playLongPressActivated = 0;
bool skipLongPressActivated = 0;

bool ledFlash = 0;
bool ledPlayFlashing = 0;
bool ledSkipFlashing = 0;

bool ampStatus = 0;
int idletime = 30000; // idle time in millis before the amp is switched off
unsigned long lastPlaying = 0;
int idleCheckFreq = 3000; // time in millis between checking the player status
unsigned long lastIdleCheck = 0;

WiFiClient client;



void setup()
{
  Serial.begin(115200);
  Serial.println();
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_SKIP, INPUT_PULLUP);
  pinMode(PIN_LED_PLAY, OUTPUT);
  pinMode(PIN_LED_SKIP, OUTPUT);
  pinMode (TRANSMITTERPIN, OUTPUT);


  digitalWrite (TRANSMITTERPIN, LOW);
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_LED_PLAY, LOW);
  digitalWrite(PIN_LED_SKIP, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);

  Serial.println();
  Serial.println();
  Serial.print("Connecting");
  ledSkipFlashing=1;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    flashLEDs();
  }
  ledSkipFlashing=0;
  Serial.println();
  digitalWrite(PIN_LED, LOW);

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname(HOSTNAME);

// No authentication by default
   ArduinoOTA.setPassword((const char *)"OTAPassword");

   ArduinoOTA.onStart([]() {
     Serial.println("Start");
   });
   ArduinoOTA.onEnd([]() {
     Serial.println("\nEnd");
   });
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
   });
   ArduinoOTA.onError([](ota_error_t error) {
     Serial.printf("Error[%u]: ", error);
     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
     else if (error == OTA_END_ERROR) Serial.println("End Failed");
   });

  ArduinoOTA.begin();

}

void loop()
{
  ArduinoOTA.handle();

  if(!digitalRead(PIN_PLAY)) // Button pressed
  {
    if(playPressed == 0) // Button only just pressed
    {
      Serial.println("JUST PRESSED NOOOOOOOOOOOOOOOOOOOOWWWWWWWWWWWWWW!!!!");
      playPressed = millis();
    }
    else if(millis() - LONG_PRESS > playPressed && !playLongPressActivated)
    {
      PlayLongPressed();
      playLongPressActivated = 1;

    }
    else
    {
      
    }
  }
  else  //  Button not pressed
  {
    if(playPressed != 0) // Button only just released
    {
      if(millis() - playPressed > DEBOUNCE && !playLongPressActivated)
      {
        PlayShortPressed();
      }
    }
    playPressed = 0;
    playLongPressActivated = 0;
  }

  if(!digitalRead(PIN_SKIP)) // Button pressed
  {
    if(!skipLongPressActivated) 
    {
      digitalWrite(PIN_LED_SKIP, HIGH);
    }

    if(skipPressed == 0) // Button only just pressed
    {
      Serial.println("JUST PRESSED NOOOOOOOOOOOOOOOOOOOOWWWWWWWWWWWWWW!!!!");
      skipPressed = millis();
    }
    else if(millis() - LONG_PRESS > skipPressed && !skipLongPressActivated)
    {
      digitalWrite(PIN_LED_SKIP, LOW);
      SkipLongPressed();
      skipLongPressActivated = 1;
    }
    else
    {
      
    }
  }
  else  //  Button not pressed
  {
    digitalWrite(PIN_LED_SKIP, LOW);

    if(skipPressed != 0) // Button only just released
    {
      if(millis() - skipPressed > DEBOUNCE && !skipLongPressActivated)
      {
        SkipShortPressed();
      }
    }
    skipPressed = 0;
    skipLongPressActivated = 0;
  }
    
  checkPlayerState();
  flashLEDs();
}

void flashLEDs()
{
  if(ledPlayFlashing || ledSkipFlashing)
  {
    ledFlash = (millis()%1000 >= 500);
    if(ledPlayFlashing)
    {
      digitalWrite(PIN_LED_PLAY, ledFlash);
    }
    if(ledSkipFlashing)
    {
      digitalWrite(PIN_LED_SKIP, ledFlash);
    }
  }
}

void checkPlayerState()
{
  if(millis()-lastIdleCheck > idleCheckFreq)
  {
    Serial.println("Checking Player State");
    lastIdleCheck=millis();

    String req = PLAYERID;
    req.concat(" mode ?");
    String resp = ServerRequest(req);
    Serial.println(resp);
    if(resp.endsWith("play\r\n"))
    {
      Serial.println("Music Playing");
      lastPlaying = millis();
      digitalWrite(PIN_LED_PLAY, HIGH);
      ledPlayFlashing = 0;
      if(!ampStatus)
      {
        Serial.println("Turning Amp ON");
        sendSC2262Packets(PLUGID,1,20);
        ampStatus = 1;
      }
      checkforLastTrack();
    }
    else
    {
      Serial.println("Music Not Playing");
      ledPlayFlashing = 1;
      if(ampStatus && millis()-lastPlaying > idletime)
      {
        Serial.println("Turning Amp OFF");
        sendSC2262Packets(PLUGID,0,20);
        ampStatus = 0;
      }
    }
  }
}

void checkforLastTrack()
{
  String req;
  String resp;
  int index = 0;
  int tracks = 0;

  req = PLAYERID;
  req.concat(" playlist index ?");
  resp = ServerRequest(req);
  Serial.println(resp);

  index = resp.substring(43).toInt();
  //Serial.println(index);

  req = PLAYERID;
  req.concat(" playlist tracks ?");
  resp = ServerRequest(req);
  Serial.println(resp);

  tracks = resp.substring(44).toInt();
  //Serial.println(tracks);

  if(index + 1 == tracks)
  {
    addRandomAlbum();
  }
}

void addRandomAlbum()
{
  String req;
  String resp;
  int albums;
  int index;
  
  // Get total number of albums
  req = "info total albums ?";
  resp = ServerRequest(req);
  Serial.println(resp);

  albums = resp.substring(18).toInt();
  Serial.println(albums);

  req = "albums ";
  req.concat(random(albums+1));
  req.concat(" 1");
  resp = ServerRequest(req);
  Serial.println(resp);

  index = resp.indexOf("A");
  Serial.println(index);
  resp = resp.substring(index+1);
  index = resp.indexOf(" ");
  index = resp.substring(0, index+1).toInt();
  Serial.println(resp.substring(0, index+1));
  
  req = PLAYERID;
  req.concat(" playlist addtracks album.id=");
  req.concat(index);
  resp = ServerRequest(req);
  Serial.println(resp);  

/*
  req = PLAYERID;
  req.concat(" playlist addtracks album.id=");
  req.concat(albums+1);
  resp = ServerRequest(req);
  Serial.println(resp);
*/

}

void SkipShortPressed()
{
  Serial.println("SKIP SHORT PRESSED");
  String req = PLAYERID;
  req.concat(" playlist index +1");
  String resp = ServerRequest(req);
  Serial.println(resp);

  delay(1000);
}

void SkipLongPressed()
{
  Serial.println("SKIP LONG PRESSED");
  String req = PLAYERID;
  req.concat(" playlist clear");
  String resp = ServerRequest(req);
  Serial.println(resp);

  delay(1000);
}


void PlayShortPressed()
{
  Serial.println("PLAY SHORT PRESSED");
  String req = PLAYERID;
  req.concat(" mode ?");
  String resp = ServerRequest(req);
  Serial.println(resp);
  if(resp.endsWith("play\r\n"))
  {
    Serial.println("Music Playing");
    req = PLAYERID;
    req.concat(" pause");
    resp = ServerRequest(req);
    Serial.println(resp);
    digitalWrite(PIN_LED_PLAY, LOW);
    ledPlayFlashing = 1;
  }
  else if(resp.endsWith("pause\r\n"))
  {
    Serial.println("Music Paused");
    playerPlay();
  }
  else if(resp.endsWith("stop\r\n"))
  {
    Serial.println("Music Stopped");
    playerPlay();
  }
  else
  {
    Serial.println("Not Sure...");
  }

    Serial.println("Turning Amp ON");
    sendSC2262Packets(PLUGID,1,20);
    ampStatus = 1;
  
}


void PlayLongPressed()
{
  Serial.println("PLAY LONG PRESSED");
    Serial.println("SKIP PRESSED");
  String req = PLAYERID;
  req.concat(" favorites playlist play");
  String resp = ServerRequest(req);
  Serial.println(resp);
}


void playerPlay()
{
  String req = PLAYERID;
  req.concat(" playlist tracks count ?");
  String resp = ServerRequest(req);
  Serial.println(resp);

  if(resp.endsWith(" 0\r\n"))
  {

    req = PLAYERID;
    req.concat(" randomplay albums");
    resp = ServerRequest(req);
    Serial.println(resp);
    
  }



  req = PLAYERID;
  req.concat(" play");
  resp = ServerRequest(req);
  Serial.println(resp);

  digitalWrite(PIN_LED_PLAY, HIGH);
  ledPlayFlashing = 0;
  if(!ampStatus)
  {
    Serial.println("Turning Amp ON");
    sendSC2262Packets(PLUGID,1,20);
    ampStatus = 1;
  }
}

void playerPause()
{
    String req = PLAYERID;
    req.concat(" pause");
    String resp = ServerRequest(req);
    Serial.println(resp);
}

String ServerRequest(String request)
{
  String response = "" ;

  if (!client.connect(HOSTIP, PORT)) 
    {
      Serial.println();
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(HOST);
      Serial.println("connection failed");
      delay(5000);
      return ">>> Connection Failed !";
    }
    else{
      Serial.print(">>> Sending:    ");
      Serial.println(request);
      client.println(request);
        
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          //Serial.println(">>> Client Timeout !");
          client.stop();
          //delay(60000);
          return ">>> Client Timeout !";
        }
      }
      while (client.available())
        {
          char ch = static_cast<char>(client.read());
          response.concat(ch);

            //ok = sscanf( input, "macaddress mode %s", mode )   
          //if( ok)
          //{

          }
        }

  return response;
    
}
