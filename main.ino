#include <Keypad.h>

//initializing info - These are the only parameters that would change per building.
const int exitCount = 6;
char exitName[exitCount] = {'A', 'B', 'C', 'D', 'E', 'F'};
int BuildingInfo[2][exitCount]= {
  {3, 2, 1, 2, 1, 4},
  {0, 0, 0, 0, 0, 0} 
};
int RateOfSpread = 1;
int sensor1Dist[exitCount] = {0, 40, 230, 140, 200, 100};  //distance of exits from various sensors
int sensor2Dist[exitCount] = {190, 50, 100, 0, 130, 240};
int sensor3Dist[exitCount] = {90, 250, 130, 140, 0, 100};


//global optimization variables 
int maxPplPerExit[exitCount];
int maxPpl;
float idealTime;
int find_idealTimeVal;


//main output array
int ppl[exitCount];

//initializing smoke sensors
int sensor_1 = A5;     
int gasReading_1 = 0;
int led1 = 4;

int sensor_2 = A4;     
int gasReading_2 = 0;
int led2 = 3;

int sensor_3 = A3;     
int gasReading_3 = 0;
int led3 = 2;

bool smoke_trigger = false;



//initializing ppl counter through keypad
int pplCount = 0;
char currentKey=' ';

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};  
byte rowPins[ROWS] = {11, 10, 9, 8}; 
byte colPins[COLS] = {7, 6, 5}; 

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
 
void setup(){
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  Serial.begin(9600);
  
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
}

