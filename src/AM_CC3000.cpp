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
 * All rights reserved
 *
 */

#include "AM_CC3000.h"

#ifdef ALARMS_SUPPORT

#include <avr/eeprom.h>

AMController::AMController(Adafruit_CC3000 *cc3000,
                             Adafruit_CC3000_Server *server,
                             void (*doWork)(void), 
                             void (*doSync)(void),
                             void (*processIncomingMessages)(char *variable, char *value),
                             void (*processOutgoingMessages)(void),
                             void (*processAlarms)(char *alarm),
                             void (*deviceConnected)(void),
							 void (*deviceDisconnected)(void)
                             )
{
	_var = true;
    _idx = 0;
    _server = server;
    _doWork = doWork;
    _doSync = doSync;
    _processIncomingMessages = processIncomingMessages;
    _processOutgoingMessages = processOutgoingMessages;
    _processAlarms = processAlarms;
    _deviceConnected = deviceConnected;
    _deviceDisconnected = deviceDisconnected;
    
    _variable[0] = '\0';
    _value[0]    = '\0';
        
    _cc3000          = cc3000;    
    _countdown       = 0;  // loop() iterations until next time server query
	_lastPolledTime  = 0L, // Last value retrieved from time server
	_sketchTime      = 1000000L;
	_lastFireCheckTime = 1000000L;
        
	this->inizializeAlarms();
}
#endif 


AMController::AMController(Adafruit_CC3000_Server *server,
                             void (*doWork)(void), 
                             void (*doSync)(void),
                             void (*processIncomingMessages)(char *variable, char *value),
                             void (*processOutgoingMessages)(void),
                             void (*deviceConnected)(void),
							 void (*deviceDisconnected)(void)
                             )
{
	_var = true;
    _idx = 0;
    _server = server;
    _doWork = doWork;
    _doSync = doSync;
    _processIncomingMessages = processIncomingMessages;
    _processOutgoingMessages = processOutgoingMessages;
    _deviceConnected = deviceConnected;
    _deviceDisconnected = deviceDisconnected;
    
    _variable[0] = '\0';
    _value[0]    = '\0';    
}                             

void AMController::loop() {
	this->loop(150);
}

void AMController::loop(unsigned long _delay) {
  	
#ifdef ALARMS_SUPPORT  	

#ifdef DEBUG    	
	//Serial.print(F("C ")); Serial.println((millis()-_sketchTime));    	
#endif		

	if((millis()-_sketchTime) > TIMESYNCHECKINTERVAL) {            	// Time's up?
	
    	unsigned long t  = this->getTime(); 	// Query time server
#ifdef DEBUG    	
		Serial.print(F("Time ")); Serial.println(t);    	
#endif		
    	
    	if(t) {                       	// Success?
      		_lastPolledTime = t;         // Save time
      		_sketchTime     = millis();  // Save sketch time of last valid time query
    	}
  	} 

 	if (_processAlarms != NULL && (millis()-_lastFireCheckTime) > ALARMSCHECKINTERVAL) {
 	
 		_lastFireCheckTime = millis();
		this->checkAndFireAlarms();
    }
#endif  	
  	
 	_doWork();

	//_pClient = &(_server->available());
	Adafruit_CC3000_ClientRef tmp = _server->available();
	_pClient = &tmp;
    
  	if (*_pClient) {
  	
		//Serial.println("Client");  	

    	// Client connected
            
        if (_deviceConnected != NULL) {
        
        	_deviceConnected();	
        	// _sendNtpRequest = true;  // Sync Time
		}
        
    	while(_pClient->connected()) {
            
            //Serial.println("Connected");  
            
            // Read incoming messages if any
            this->readVariable();
            
            if (strcmp(_variable,"Sync") == 0 && strlen(_value)>0) {
                // Process sync messages for the variable _value
                _doSync();
            }
            else {
            
#ifdef ALARMS_SUPPORT             	
            	// Manages Alarm creation and update requests           
            	char id[8];
            	unsigned long time;
            
            	if (strcmp(_variable,"$AlarmId$") == 0 && strlen(_value)>0) {
            	
            		strcpy(id,_value);	
            	} 
            	else if (strcmp(_variable,"$AlarmT$") == 0 && strlen(_value)>0) {
            	
            		time=atol(_value);
            	}
            	else if (strcmp(_variable,"$AlarmR$") == 0 && strlen(_value)>0) {

					if (time == 0)
						this->removeAlarm(id);
					else
            			this->createUpdateAlarm(id,time,atoi(_value));    
            			
            		#ifdef DEBUG
						this->dumpAlarms();
					#endif 
					     		 
            	}
            	else 
#endif            	
				{
#ifdef SDLOGGEDATAGRAPH_SUPPORT

  					if (strlen(_variable)>0 && strcmp(_variable,"$SDLogData$") == 0) {
    
    					this->sdSendLogData(_value);    			
  					}
  				
#endif
            		if (strlen(_variable)>0 && strlen(_value)>0) {
                
	    	            // Process incoming messages
    	    	        _processIncomingMessages(_variable,_value);
            		} 
            	}
            }
            
#ifdef ALARMS_SUPPORT             

            // Check and Fire Alarms when connected
            if (_processAlarms != NULL && (millis()-_lastFireCheckTime) > 30000) {
	            _lastFireCheckTime = millis();
				this->checkAndFireAlarms();
            }      
#endif
            // Write outgoing messages
            _processOutgoingMessages();
              			            
            _doWork();
            
            delay(_delay);
        }
        
        // Client disconnected

        //Serial.println();
        //Serial.println("disconnecting.");
                
    	if (_deviceDisconnected != NULL)
        	_deviceDisconnected();

        _pClient->close();
    }
    else {
//    	Serial.println("!Client");
    }
}

