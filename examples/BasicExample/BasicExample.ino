/*
 * Test Arduino Manager for iPad / iPhone / Mac
 *
 * A simple test program to show the Arduino Manager
 * features.
 *
 * Author: Fabrizio Boco - fabboco@gmail.com
 *
 * Version: 1.0.0
 *
 * 06/02/2021
 *
 * All rights reserved
 *
 */
      
/*
*
 * AMController libraries, example sketches (“The Software”) and the related documentation (“The Documentation”) are supplied to you 
 * by the Author in consideration of your agreement to the following terms, and your use or installation of The Software and the use of The Documentation 
 * constitutes acceptance of these terms.  
 * If you do not agree with these terms, please do not use or install The Software.
 * The Author grants you a personal, non-exclusive license, under author's copyrights in this original software, to use The Software. 
 * Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by the Author, including but not limited to any 
 * patent rights that may be infringed by your derivative works or by other works in which The Software may be incorporated.
 * The Software and the Documentation are provided by the Author on an "AS IS" basis.  THE AUTHOR MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT 
 * LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE SOFTWARE OR ITS USE AND OPERATION 
 * ALONE OR IN COMBINATION WITH YOUR PRODUCTS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, 
 * REPRODUCTION AND MODIFICATION OF THE SOFTWARE AND OR OF THE DOCUMENTATION, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), 
 * STRICT LIABILITY OR OTHERWISE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <SPI.h> 
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>


#include <Servo.h>
#include <AM_CC3000.h>
#include "arduino_secrets.h"


#define ADAFRUIT_CC3000_IRQ   3 
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,SPI_CLOCK_DIVIDER);

#define WLAN_SECURITY   WLAN_SEC_WPA2

/*
 *
 * WiFI Library configuration
 *
 */
char ssid[] = SECRET_SSID;  // your network SSID (name) i.g. "MYNETWORK"
char pass[] = SECRET_PASS;  // your network password i.g. "MYPASSWORD"

Adafruit_CC3000_Server server(80);

/**
 *
 * Other initializations
 *
 */
#define YELLOWLEDPIN 8
int yellowLed = HIGH;

#define TEMPERATUREPIN 0
float temperature;

#define LIGHTPIN 1
int light;

#define SERVOPIN 9
Servo servo;
int servoPos;


#define CONNECTIONPIN 6
int connectionLed = LOW;

#define POTENTIOMETERPIN 2
int pot;

/*
 *
 * Prototypes of AMController’s callbacks
 *
 *
 */
void doWork();
void doSync();
void processIncomingMessages(char *variable, char *value);
void processOutgoingMessages();
void processAlarms(char *variable);
void deviceConnected();
void deviceDisconnected();


/*
 *
 * AMController Library initialization
 *
 */
#ifdef ALARMS_SUPPORT 
  AMController amComtroller(&cc3000,&server,&doWork,&doSync,&processIncomingMessages,&processOutgoingMessages,&processAlarms,&deviceConnected,&deviceDisconnected);
#else
  AMController amController(&server,&doWork,&doSync,&processIncomingMessages,&processOutgoingMessages,&deviceConnected,&deviceDisconnected);
#endif