void loop() { 
  
  
  //gas sensor code
  gasReading_1 = analogRead(sensor_1);     
  gasReading_2 = analogRead(sensor_2);   
  gasReading_3 = analogRead(sensor_3);   
  
  if (gasReading_1 > 450)
  {
    for (int p = 0; p < exitCount; p++){
      //input distance is changed to that of the triggered sensor
      BuildingInfo[1][p] = sensor1Dist[p];  
    }
    digitalWrite(led1, HIGH);
    Serial.println("SMOKE ALARM 1 TRIGGERED!");
    smoke_trigger = true;
  } 
  
  else if (gasReading_2 > 450)
  {
    for (int q = 0; q < exitCount; q++){
      BuildingInfo[1][q] = sensor2Dist[q];
    }
    digitalWrite(led2, HIGH);
    Serial.println("SMOKE ALARM 2 TRIGGERED!");
    smoke_trigger = true;
  } 
  
  else if (gasReading_3 > 450)
  {
    for (int r = 0; r < exitCount; r++){
      BuildingInfo[1][r] = sensor3Dist[r];
    }
    digitalWrite(led3, HIGH);
    Serial.println("SMOKE ALARM 3 TRIGGERED!");
    smoke_trigger = true;
  }
 
  
  //code for finding total no. of ppl using keypad
  /*
    People enter a '*' and the number of ppl exiting OR a '#' and the 
    number of ppl entering to log it in the system
    
    NOTE: This does not apply after the alarm goes off, 
    pplCount becomes static once the alarm is triggered.
  */
  
  char lkey = keypad.getKey();
  if (lkey != NO_KEY && ((int)lkey-48)>0){
    
    if (currentKey=='*') {
    	pplCount = pplCount - ((int)lkey-48);
    }
    if (currentKey=='#') {
      	pplCount = pplCount + ((int)lkey-48);
    }
    currentKey=' ';
    Serial.println(pplCount);
  }
  if (lkey != NO_KEY && lkey == '*')
  {
    Serial.print(lkey);
    currentKey=lkey;
  }
  if (lkey != NO_KEY && lkey == '#')
  {
    Serial.print(lkey);
    currentKey=lkey;
  }
  

  /*
  Here we have two main aims:
     1. To minimize evacuation time based on the rate that the smoke/gas spreads
     2. To indicate an optimum no. of ppl per exit to reduce the chances 
        of a stampede and keep the time required to a minimum without exposing 
        ppl neer the exits to where the hazard has started to any smoke/gas.
        
   >> This meant that there was only a certain safe time frame in 
      which people can use a said exit.
      
   >> This info was used to find out the maximum people who can exit the building
      absoluely safely based on how far the exits are from the triggered sensor 
      and the exit width.
  */
  
 //static values for optimization math
  for(int i = 0; i<exitCount; i++){
    maxPplPerExit[i] = BuildingInfo[0][i]*BuildingInfo[1][i]/RateOfSpread;
    maxPpl = maxPpl + maxPplPerExit[i];
    if (BuildingInfo[1][i] != 0){
      find_idealTimeVal = find_idealTimeVal + BuildingInfo[0][i];
  }
 }
 
  //dynamic optimization code
  if (smoke_trigger){
    if (maxPpl > pplCount){
      
      idealTime = pplCount/find_idealTimeVal;
      int pplFilled=0;
    
      for (int j = 0; j<exitCount; j++){
        /*
        idealTime and idealPpl refer to the value of time it would 
        have taken for ppl to evacuate not counting any gas spread limitation
        and the no. of ppl that would've exited through each exit in this situation.
        */
        float idealPpl = idealTime*BuildingInfo[0][j];  
        idealPpl = (int)idealPpl;
      
        /*here we evaluate if we can take in the ideal value without 
        crossing the max value because that would be the best possible situation*/
        if (idealPpl >= maxPplPerExit[j]){
          ppl[j] = maxPplPerExit[j];
        }
        else {
          ppl[j] = idealPpl;
        }
        pplFilled = pplFilled + ppl[j];
      }
      int pplLeft = pplCount - pplFilled; 
      //^^here^^ we have some values that had crossed max, so they were reduced, 
      //therefore, now, the whole pplCount hasn't yet been accomodated.
     
    
    
      /*
      Now we try to find out the optimum exits where we can direct the 
      rest of the ppl who couldn't be accomodated in ideal values while 
      ensuring the least possible increase in evacuation time.
      */
      int pplFilledWhileloop = 1;
      while (pplFilledWhileloop>0){
        pplFilledWhileloop = 0;
        for (int k = 0; k<exitCount; k++){
      	  int tmpfilled = int(pplLeft*BuildingInfo[0][k]/find_idealTimeVal);
          ppl[k] = ppl[k] + tmpfilled;
          if (ppl[k] > maxPplPerExit[k]){
            ppl[k] = maxPplPerExit[k];
          } else {
            	pplFilledWhileloop = pplFilledWhileloop + tmpfilled;
          }
        }
        pplLeft = pplLeft - pplFilledWhileloop;
      }
    
      int highestWidth = 0;
      int indexhighestWidth = 0;
      for (int m = 0; m<exitCount; m++){
        if (BuildingInfo[0][m] > highestWidth){
          highestWidth = BuildingInfo[0][m];
          indexhighestWidth = m;
        }
      } 
    
      ppl[indexhighestWidth] = ppl[indexhighestWidth] + pplLeft;    
    }
    else {
      Serial.println("ERROR: BUILDING EXCEEDED MAXIMUM SAFE CAPACITY!");
    }
  }
  if (smoke_trigger){
  //optimization output
    for (int n = 0; n<exitCount; n++){
      Serial.print("People who should exit from exit ");
      Serial.print(exitName[n]);
      Serial.print(" is: ");
      Serial.println(ppl[n]);
      Serial.println(" ");
    }
    Serial.println(" ");
  }
  
  smoke_trigger = false;
    
  delay(5000);
  
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
}

/*
  Accuracy of our optimization algorithm:
  
  We tested our algorithm out 10-12 times with different values and compared our results 
  with official GRG Non Linear Solver on excel that uses an advanced derivative 
  based computing technique.
  
  Every time, our results came out within +- 0.6% of the GRG results, 
  hence verifying our accuracy to a certin extent.
  
  Here's one of the result tables for reference. 
  (pplCount: 800, building info same as the example in this code, sensor triggered: sensor 1)
  
  Exits   |   GRG Solver results   |   Our Results
    A                0 ppl                0 ppl
    B               80 ppl               80 ppl
    C               90 ppl               89 ppl
    D              180 ppl              180 ppl
    E               90 ppl               89 ppl
    F              360 ppl              362 ppl
  --------------------------------------------------
   Time:            90sec               90.5sec
  --------------------------------------------------
*/
