#include "HX711.h"

// --- PINS ---
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;
const int BUZZER_PIN = 12;
const int LED_PIN = 6;

float calibration_factor = -212.55; 

// --- TIMERS ---
// const long DRINK_TIMEOUT = 2700000; // 45 Mins 
// const long WALK_TIMEOUT = 1800000;  // 30 Mins 

// TESTING TIMERS :
const long DRINK_TIMEOUT = 15000;    
const long WALK_TIMEOUT = 60000;     

// --- THRESHOLDS ---
const int SLOUCH_BUFFER_CM = 5;      
const int WALK_THRESHOLD_CM = 30;    
const int SIP_THRESHOLD = 5;        
const int MIN_BOTTLE_WEIGHT = 5;    

const unsigned long SLOUCH_TOLERANCE_MS = 3000; 
const unsigned long EMPTY_BOTTLE_TIMEOUT = 10000; 

HX711 scale;
int baselineDist = 0;           
int lastKnownPos = 0;           
float lastStableWeight = 0;     
bool bottleRemoved = false;     

unsigned long lastDrinkTime = 0;
unsigned long lastWalkTime = 0;
unsigned long lastAlarmTime = 0; 

unsigned long slouchTimerStart = 0;
bool isSlouchingState = false;

unsigned long emptyBottleStart = 0;
bool isEmptyState = false;
unsigned long lastRefillBeep = 0;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("\n--- SYSTEM RESTARTING ---");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare(); 
  Serial.println("Scale Zeroed.");

  Serial.println("Place bottle on coaster...");
  while(scale.get_units(5) < MIN_BOTTLE_WEIGHT) {
      blinkLED(1, 100); 
  }
  delay(1000); 
  lastStableWeight = scale.get_units(10);
  Serial.print("Initial Bottle Weight: "); Serial.print(lastStableWeight); Serial.println(" g");

  Serial.println("Sit up straight! Calibrating...");
  blinkLED(3, 200);
  digitalWrite(LED_PIN, HIGH); 
  
  long totalDist = 0;
  int readings = 0;
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) { 
    int d = getDistance();
    if (d > 0 && d < 200) { totalDist += d; readings++; }
    delay(100);
  }
  digitalWrite(LED_PIN, LOW); 
  
  if (readings > 0) baselineDist = totalDist / readings;
  else baselineDist = 50; 
  
  lastKnownPos = baselineDist;
  
  lastDrinkTime = millis();
  lastWalkTime = millis();
  
  tone(BUZZER_PIN, 1000, 500); 
  Serial.println("--- MONITORING ACTIVE ---");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // --- READ SENSORS ---
  int currentDist = getDistance();
  float currentWeight = scale.get_units(5); 
  if (currentWeight < 0) currentWeight = 0; 

  // --- DEBUG PRINT ---
  long drinkElapsed = currentMillis - lastDrinkTime;
  long drinkRemaining = (DRINK_TIMEOUT - drinkElapsed) / 1000;
  if (drinkRemaining < 0) drinkRemaining = 0; 

  Serial.print("Dist: "); Serial.print(currentDist);
  Serial.print("cm | Wt: "); Serial.print(currentWeight, 0);
  Serial.print("g | DrinkTmr: "); Serial.print(drinkRemaining);
  Serial.print("s | WalkTmr: "); Serial.println((WALK_TIMEOUT - (currentMillis - lastWalkTime))/1000);

  // 1. HYDRATION MONITOR (Non-Blocking)
  
  // A. Check if Lifted
  if (currentWeight < MIN_BOTTLE_WEIGHT && !bottleRemoved) {
    bottleRemoved = true; 
  }
  // B. Check if Replaced
  else if (currentWeight > MIN_BOTTLE_WEIGHT && bottleRemoved) {
    delay(1000); // Wait for settle
    float newStableWeight = scale.get_units(10);
    float weightDiff = lastStableWeight - newStableWeight;

    if (weightDiff >= SIP_THRESHOLD) {
      Serial.println("--> SIP CONFIRMED! Resetting Drink Timer.");
      lastDrinkTime = currentMillis; 
      tone(BUZZER_PIN, 2000, 150); tone(BUZZER_PIN, 2500, 150);
    } 
    else {
      Serial.println("--> Bottle Replaced (No Sip / Refill). Timer Continues.");
    }
    lastStableWeight = newStableWeight;
    bottleRemoved = false; 
  }

  // C. Drink Timer Alarm 
  if (currentMillis - lastDrinkTime > DRINK_TIMEOUT) {
    if (currentMillis - lastAlarmTime > 5000) {
       Serial.println("!-- DRINK REMINDER --!");
       tone(BUZZER_PIN, 1500, 300);    
       digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
       lastAlarmTime = currentMillis; 
    }
  }

  // If weight is near zero (Empty or Removed)
  if (currentWeight < MIN_BOTTLE_WEIGHT) {
      if (!isEmptyState) {
          // Start the timer
          isEmptyState = true;
          emptyBottleStart = currentMillis;
      } 
      else if (currentMillis - emptyBottleStart > EMPTY_BOTTLE_TIMEOUT) {
          // 10 seconds passed with weight at zero
          if (currentMillis - lastRefillBeep > 5000) { // Don't beep constantly, every 5 sec
             Serial.println("!-- REFILL BOTTLE --!");
             // Distinct triple beep
             tone(BUZZER_PIN, 800, 150); delay(200);
             tone(BUZZER_PIN, 800, 150); delay(200);
             tone(BUZZER_PIN, 800, 150);
             lastRefillBeep = currentMillis;
          }
      }
  } else {
      // Weight returned, reset state
      isEmptyState = false;
  }

  // 2. SLOUCH & WALK MONITOR
  // Check if current distance indicates slouching
  bool currentSlouchCheck = (currentDist < 150 && currentDist < (baselineDist - SLOUCH_BUFFER_CM));

  if (currentSlouchCheck) {
      if (!isSlouchingState) {
          // Just started slouching, start timer
          isSlouchingState = true;
          slouchTimerStart = currentMillis;
      } 
      else if (currentMillis - slouchTimerStart > SLOUCH_TOLERANCE_MS) {
          // Has been slouching for > 3 seconds
          Serial.println("--> SLOUCH DETECTED (Confirmed)!");
          tone(BUZZER_PIN, 400, 200); 
      }
  } else {
      // User sat up, reset timer
      isSlouchingState = false;
  }

  // Check for Walking (Movement)
  if (abs(currentDist - lastKnownPos) > WALK_THRESHOLD_CM) {
      lastWalkTime = currentMillis; 
      lastKnownPos = currentDist;   
      Serial.println("--> Walk Detected. Timer Reset.");
  }

  // The Jail Loop (Walk Alarm)
  if (currentMillis - lastWalkTime > WALK_TIMEOUT) {
    Serial.println("OPENDIALOG"); 
    Serial.println("--> WALK ALARM ACTIVE.");
    
    bool userHasWalked = false;
    while (!userHasWalked) {
      tone(BUZZER_PIN, 1000); digitalWrite(LED_PIN, HIGH);
      delay(300);
      noTone(BUZZER_PIN); digitalWrite(LED_PIN, LOW);
      delay(300);

      int checkDist = getDistance();
      if (abs(checkDist - lastKnownPos) > WALK_THRESHOLD_CM || checkDist > 150) {
        userHasWalked = true;
        Serial.println("--> ALARM CLEARED.");
        lastWalkTime = millis();
        lastKnownPos = checkDist; 
        tone(BUZZER_PIN, 2000, 200);
      }
    }
  }

  delay(100); 
}

// --- HELPERS ---
int getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
  int d = duration * 0.034 / 2;
  if (d == 0) return 300; 
  return d;
}

void blinkLED(int times, int speed) {
  for(int i=0; i<times; i++) {
    digitalWrite(LED_PIN, HIGH); delay(speed);
    digitalWrite(LED_PIN, LOW); delay(speed);
  }
}