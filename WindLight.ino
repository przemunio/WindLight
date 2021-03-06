int relayOut = 2;
int buzzerOut = 3;
int lightDetectorIn = A0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(relayOut, OUTPUT);
  pinMode(buzzerOut, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

enum State {
  ZERO,
  INIT,
  LIGHT_DETECTED,
  LIGHT_DETECTED_RELAY_ON,
  RELAY_ON
};


const unsigned long triggerRelayTimeMs = ((unsigned long)5)*60*1000;//5 minutes
const unsigned long maxTimeToEnableTriggerMinutes = 20*60;//20 minutes

State state = ZERO;

void shortBeep(int del)
{
    digitalWrite(buzzerOut,HIGH);
    delay(del);
    digitalWrite(buzzerOut,LOW);
}

void relayOn()
{
  digitalWrite(relayOut,HIGH);
}

void relayOff()
{
  digitalWrite(relayOut,LOW);
}

void ledOn()
{
    digitalWrite(LED_BUILTIN,HIGH);
}

void ledOff()
{
    digitalWrite(LED_BUILTIN,LOW);
}



bool isLightDetected()
{  
  const unsigned long debounceDelay = 500;    // the debounce time; increase if the output flickers

  static unsigned long lastReading = 0;
  static bool lastState = false;   // the previous reading from the input pin
  static bool state = false;
  static unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled


  if((millis() - lastReading) < 100)
    return state;

  lastReading = millis();
  
  
  bool reading = analogRead(lightDetectorIn) > 30;

  if (reading != lastState) {
    lastDebounceTime = millis();
    Serial.print("Debounce: different state, reading: ");
    Serial.print(reading);
    Serial.print(" last state: ");
    Serial.print(lastState);
    Serial.print(" STATE: ");
    Serial.println(state);
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != state) {
            Serial.print("Debounce: time!, reading: ");
            Serial.print(reading);
        
            Serial.print(" STATE: ");
            Serial.println(state);
            state = reading;
      }
  }

  lastState = reading;


  return state;
}

bool isDarkDetected()
{
  return !isLightDetected();
}

void alive()
{
  unsigned long static lastChangeTime = 0;
  bool static isOn = false;

  if(millis() - lastChangeTime > 1000)
  {
    lastChangeTime = millis();
    isOn = !isOn;
    
    if(isOn)
      ledOn();
    else
      ledOff(); 

     int val = analogRead(A0);
     Serial.print("Wartosc detektora:");
     Serial.println(val);
  }
}

unsigned long lightStartTime;
unsigned long lightEndTime;

void loop() {
  // put your main code here, to run repeatedly:
  alive();
  
  switch(state)
  {
    case ZERO:
      Serial.println("Startujemy!!!");
      relayOff();
      shortBeep(100);
      delay(50);
      shortBeep(100);
      delay(1000);
      state = INIT;
      break;
    case INIT:
      relayOff();
      if(isLightDetected())//wykryl swiatlo
      {
        Serial.println("Swiatlosc nastapila - zapalenie za 5 minut");
        lightStartTime = millis();        
        shortBeep(50);
        state = LIGHT_DETECTED;
      }
      break;
    case LIGHT_DETECTED:
      if(isDarkDetected())//zgaszone
      {
        Serial.print("Ciemnosc widze do odpalenia wiatraczka zostalo ");
        Serial.println( 5*60 - (millis() - lightStartTime)/1000 );
        shortBeep(50);
        state = INIT;
      }
      else if((millis() - lightStartTime) > triggerRelayTimeMs)//dalej zapalone przez 5s
      {
        Serial.println("Wlaczam wiatraczek");
        shortBeep(100);
        delay(50);
        shortBeep(100);
        relayOn();
        state = LIGHT_DETECTED_RELAY_ON;
      }
      break;
   case LIGHT_DETECTED_RELAY_ON:
      if(isDarkDetected())//zgaszone
      {
        Serial.println("Ciemnosc widze");
        shortBeep(50);
        lightEndTime = millis();
        int lightTimeSec = (lightEndTime - lightStartTime)/1000;
        Serial.print("Czas swiatla: ");
        Serial.print(lightTimeSec); 
        Serial.println("s");
        state = RELAY_ON;
      }
    break;
    case RELAY_ON:
      if(isLightDetected())//zapalam
      {
        Serial.println("Swiatlosc nastapila kiedy stan = RELAY_ON");
        shortBeep(50);
        state = LIGHT_DETECTED_RELAY_ON;
      }
      else//caly czas zgaszone
      {
        int lightTimeSec = (lightEndTime - lightStartTime)/1000;//liczba s kiedy bylo zapalone
        int elapsedTimeSec = (millis() - lightEndTime)/1000;
        int limitSec = lightTimeSec/2;
        
        if(limitSec> maxTimeToEnableTriggerMinutes )
          limitSec = maxTimeToEnableTriggerMinutes;

        if(elapsedTimeSec > limitSec)//czy uplynelo wicej czasu niz polowa zapalone swiatla (max do 20 min)
        {
          Serial.println("Koniec!");
          shortBeep(100);
          delay(50);
          shortBeep(100);
          state = INIT;
        }
      }      
    break;
  }
}
