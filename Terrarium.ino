  //Libraries
#include <DHT.h>

//Constants
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino


//Variables

bool DEBUG = false;

const int heatOutPin = 12;  
const int humidOutPin = 13; 
const int lightOutPin = 4; 
const int humidWarningPin = 8; 

const int heatSettingPin = A0;  
const int humidSettingPin = A1; 

const int lightReadingPin = A2; 
const int waterReadingPin = A3; 

float humidityReading = 99;  //Stores humidity value
float tempReading = 99; //Stores temperature value

float humiditySetting = 0;
float tempSetting = 0;

float maxHeat = 93;
float minHeat = 20;

float maxHumid = 100;
float minHumid = 10;

float waterThreshold = 1020;
float lightOnThreshold = 500;
float lightOffThreshold = 700;

float timeSinceLastHeat = 300;
float timeSinceLastWater = 300;
float wateringTime = 0;

float timePerLoop = 0.5;

int waterLevelGoodBuffer = 10;

int heatBuffer = 3;

int wateringWait = 300;
int wateringAfterHeatWait = 60;
int heatingWait = 5;

void setup()
{
  Serial.begin(9600);

  pinMode(heatOutPin, OUTPUT); 
  pinMode(humidOutPin, OUTPUT); 
  pinMode(humidWarningPin, OUTPUT); 
  pinMode(lightOutPin, OUTPUT); 
  
  pinMode(heatSettingPin, INPUT); 
  pinMode(humidSettingPin, INPUT); 
  pinMode(lightReadingPin, INPUT); 
  pinMode(waterReadingPin, INPUT); 
  
  dht.begin();

  digitalWrite(heatOutPin, LOW);
  digitalWrite(humidOutPin, LOW);
  digitalWrite(humidWarningPin, LOW);
  digitalWrite(lightOutPin, LOW);
  
  delay(1000);
}

void loop()
{
    //Read data and store it to variables humidityReading and tempReading
    humidityReading = dht.readHumidity();
    tempReading= dht.readTemperature();

    float tempSettingRaw = analogRead(heatSettingPin);                                          //Read and save analog value from potentiometer
    tempSetting = map(tempSettingRaw, 0, 1023, minHeat, maxHeat);                               //Map value 0-1023 to min-max

    float humiditySettingRaw = analogRead(humidSettingPin);                                          //Read and save analog value from potentiometer
    humiditySetting = map(humiditySettingRaw, 0, 1023, minHumid, maxHumid);                               //Map value 0-1023 to min-max

    float lightReadRaw = analogRead(lightReadingPin); 
    float waterReadingRaw = analogRead(waterReadingPin);                              

    bool waitingForColdLamp = false;
    bool waterLevelLow = false;

    if(humidityReading < humiditySetting && timeSinceLastWater > wateringWait && wateringTime < 5)
    {
      waitingForColdLamp = (timeSinceLastHeat < wateringAfterHeatWait);
      waterLevelLow = waterReadingRaw < waterThreshold;
      
      //This is to mitigate temporary misreadings in the water level sensor. Sometimes it will spike up, this ensures that the water level has been good for at least 5 seconds before turning on
      if(waterLevelLow)
      {
        waterLevelGoodBuffer = 0;
      }
      else
      {
        waterLevelGoodBuffer = waterLevelGoodBuffer + 1;
      }

      if(DEBUG)
        {
          Serial.print("WATER LEVEL BUFFER ");
          Serial.println(waterLevelGoodBuffer);
        }
      
      if(waterLevelLow || waterLevelGoodBuffer < 10)  //Pump on but water level is too low
      {
        digitalWrite(humidOutPin, LOW);
        digitalWrite(humidWarningPin, HIGH);

        timeSinceLastWater = timeSinceLastWater + timePerLoop;
        wateringTime = 0;

        if(DEBUG)
        {
          Serial.println("PUMP OFF - NO WATER");
        }
      }
      else if(waitingForColdLamp)  //Pump on but water level is too low
      {
        digitalWrite(humidOutPin, LOW);
        digitalWrite(humidWarningPin, HIGH);

        timeSinceLastWater = timeSinceLastWater + timePerLoop;
        wateringTime = 0;

        if(DEBUG)
        {
          Serial.println("PUMP OFF - LAMP HOT");
        }
      }
      else
      {
        digitalWrite(humidOutPin, HIGH);
        digitalWrite(humidWarningPin, LOW);

        wateringTime = wateringTime + timePerLoop;
        if(wateringTime >= 5)
        {
          timeSinceLastWater = 0;
        }

        if(DEBUG)
        {
          Serial.println("PUMP ON");
        }
      }
    }
    else
    {
      digitalWrite(humidOutPin, LOW);
      digitalWrite(humidWarningPin, LOW);
      
      timeSinceLastWater = timeSinceLastWater + timePerLoop;
      wateringTime = 0;
      
      if(DEBUG)
      {
        Serial.println("PUMP OFF");
      }
    }

    if(tempReading < (tempSetting - heatBuffer) && !waitingForColdLamp && timeSinceLastWater > heatingWait)
    {
      digitalWrite(heatOutPin, HIGH);
      timeSinceLastHeat = 0;
      
      if(DEBUG)
      {
        Serial.println("HEATER ON");
      }
    }
    else
    {
      digitalWrite(heatOutPin, LOW);
      timeSinceLastHeat = timeSinceLastHeat + timePerLoop;
      
      if(DEBUG)
      {
        Serial.println("HEATER OFF");
      }
    }

    if(lightReadRaw < lightOnThreshold && tempReading < (tempSetting - heatBuffer))
    {
      digitalWrite(lightOutPin, HIGH);
      
      if(DEBUG)
      {
        Serial.println("LIGHT ON");
      }
    }
    else if(lightReadRaw > lightOffThreshold || tempReading >= (tempSetting + heatBuffer))
    {
      digitalWrite(lightOutPin, LOW);
      
      if(DEBUG)
      {
        Serial.println("LIGHT OFF");
      }
    }

    if(DEBUG)
    {
      //Print temp and humidity values to serial monitor
      Serial.print("Humidity: ");
      Serial.print(humidityReading);
      Serial.print(" %, Temp: ");
      Serial.print(tempReading);
      Serial.println(" Celsius");
  
      
      Serial.print("Humidity setting: ");
      Serial.print(humiditySetting);
      Serial.print(" %, Temp setting: ");
      Serial.print(tempSetting);
      Serial.println("");
  
      Serial.print("Light level reading: ");
      Serial.print(lightReadRaw);
      Serial.print("  Water level reading: ");
      Serial.println(waterReadingRaw);
    }

    delay(timePerLoop*1000);
}

   