void AMController::readVariable(void) {
	_variable[0]='\0'; 
	_value[0]='\0';
	    
    //Serial.println("readVariable");
    
	while (_pClient->available()) {
        
        char c = _pClient->read();
        
        //Serial.print(c);
        
        if (isprint (c)) {
            
            //Serial.println("isprintf");
            
            if ((char)c == '=') {
                
                _variable[_idx]='\0'; 
                _var = false; 
                _idx = 0;
            }
            else {
                
                if ((char)c == '#') {
                                        
                    _value[_idx]='\0'; 
                    _var = true; 
                    _idx = 0; 
                    
#ifdef SD_SUPPORT	
				if (strcmp(_variable,"SD")==0) {
   
   					//Serial.println("List of Files");
   					
					this->sendFileList();
				}

				if (strcmp(_variable,"$SDDL$")==0) {
    
  					//Serial.print("File: "); Serial.println(_value);
    
   					this->sendFile(_value);
				}
#endif 	                           
                    return;
                }
                else {
                    
                    if (_var) {
                    
                        if(_idx==VARIABLELEN) 
	                        _variable[_idx] = '\0';
	                        else
                        	_variable[_idx++] = c;
                    }
                    else {
                        
                        if(_idx==VALUELEN)
                        	_value[_idx] = '\0';
                        	else
                        	_value[_idx++] = c;
                    }
                }
            }
        }
    }    
}


void AMController::writeMessage(const char *variable, int value) {
  char buffer[VARIABLELEN+VALUELEN+3];
         
	if (_pClient == NULL)
    	return;   
    	
    snprintf(buffer,VARIABLELEN+VALUELEN+3, "%s=%d#", variable, value); 
        
    _pClient->write((const uint8_t *)buffer, strlen(buffer)*sizeof(char));
}

void AMController::writeMessage(const char *variable, float value) {
    char buffer[VARIABLELEN+VALUELEN+3];
    char vbuffer[VALUELEN];
         
	if (_pClient == NULL)
    	return;   
    	
    dtostrf(value, 0, 3, vbuffer);    
    snprintf(buffer,VARIABLELEN+VALUELEN+3, "%s=%s#", variable, vbuffer); 
        
    _pClient->write((const uint8_t *)buffer, strlen(buffer)*sizeof(char));
}

void AMController::writeTripleMessage(const char *variable, float vX, float vY, float vZ) {

    char buffer[VARIABLELEN+VALUELEN+3];
    char vbufferAx[VALUELEN];
    char vbufferAy[VALUELEN];
    char vbufferAz[VALUELEN];
    
    dtostrf(vX, 0, 2, vbufferAx); 
    dtostrf(vY, 0, 2, vbufferAy); 
    dtostrf(vZ, 0, 2, vbufferAz);    
    snprintf(buffer,VARIABLELEN+VALUELEN+3, "%s=%s:%s:%s#", variable, vbufferAx,vbufferAy,vbufferAz); 
        
    _pClient->write((const uint8_t *)buffer, strlen(buffer)*sizeof(char));    
}

