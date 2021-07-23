#include <FastSerial.h>
#include <AP_Common.h>
#include <AP_Math.h>            // ArduPilot Mega Vector/Matrix math Library
#include <SPI.h>                // Arduino SPI library
#include <AP_OpticalFlow.h>     // ArduCopter OpticalFlow Library
#include <Streaming.h>          

#include <math.h>

// Constants
//------------------------------------------------------------------------------------
enum {NUM_SENSOR=2};
const int csPin[NUM_SENSOR] = {49,53};  // Chip select pin
const int resetPin[NUM_SENSOR] = {48,47}; // Chip reset pins
const int resolution = 1600;        // allowed values 400,1600 counts/inch
const int frameRate = 2000;         // allowed range [2000,6469]
const int shutterSpeed = 5000;      // allowed arange [1000,9000] clock cycles (where does this come from?)
const int loopDt = 0;               // loop delay (ms)
const int tickOpFlPin = 5; 
const int thImagePin = 4;
int running = 0;
int cont = 0;
const bool showSettings = false;

const int ledPin = LED_BUILTIN;
const int extSigPin = 38;
const int firePin = A0;
const int syncPin = A1; 
boolean  toggle1 = 0;

FastSerialPort0(Serial);        
AP_OpticalFlow_ADNS3080 flowSensor[NUM_SENSOR] = {
    AP_OpticalFlow_ADNS3080(csPin[0]),
    AP_OpticalFlow_ADNS3080(csPin[1])
};

int readInteger(){
   int ret = 0; 
   int done = 0; // False
   int newDigit = 0;
   while (!done){
     
     while(Serial.available()<1){
        ; 
     }
     
     char newChar = Serial.read();
     if (newChar == '*'){ 
       done = 1; // True 
     }
     if (newChar != '*' && newChar > 47){
       newDigit = newChar - '0'; 
       ret = ret*10;
       ret = ret + newDigit;
     }
   }
   return ret;
}

void setTimerInterrupts(int fps)
{  
  int prescale_8_CMR    = (16000000 / (8*fps)) -1; // For FPS going from 31 to 2000
  int prescale_1024_CMR = (16000000 / (1024*fps)) -1; // For FPS going from 1 to 30

  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0

  // set compare match register
  if (prescale_8_CMR > 256 && prescale_8_CMR < 65,536) 
  {
    OCR1A  = prescale_8_CMR; // (must be <65536)
  }
  else
  {
    OCR1A  = prescale_8_CMR;
  }
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);

  // set prescaler 
  if (prescale_8_CMR > 256 && prescale_8_CMR < 65,536) 
  {
    // Set CS11 bit for 8 prescaler
    TCCR1B |= (1 << CS11);  
  }
  else
  {
    // Set CS10 and CS12 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10); 
  }
 
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(extSigPin, OUTPUT);
  pinMode(firePin, OUTPUT);
  pinMode(syncPin, OUTPUT);
  pinMode(tickOpFlPin, OUTPUT);
  pinMode(thImagePin, INPUT);
  
  
  for (int i=0; i<NUM_SENSOR;i++)
    {
        pinMode(resetPin[i],OUTPUT);
        digitalWrite(resetPin[i],HIGH);
    }
    delay(100);
    for (int i=0; i<NUM_SENSOR; i++)
    {
        digitalWrite(resetPin[i],LOW);
    }
    delay(1500);

    // Setup sensor
    // ----------------------------------------------------------------------------
    for (int i=0; i<NUM_SENSOR; i++)
    {
        if( flowSensor[i].init(true) == false ) {
            //Serial << "Sensor initialization failed" << endl;
        }
        flowSensor[i].set_orientation(AP_OPTICALFLOW_ADNS3080_PINS_FORWARD);
        flowSensor[i].set_field_of_view(AP_OPTICALFLOW_ADNS3080_08_FOV);
        flowSensor[i].set_resolution(resolution);        
        flowSensor[i].set_frame_rate_auto(false);
        flowSensor[i].set_frame_rate(frameRate);        
        flowSensor[i].set_shutter_speed_auto(true);
        //flowSensor[i].set_shutter_speed(shutterSpeed);
    }
    delay(1000);

    // Display sensor settings 
    // ----------------------------------------------------------------------------
    if (showSettings)
    {
        for (int i=0; i<NUM_SENSOR; i++)
        {
            Serial << endl;
            Serial << "Sensor settings" << i << endl;
            Serial << "--------------------------------------------------" << endl;
            Serial << "resolution:         " << flowSensor[i].get_resolution() << endl;
            Serial << "frame_rate_auto:    " << flowSensor[i].get_frame_rate_auto() << endl;
            Serial << "frame_rate:         " << flowSensor[i].get_frame_rate() << endl;
            Serial << "shutter_speed_auto: " << flowSensor[i].get_shutter_speed_auto() << endl;
            Serial << "shutter_speed:      " << flowSensor[i].get_shutter_speed() << endl;
            Serial << endl;
        }
    }

  //digitalWrite(ledPin,HIGH);
  
  //int fps = readInteger() * 2;
  //noInterrupts();
  //setTimerInterrupts(fps);
  //interrupts();

  //digitalWrite(ledPin,LOW); 
  
}