void setup() {
  
  Serial.begin(9600);

  /* Initialise the module */
  Serial.println(F("\nSetting up"));
  if (!cc3000.begin()) {
    
    Serial.println(F("Check wiring"));
    while(1);
  }
  
  /* Delete any old connection data on the module */
//  Serial.println(F("\nDeleting old connection profiles"));
//  if (!cc3000.deleteProfiles()) {
//    Serial.println(F("Failed!"));
//    while(1);
//  }
  
   /* Setting a static IP Address  */

  if (!cc3000.setStaticIPAddress(cc3000.IP2U32(192, 168, 1, 233),   
                          cc3000.IP2U32(255, 255, 255, 0),  
                          cc3000.IP2U32(192, 168, 1, 1),   
                          cc3000.IP2U32(8, 8, 8, 8))) {
    
    Serial.println(F("Failed to set static IP"));
    while(true);
  }


/* End setting static IP Address*/

  Serial.print(F("Connecting to ")); Serial.print(ssid); Serial.print(F("..."));
  
  if (!cc3000.connectToAP(ssid, pass, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  while (!cc3000.checkDHCP()) {
    
    delay(100); 
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  int i=0;
  while (!displayConnectionDetails()) {
    i++;
    if (i>1)
      Serial.println("Unable to retrieve the IP Address!\r\n");
    delay(500);
  }
  
  // Start listening for connections
  server.begin();
  
  /**
   *
   * Other initializations
   *
   */   

  // Yellow LED on
  pinMode(YELLOWLEDPIN,OUTPUT);
  digitalWrite(YELLOWLEDPIN,yellowLed);


  // Servo position at 90 degrees 
  servo.attach(SERVOPIN);
  servoPos = 0;
  servo.write(servoPos);

  // Red LED OFF
  pinMode(CONNECTIONPIN,OUTPUT);
  digitalWrite(CONNECTIONPIN,connectionLed);
  
  Serial.println(F("Setup Completed"));
}


/**
 * 
 * Standard loop function
 *
 */
void loop() {
  amController.loop(200);
}

/**
*
*
* This function is called periodically and its equivalent to the standard loop() function
*
*/
void doWork() {

  temperature = getVoltage(TEMPERATUREPIN);  //getting the voltage reading from the temperature sensor
  temperature = (temperature - 0.5) * 100;  // converting from 10 mv per degree with 500 mV offset
                                            // to degrees ((voltage – 500mV) times 100  
                                                                                       
  digitalWrite(YELLOWLEDPIN,yellowLed);                                            

  servo.write(servoPos);

  light = analogRead(LIGHTPIN);
  
  pot = analogRead(POTENTIOMETERPIN);  
}


/**
*
*
* This function is called when the ios device connects and needs to initialize the position of switches and knobs
*
*/
void doSync() {
    amController.writeMessage("Knob1",map(servo.read(),0,180,0,1023));
    amController.writeMessage("S1",digitalRead(YELLOWLEDPIN));
    amController.writeTxtMessage("Msg","Hello, I'm your Arduino board");
}

/**
*
*
* This function is called when a new message is received from the iOS device
*
*/
void processIncomingMessages(char *variable, char *value) {  
  
  if (strcmp(variable,"S1")==0) {

    yellowLed = atoi(value);
  }

  if (strcmp(variable,"Knob1")==0) {

      servoPos = atoi(value);  
      servoPos = map(servoPos, 0, 1023, 0, 180);      
  }

  if (strcmp(variable,"Push1") == 0) {
    
    amController.temporaryDigitalWrite(CONNECTIONPIN,LOW,500);
  }

  if (strcmp(variable,"Cmd_01")==0) {
    
    amController.log("Command: "); amController.logLn(value);
    
    Serial.print(F("Cmd: "));
    Serial.println(value);
  } 
  
  if (strcmp(variable,"Cmd_02")==0) {
    
     amController.log("Cmd: "); amController.logLn(value);
     
    Serial.print(F("Cmd: "));
    Serial.println(value);
  } 
  
}

/**
*
*
* This function is called periodically and messages can be sent to the iOS device
*
*/
void processOutgoingMessages() {


  amController.writeMessage("T",temperature);

  amController.writeMessage("L",light);

  amController.writeMessage("Led13",yellowLed);
  
  amController.writeMessage("Pot",pot);
  
}

/**
*
*
* This function is called when a Alarm is fired
*
*/
void processAlarms(char *alarm) {
  
  Serial.print(alarm);
  Serial.println(F(" fired"));
  
  servoPos = 0;
}

/**
*
*
* This function is called when the iOS device connects
*
*/
void deviceConnected () {

  digitalWrite(CONNECTIONPIN,HIGH);
  
  //amController.writeTxtMessage("Msg","Hello, I'm your Arduino board");
  
  Serial.println(F("iOS connected"));
}

/**
*
*
* This function is called when the iOS device disconnects
*
*/
void deviceDisconnected () {

  Serial.println(F("iOS disconnected"));
  digitalWrite(CONNECTIONPIN,LOW);
}

/**
*  
* Auxiliary functions
*
*/

/*
 * getVoltage() – returns the voltage on the analog input defined by pin
 * 
 */
float getVoltage(int pin) {

  return (analogRead(pin) * .004521);  // converting from a 0 to 1023 digital range
                                          // to 0 to 4.56 volts (each 1 reading equals ~ 5 millivolts)
}


bool displayConnectionDetails(void) {

  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
    return false;
  }
  else {
    Serial.print(F("IP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
