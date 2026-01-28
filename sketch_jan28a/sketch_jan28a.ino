// --- ULTRASONIC SENSOR DEBUGGER ---
// Pins:
// VCC  -> 5V
// GND  -> GND
// TRIG -> Pin 9
// ECHO -> Pin 10

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor to open

  Serial.println("--- ULTRASONIC SENSOR TEST ---");
  
  pinMode(TRIG_PIN, OUTPUT); // Sends the sound wave
  pinMode(ECHO_PIN, INPUT);  // Listens for the echo
}

void loop() {
  // 1. Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // 2. Send a 10 microsecond pulse (The "Ping")
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 3. Read the echo
  // pulseIn waits for the pin to go HIGH and counts how long it stays there
  long duration = pulseIn(ECHO_PIN, HIGH);

  // 4. Calculate Distance
  // Speed of sound = 0.034 cm/us. We divide by 2 (go and return).
  int distance = duration * 0.034 / 2;

  // 5. Print Results
  Serial.print("Raw Duration: ");
  Serial.print(duration);
  Serial.print(" | Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(500); // Wait half a second before next read
}
