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
 * Author: Fabrizio Boco - fabboco@gmail.com
 *
 * Version: 1.1.0
 *
 * All rights reserved
 *
 */

#ifndef AM_CC3000_H
#define AM_CC3000_H

#define ALARMS_SUPPORT    // uncomment to enable support for Alarm Widget
//#define SD_SUPPORT        // uncomment to enable support for SD Widget - Download only
//#define SDLOGGEDATAGRAPH_SUPPORT    // uncomment to enable support for Logged Data Widget
#define DEBUG             // uncomment to enable debugging - You should not need it !

#include <Adafruit_CC3000.h>

#ifdef SD_SUPPORT
	#include <SD.h>
#endif 

#ifdef ALARMS_SUPPORT

#define  CONNECTTIMEOUT  		15000L 
#define  RESPONSETIMEOUT 		25000L
#define  TIMESYNCHECKINTERVAL	3600000L  // once every hour [ms]
#define  ALARMSCHECKINTERVAL	30000L    // once every 30'' [ms]

typedef struct  {
	char 			id[12];  // First character of id is always A
	unsigned long 	time;
	bool			repeat;
} alarm;

#endif

#define VARIABLELEN 14
#define VALUELEN    18

class AMController {

private:
char						_variable[VARIABLELEN+1];
char 	   					_value[VALUELEN+1];
bool	   					_var;
int       					_idx;
Adafruit_CC3000_Server 		*_server;
Adafruit_CC3000_ClientRef  	*_pClient;

#ifdef ALARMS_SUPPORT
Adafruit_CC3000  			*_cc3000;
int 						_countdown;      // loop() iterations until next time server query
unsigned long    			_lastPolledTime; // Last value retrieved from time server
unsigned long    			_lastFireCheckTime;
unsigned long    			_sketchTime;
#endif

#ifdef SD_SUPPORT
/*
bool			_upload;
File			_dataFile;
unsigned long 	_fileLen;
unsigned long	_expectedFileLen;
*/
#endif

/**
Pointer to the function where to put code in place of loop()
**/
void (*_doWork)(void); 

/*
Pointer to the function where Switches, Knobs and Leds are syncronized
*/
void (*_doSync)(void);

/*
Pointer to the function where incoming messages are processed
*
* variable
*
* value
*
*/
void (*_processIncomingMessages)(char *variable, char *value);

/*
Pointer to the function where outgoing messages are processed
*
*/
void (*_processOutgoingMessages)(void);

#ifdef ALARMS_SUPPORT
/*
Pointer to the function where alerts are processed
*
*/
void (*_processAlarms)(char *alarm);
#endif

/*
Pointer to the function called when a device connects to Arduino
*
*/
void (*_deviceConnected)(void);

/*
Pointer to the function called when a device disconnects from Arduino
*
*/
void (*_deviceDisconnected)(void);

void readVariable(void);

#ifdef ALARMS_SUPPORT

//void breakTime(unsigned long time, int *seconds, int *minutes, int *hours, int *Wday, long *Year, int *Month, int *Day);

void inizializeAlarms();
void checkAndFireAlarms();
void createUpdateAlarm(char *id, unsigned long time, bool repeat);
void removeAlarm(char *id);	

#endif

public:

#ifdef ALARMS_SUPPORT
	AMController(Adafruit_CC3000 *cc3000,
								  Adafruit_CC3000_Server *server,
								  void (*doWork)(void), 
								  void (*doSync)(void), 
								  void (*processIncomingMessages)(char *variable, char *value),
								  void (*processOutgoingMessages)(void),
								  void (*processAlarms)(char *alarm),
								  void (*deviceConnected)(void),
								  void (*deviceDisconnected)(void));
#endif

	AMController(Adafruit_CC3000_Server *server,
								  void (*doWork)(void), 
								  void (*doSync)(void), 
								  void (*processIncomingMessages)(char *variable, char *value),
								  void (*processOutgoingMessages)(void),
								  void (*deviceConnected)(void),
								  void (*deviceDisconnected)(void));
								  
								  
	void loop();
	void loop(unsigned long delay);
	void writeMessage(const char *variable, int value);
	void writeMessage(const char *variable, float value);
	void writeTripleMessage(const char *variable, float vX, float vY, float vZ);	
	void writeTxtMessage(const char *variable, const char *value);
	
	void log(const char *msg);
	void log(int msg);

	void logLn(const char *msg);
	void logLn(int msg);
	void logLn(long msg);
	void logLn(unsigned long msg);
	
#ifdef SD_SUPPORT	
	void sendFileList(void);
	void sendFile(char *fileName);
#endif
	
	void temporaryDigitalWrite(uint8_t pin, uint8_t value, unsigned long ms);
	
#ifdef ALARMS_SUPPORT 
    unsigned long getTime();
	unsigned long now();
#ifdef DEBUG
	void dumpAlarms();
  	void printTime(unsigned long time);
#endif 
#endif 

#ifdef SDLOGGEDATAGRAPH_SUPPORT

	void sdLogLabels(const char *variable, const char *label1);
	void sdLogLabels(const char *variable, const char *label1, const char *label2);
	void sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3);
	void sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4);
	void sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5);

	void sdLog(const char *variable, unsigned long time, float v1);
	void sdLog(const char *variable, unsigned long time, float v1, float v2);
	void sdLog(const char *variable, unsigned long time, float v1, float v2, float v3);
	void sdLog(const char *variable, unsigned long time, float v1, float v2, float v3, float v4);
	void sdLog(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5);
	
	void sdSendLogData(const char *variable);
	
	void sdPurgeLogData(const char *variable);
	
#endif	

};

#endif

