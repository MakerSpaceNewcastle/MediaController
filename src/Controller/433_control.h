/*
 * 433 MHz SC2262 Remote Emulator (Spark Core port)
 * Alistair MacDonald 2014
 *
 * Once uploaded to a Spark Core it can be executed remotly using the following command...
 *
 * curl https://api.spark.io/v1/devices/{device_id}/switch -d access_token={token} -d "args={remote_device},{on_off}"
 * 
 * Wherer...
 * {device_id}     = The Device ID of the Spark Core (Browse to https://www.spark.io/build/ and select the "Cores" icon to find)
 * {token}         = Your Access Token (Browse to https://www.spark.io/build/ and select the "Settings" icon to find)
 * {remote_device} = The ID set on the mains switch. Add the group (A=0,B=4,C=8,D=12) and the device (1-4)
 * {on_off}        = 0=Off, 1=On
 *
 */

// General settings
#define TRANSMITTERPIN   D3      // Pin connected to the 433MH transmitter

// Timing values that might need tweeking for some devices
#define DURATIONSHORT    400    // Short pulse length
#define DURATIONLONG     1200   // Long pulse length
#define DURATIONSYNCLONG 12400  // Lond sync pusle length


// Send a virtual low
void sendLow ()
{
  // send short long
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONSHORT);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONLONG);
  // send short long
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONSHORT);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONLONG);
}

// Send a virtual high
void sendHigh ()
{
  // send long short
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONLONG);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONSHORT);
  // send long short
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONLONG);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONSHORT);
}

// Send a virtual float
void sendFloat ()
{
  // send short long
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONSHORT);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONLONG);
  // send long short
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONLONG);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONSHORT);
}

// Send a sync pulse
void sendSync ()
{
  // send short synclong
  digitalWrite (TRANSMITTERPIN, HIGH);
  delayMicroseconds (DURATIONSHORT);
  digitalWrite (TRANSMITTERPIN, LOW);
  delayMicroseconds (DURATIONSYNCLONG);
}

// Send the command
void sendSC2262Packet ( int inDevice, int inOnOff ) {

  // Send the device set code
  int theABCD = ( inDevice / 4 ) % 4;
  ( theABCD == 0 ) ? sendLow() : sendFloat();
  ( theABCD == 1 ) ? sendLow() : sendFloat();
  ( theABCD == 2 ) ? sendLow() : sendFloat();
  ( theABCD == 3 ) ? sendLow() : sendFloat();

  // Send the device number code
  int the123 = inDevice % 4;
  ( the123 == 1 ) ? sendLow() : sendFloat();
  ( the123 == 2 ) ? sendLow() : sendFloat();
  ( the123 == 3 ) ? sendLow() : sendFloat();
  ( the123 == 4 ) ? sendLow() : sendFloat();
  
  // Padding
  sendFloat();
  sendFloat();
  sendFloat();

  // Send the state
  ( inOnOff ) ? sendHigh() : sendLow();

  // Send the end of command sync
  sendSync();

}

// Send the command multiple times
void sendSC2262Packets( int inDevice, int inOnOff, int inCount ) {
  int i;
  for ( i=0; i < inCount; i++) {
    sendSC2262Packet( inDevice, inOnOff );
  }
}

