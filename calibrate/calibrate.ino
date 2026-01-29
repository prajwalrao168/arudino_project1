#include "HX711.h"

// --- PINS ---
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

HX711 scale;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial to connect

  Serial.println("\n\n--- AUTO CALIBRATION WIZARD ---");
  Serial.println("Initializing scale...");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(); // Start with no scaling
  
  // --- STEP 1: TARE ---
  Serial.println("\n[STEP 1] EMPTY THE SCALE");
  Serial.println("Remove all items from the scale.");
  Serial.println("Type '1' and press ENTER when ready...");
  
  while(Serial.available() == 0) { } // Wait for input
  Serial.readString(); // Clear buffer
  
  scale.tare(); // Reset to 0
  Serial.println("Scale Zeroed!");

  // --- STEP 2: PLACE WEIGHT ---
  Serial.println("\n[STEP 2] PLACE KNOWN WEIGHT");
  Serial.println("Place an object of KNOWN weight on the scale.");
  Serial.println("(e.g., A phone, a full soda can ~350g, a bag of sugar)");
  Serial.println("Type '2' and press ENTER when the object is stable...");

  while(Serial.available() == 0) { } // Wait for input
  Serial.readString(); // Clear buffer

  long reading = scale.get_units(10); // Get raw average of 10 readings
  Serial.print("Raw reading captured: ");
  Serial.println(reading);

  // --- STEP 3: INPUT WEIGHT ---
  Serial.println("\n[STEP 3] ENTER REAL WEIGHT");
  Serial.println("Enter the EXACT weight (in grams) of the object you placed.");
  Serial.println("Example: If it is 250 grams, type '250'");
  
  while(Serial.available() == 0) { } // Wait for input
  float realWeight = Serial.parseFloat(); // Read the number user typed

  if (realWeight != 0) {
    float newFactor = reading / realWeight; // THE MAGIC MATH
    
    Serial.println("\n--------------------------------------------");
    Serial.println("       CALIBRATION COMPLETE!");
    Serial.println("--------------------------------------------");
    Serial.print("Your Calibration Factor is: ");
    Serial.println(newFactor);
    Serial.println("--------------------------------------------");
    Serial.println("ACTION: Go to your main code and update this line:");
    Serial.print("float calibration_factor = "); 
    Serial.print(newFactor); 
    Serial.println(";");
    Serial.println("--------------------------------------------");
  } else {
    Serial.println("Error: You entered zero. Please reset and try again.");
  }
}

void loop() {
  // Do nothing
}