void AMController::writeTxtMessage(const char *variable, const char *value) 
{
    char buffer[128];    
        
    snprintf(buffer,128, "%s=%s#", variable, value);
    
    _pClient->write((const uint8_t *)buffer, strlen(buffer)*sizeof(char));
}

void AMController::log(const char *msg) 
{
	this->writeTxtMessage("$D$",msg);
}

void AMController::log(int msg)
{
	char buffer[11];
	itoa(msg, buffer, 10);
	
	this->writeTxtMessage("$D$",buffer);
}


void AMController::logLn(const char *msg) 
{
	this->writeTxtMessage("$DLN$",msg);
}

void AMController::logLn(int msg)
{
	char buffer[11];
	itoa(msg, buffer, 10);
	
	this->writeTxtMessage("$DLN$",buffer);
}

void AMController::logLn(long msg)
{
	char buffer[11];
	ltoa(msg, buffer, 10);
	
	this->writeTxtMessage("$DLN$",buffer);
}

void AMController::logLn(unsigned long msg) {

	char buffer[11];
	ltoa(msg, buffer, 10);
	
	this->writeTxtMessage("$DLN$",buffer);
}

#ifdef SD_SUPPORT

void AMController::sendFileList(void) 
{
 	File root = SD.open("/");
 	root.rewindDirectory();
    
    File entry =  root.openNextFile();
        
    while(entry) {
    
      	if(!entry.isDirectory())
        	this->writeTxtMessage("SD",entry.name());

		entry.close();
      	entry =  root.openNextFile();
    }
    
    root.close();
    
    this->writeTxtMessage("SD","$EFL$");
}


void AMController::sendFile(char *fileName) {
	
    File dataFile = SD.open(_value,FILE_READ);
	
	if (dataFile) {
	
		//unsigned long tot = 0;
	
		this->writeTxtMessage("SD","$C$");
		//_pClient.print("SD=$C$#");
				
		delay(3000);
				
		byte n=0;
		uint8_t buffer[32];

		while(dataFile.available()) {
				
			n = dataFile.read(buffer, sizeof(buffer));			
			_pClient->write(buffer, n*sizeof(uint8_t));	
				
			//tot += n;		
			//Serial.println(tot);
		}

		strcpy((char *)&buffer[0],"SD=$E$#");
		_pClient->write(buffer,7*sizeof(uint8_t));
		
		dataFile.close();
		
		//Serial.println("End File sent");
	}
	else {
		
		//Serial.println("File not available");
	}

}

#endif

void AMController::temporaryDigitalWrite(uint8_t pin, uint8_t value, unsigned long ms) {

	int previousValue = digitalRead(pin);

    digitalWrite(pin, value);
    delay(ms);
    digitalWrite(pin, previousValue);
}


// Time Management 

#ifdef ALARMS_SUPPORT 
/*
void AMController::setNTPServerAddress(IPAddress address) {

	
}
*/