ISR(TIMER1_COMPA_vect)
{
  //timer1 interrupt 1Hz toggles pin 13 (LED)
  if (toggle1)
  {
    digitalWrite(firePin,HIGH);
    digitalWrite(syncPin,HIGH);
    toggle1 = 0;
  }
  else
  {
    digitalWrite(firePin,LOW);
    digitalWrite(syncPin,LOW);
    toggle1 = 1;
  }
}

void loop()
{
    digitalWrite(extSigPin,LOW);
    int startCapture = false;
    bool readOpFl = false;
    int fps = 0;
    unsigned long startTime = 0;
    //Serial.println("Starting loop");
    int mode = readInteger();
    //Serial.println(mode);
    int totTimeSec = readInteger();
    unsigned long totTime = (unsigned long)totTimeSec * 1000;
    //Serial.println(totTimeSec);
    //Serial.println(totTime);
    if (mode < 4)
    {
      if (mode > 1)
      {
        readOpFl = true;
      }
      if (mode != 2)
      {
        fps = readInteger() * 2;
        //Serial.println(fps);
      }
      startCapture = true;
    }
    else
    {
      if (mode > 5)
      {
        readOpFl = true;
      }
      if (mode != 6)
      {
        fps = readInteger() * 2;
      }
      while(!startCapture)
      {
        startCapture = digitalRead(thImagePin);
      }
    }
    
    if(fps>0)
    {
      noInterrupts();
      setTimerInterrupts(fps);
      interrupts();

      digitalWrite(extSigPin,HIGH);
    }
    
    startTime = millis(); 
     
    while(startCapture)
    {
      if(readOpFl)
      {
        digitalWrite(tickOpFlPin,HIGH);       
        for (int i=0; i<NUM_SENSOR; i++)
        {           
           flowSensor[i].update();
        }
        digitalWrite(tickOpFlPin,LOW);        
        
        /*for (int i=0; i<NUM_SENSOR; i++)
        {
          Serial << flowSensor[i].raw_dx << ", " << flowSensor[i].raw_dy; 
          if (i < NUM_SENSOR-1)
          {
            Serial << ", ";
          }
        }
        Serial << endl;*/
        Serial << flowSensor[0].raw_dx << ", " << flowSensor[0].raw_dy << ", " << flowSensor[1].raw_dx << ", " << flowSensor[1].raw_dy << ", " << flowSensor[0].last_update << endl;
      }     
      
      if (mode < 4)
      {
        if (millis() - startTime > totTime)
        {
          startCapture = false;
        }
      }
      else
      {
        startCapture = digitalRead(thImagePin);
	delay(1);
	startCapture = startCapture || digitalRead(thImagePin);
      }
      delay(1);
    }
    digitalWrite(extSigPin,LOW);
    Serial << "*" << endl;
    TIMSK1 &= ~(1 << OCIE1A);
    //noInterrupts();
}
