#include "DetectionResponder.h"
#include <Arduino.h>

namespace DetectionResponder {

const int LED_RED = 3;   // Red LED
const int LED_GREEN = 4; // Green LED
const int LED_BLUE = 5;  // Blue LED


void setup() {
  // Initialize the built-in LED pins as outputs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  // Start with LEDs off (LEDs are active LOW)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
}

void respondToDetection(int predicted_class, float confidence) {
  // Turn all LEDs OFF (LEDs are active LOW)
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  // Respond based on the predicted class
  switch (predicted_class) {
    case 0: // Class 0: No arrhythmia detected
      digitalWrite(LED_GREEN, LOW); // Green LED ON
      break;

    case 1: // Class 1: Type 1 arrhythmia
      digitalWrite(LED_RED, LOW); // Red LED ON
      break;

    case 2: // Class 2: Type 2 arrhythmia
      digitalWrite(LED_BLUE, LOW); // Blue LED ON
      break;

    case 3: // Class 3: Type 3 arrhythmia
      digitalWrite(LED_RED, LOW); // Red LED ON
      digitalWrite(LED_BLUE, LOW); // Red and Blue LEDs ON
      break;

    case 4: // Class 4: Type 4 arrhythmia
      digitalWrite(LED_RED, LOW); // Red LED ON
      digitalWrite(LED_GREEN, LOW); // Red and Green LEDs ON
      break;

    default: // Unknown class
      digitalWrite(LED_RED, LOW); // Flash Red LED for error
      delay(500);
      digitalWrite(LED_RED, HIGH);
      delay(500);
      break;
  }

  // Optional: Print confidence level for debugging
  Serial.print("Class: ");
  Serial.print(predicted_class);
  Serial.print(", Confidence: ");
  Serial.println(confidence);
}

} // namespace DetectionResponder