unsigned long AMController::getTime() {

  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;
  Adafruit_CC3000_Client client;

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if(_cc3000->getHostByName("pool.ntp.org", &ip)) {
  
    static const unsigned char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    startTime = millis();
    do {
      client = _cc3000->connectUDP(ip, 123);
    } while((!client.connected()) &&
            ((millis() - startTime) < CONNECTTIMEOUT));

    if(client.connected()) {

      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      Serial.print(F("Time Syncing..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!client.available()) && ((millis() - startTime) < RESPONSETIMEOUT));
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
        Serial.print(F("OK\r\n"));
      }
      client.close();
    }
  }
  if(!t) 
  	Serial.println(F("Error"));
  return t;
}

unsigned long AMController::now() {
	
	unsigned long now = _lastPolledTime + (millis() - _sketchTime) / 1000;
	
	return now;
}


#ifdef DEBUG
void AMController::printTime(unsigned long time) {

		Serial.print(time);	   	
}
#endif

void AMController::createUpdateAlarm(char *id, unsigned long time, bool repeat) {

	char lid[12];
	
	lid[0] = 'A';
	strcpy(&lid[1],id);

	// Update

noInterrupts();	

	for(int i=0; i<5; i++) {
		
		alarm a;
		
		eeprom_read_block((void*)&a, (void*)(i*sizeof(a)), sizeof(a));
		
		if (strcmp(a.id,lid) == 0) {
		
			a.time = time;
			a.repeat = repeat;
				
			eeprom_write_block((const void*)&a, (void*)(i*sizeof(a)), sizeof(a));				
				
			interrupts();
			
			return;				
		}		
	}

	// Create

	for(int i=0; i<5; i++) {
	
		alarm a;

		eeprom_read_block((void*)&a, (void*)(i*sizeof(a)), sizeof(a));
	
		if(a.id[1]=='\0') {
		
			strcpy(a.id,lid);
			a.time = time;
			a.repeat = repeat;
		
			eeprom_write_block((const void*)&a, (void*)(i*sizeof(a)), sizeof(a));	
			
			interrupts();
			
			return;
		}
	}	
interrupts();	
}

void AMController::removeAlarm(char *id) {

	char lid[12];
	
	lid[0] = 'A';
	strcpy(&lid[1],id);

	for(int i=0; i<5; i++) {
	
		alarm a;
		
		eeprom_read_block((void*)&a, (void*)(i*sizeof(a)), sizeof(a));
	
		if(strcmp(a.id,lid) == 0) {
		
			a.id[1]='\0';
			a.time = 0;
	        a.repeat = 0;
			
			eeprom_write_block((const void*)&a, (void*)(i*sizeof(a)), sizeof(a));
		}
	}
}

void AMController::inizializeAlarms() {

	for(int i=0; i<5; i++) {
	
		alarm a;
		
		eeprom_read_block((void*)&a, (void*)(i*sizeof(a)), sizeof(a));
	
		if(a.id[0] != 'A') {
		
			a.id[0]='A';
			a.id[1]='\0';
			a.time=0;
			a.repeat = 0;
			
			eeprom_write_block((const void*)&a, (void*)(i*sizeof(a)), sizeof(a));
		}
	}
}

#ifdef DEBUG
void AMController::dumpAlarms() {

	Serial.println(F("\t----Dump Alarms -----")); 
	
	for(int i=0;i<5; i++) {

		alarm al;
				
noInterrupts();					
		eeprom_read_block((void*)&al, (void*)(i*sizeof(al)), sizeof(al));
interrupts();

		Serial.print(F("\t"));
    	Serial.print(al.id); 
    	Serial.print(F(" ")); 
    	this->printTime(al.time);
    	Serial.print(F(" "));
    	Serial.println(al.repeat);
	}
}
#endif

void AMController::checkAndFireAlarms() {

	unsigned long now = this->now();

#ifdef DEBUG
		Serial.print(F("checkAndFireAlarms "));
	    this->printTime(now);
	    Serial.println();
#endif
	    for(int i=0; i<5; i++) {

			alarm a;

			eeprom_read_block((void*)&a, (void*)(i*sizeof(a)), sizeof(a));

	    	if(a.id[1]!='\0' && a.time<now) {

				// First character of id is A and has to be removed
            	_processAlarms(&a.id[1]);

	    		if(a.repeat) {
	    	
    	    		a.time += 86400; // Scheduled again tomorrow
    	    		
#ifdef DEBUG
					Serial.print(F("Alarm rescheduled at "));
	            	this->printTime(a.time);
		            Serial.println();	
#endif		            
            	}
	        	else {
            		//     Alarm removed
            		
	            	a.id[1]='\0';
	            	a.time = 0;
	            	a.repeat = 0;
            	}

				eeprom_write_block((const void*)&a, (void*)(i*sizeof(a)), sizeof(a));
#ifdef DEBUG
				this->dumpAlarms();
#endif        		 
			}
    	}
//	}
}
#endif

#ifdef SDLOGGEDATAGRAPH_SUPPORT

void AMController::sdLogLabels(const char *variable, const char *label1) {

	this->sdLogLabels(variable,label1,NULL,NULL,NULL,NULL);
}

void AMController::sdLogLabels(const char *variable, const char *label1, const char *label2) {

	this->sdLogLabels(variable,label1,label2,NULL,NULL,NULL);
}

void AMController::sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3) {

	this->sdLogLabels(variable,label1,label2,label3,NULL,NULL);
}

void AMController::sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4) {

	this->sdLogLabels(variable,label1,label2,label3,label4,NULL);
}

