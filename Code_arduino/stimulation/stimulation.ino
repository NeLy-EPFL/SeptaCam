// Constants
//------------------------------------------------------------------------------------

const int trigStimulus = 12;
const int LED = 44;
const int extSigPin = 38;
const int buttonPin =40;
int buttonState;

int readInteger(){
   int ret = 0;
   int done = 0; // False
   int newDigit = 0;
   while (!done){

     while(Serial.available()<1){
        singleShot();
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

/*int[] readArray(int actSeq[]){
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
     if (newChar == ','){
       
     }
   }
   return ret;
}*/

void setup() {

  Serial.begin(115200);
  pinMode(trigStimulus,OUTPUT);
  pinMode(LED,OUTPUT);
  pinMode(extSigPin,INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop()
{
    digitalWrite(trigStimulus,LOW);
    digitalWrite(LED,LOW);
    int startCapture = false;
    unsigned long startTime;
    int numLoops = readInteger();
    int numStim = readInteger();
    //Serial.println(numLoops);
    //Serial.println(numStim);

    int seqLength = numStim+1;
    unsigned long actSeq[seqLength];
    unsigned long ret = 0;
    unsigned long timeDiff = 0;
    int done = 0;
    int newDigit = 0;
    int cont = 0;
    
    while (!done){
      while(Serial.available()<1){
         ;
      }

      char newChar = Serial.read();
      if (newChar != '*' && newChar > 47){
        newDigit = newChar - '0';
        ret = ret*10;
        ret = ret + newDigit;
      }
      if (newChar == ','){
        actSeq[cont] = ret;
        ret=0;
        cont+=1;
      }
      if (newChar == '*'){
        actSeq[cont] = ret;
        ret=0;
        done = 1; // True 
      }
   }
   
   //for(int a= 0; a < numStim; a++){
   //   Serial.println(a);
   //   Serial.println(actSeq[a]);
   //}
    
//Uncomment for activating sequence using an external signal
   while(!startCapture)
   {
     singleShot();
     startCapture = digitalRead(extSigPin);
     //Serial.println("Waiting");
   }

   for(int i= 0; i < numLoops; i++){
      for(int j = 0; j < numStim; j++){
       startTime = millis();
       timeDiff = 0;
       
       if(j%2 == 0){
          digitalWrite(trigStimulus,LOW);
          digitalWrite(LED,LOW);
       }
       else{
          digitalWrite(trigStimulus,HIGH);
          digitalWrite(LED,HIGH);
       }
        
       while(timeDiff < actSeq[j]){        
           timeDiff = millis() - startTime;
           //Serial.println(timeDiff);
       }
      }
   }
   digitalWrite(trigStimulus,LOW);
   digitalWrite(LED,LOW);
   //Serial.println("*");
}

void singleShot(){
   buttonState = digitalRead(buttonPin);

   if(!buttonState){
     delay(30);
     if (!buttonState){
        digitalWrite(trigStimulus,HIGH);
        digitalWrite(LED,HIGH);
     }
   }
   else{
        digitalWrite(trigStimulus,LOW);
        digitalWrite(LED,LOW);
   }
}