void AMController::sdLogLabels(const char *variable, const char *label1, const char *label2, const char *label3, const char *label4, const char *label5) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile) 
  	{
    	dataFile.print("-");
    	dataFile.print(";");
    	dataFile.print(label1);
    	dataFile.print(";");

    	if(label2 != NULL)
	    	dataFile.print(label2);
	    else
	    	dataFile.print("-");
    	dataFile.print(";");
    	
    	if(label3 != NULL)
	    	dataFile.print(label3);
	    else
	    	dataFile.print("-");
    	dataFile.print(";");
    	
    	if(label4 != NULL)
	    	dataFile.print(label4);
	    else
	    	dataFile.print("-");
    	dataFile.print(";");
    	
    	if(label5 != NULL)
	    	dataFile.println(label5);
	    else
	    	dataFile.println("-");
    
	    dataFile.println("\n");
	    
    	dataFile.flush();
    	dataFile.close();
  	}
}


void AMController::sdLog(const char *variable, unsigned long time, float v1) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile) 
  	{
    	dataFile.print(time);
    	dataFile.print(";");
    	dataFile.print(v1);
     
    	dataFile.print(";-;-;-;-\n");
    	    
    	dataFile.flush();
    	dataFile.close();
  	}
}

void AMController::sdLog(const char *variable, unsigned long time, float v1, float v2) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile && time>0) 
  	{
    	dataFile.print(time);
    	dataFile.print(";");
    	dataFile.print(v1);
    	dataFile.print(";");

    	dataFile.print(v2);
     
    	dataFile.print(";-;-;-\n");
    	    
    	dataFile.flush();
    	dataFile.close();
  	}
}

void AMController::sdLog(const char *variable, unsigned long time, float v1, float v2, float v3) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile && time>0) 
  	{
    	dataFile.print(time);
    	dataFile.print(";");
    	dataFile.print(v1);
    	dataFile.print(";");

    	dataFile.print(v2);
    	dataFile.print(";");
     
       	dataFile.print(v3);
    	
    	dataFile.print(";-;-\n");
    	        
    	dataFile.flush();
    	dataFile.close();
  	}
}

void AMController::sdLog(const char *variable, unsigned long time, float v1, float v2, float v3, float v4) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile && time>0) 
  	{
    	dataFile.print(time);
    	dataFile.print(";");
    	dataFile.print(v1);
    	dataFile.print(";");

    	dataFile.print(v2);
    	dataFile.print(";");
     
       	dataFile.print(v3);
    	dataFile.print(";");
    	
    	dataFile.print(v4);
    	    
	    dataFile.println(";-\n");
    
    	dataFile.flush();
    	dataFile.close();
  	}
}

void AMController::sdLog(const char *variable, unsigned long time, float v1, float v2, float v3, float v4, float v5) {

	File dataFile = SD.open(variable, FILE_WRITE);
  
  	if (dataFile && time>0) 
  	{
    	dataFile.print(time);
    	dataFile.print(";");
    	dataFile.print(v1);
    	dataFile.print(";");

    	dataFile.print(v2);
    	dataFile.print(";");
    	
    	dataFile.print(v3);
    	dataFile.print(";");
    	
    	dataFile.print(v4);
    	dataFile.print(";");
    	
    	dataFile.println(v5);
    	
    	dataFile.println("\n");
    
    	dataFile.flush();
    	dataFile.close();
  	}
}
	
void AMController::sdSendLogData(const char *variable) {

//Serial.print("sdSendLogData for ");
//Serial.println(variable);

int rows = 0;

	cli();
    
  	File dataFile = SD.open(variable, FILE_READ); 
  	if (dataFile) {
    
    	char c;
    	char buffer[128];
    	int i = 0;
                        
    	dataFile.seek(0);
    	
    	
    	while( (c=dataFile.read()) != -1 ) {
      
      		if (c == '\n') {
        
        		buffer[i++] = '\0';
        
		        this->writeTxtMessage(variable,buffer); 
        
        		i=0;
        		
        		rows++;
      		}
      		else {      		
        		buffer[i++] = c;
        	}
        	
        	//delay(5);
    	}    	

		dataFile.close();
  	}
  
  	this->writeTxtMessage(variable,"");
  
  	sei();  	
}	

void AMController::sdPurgeLogData(const char *variable) {
	
	cli();
	
	SD.remove(variable);
	
	sei();
}
	
#endif	