#include <Arduino.h>

// Define constants
const int ECG_PIN = A0;           // Analog pin connected to potentiometer
const int SAMPLE_RATE = 250;      // Hz
const int WINDOW_SIZE = 5 * SAMPLE_RATE; // 5 seconds of data
const unsigned long SAMPLING_PERIOD = 1000000 / SAMPLE_RATE; // in microseconds

// Selected athletic ECG thresholds (7 key features)
const float BRADYCARDIA_THRESHOLD = 60.0;     // bpm
const float QRS_DURATION_THRESHOLD = 110.0;   // ms (incomplete RBBB)
const float EARLY_REPOL_THRESHOLD = 0.1;      // mV
const float LVH_VOLTAGE_THRESHOLD = 3.5;      // mV
const float T_WAVE_INV_THRESHOLD = -0.1;      // mV
const float ST_DEPRESSION_THRESHOLD = -0.05;  // mV
const float PVC_THRESHOLD = 2;                // PVCs per 10s

// Circular buffer for ECG data
int ecgBuffer[WINDOW_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;

// Data for analysis
unsigned long lastPeakTime = 0;
int peakCount = 0;
int rPeaks[10];  // Store the last 10 R peaks for analysis
int rPeakIndex = 0;

// Results storage
bool hasBradycardia = false;
bool hasIncompleteRBBB = false;
bool hasEarlyRepol = false;
bool hasLVH = false;
bool hasTWaveInversion = false;
bool hasSTDepression = false;
bool hasPVCs = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  // Initialize buffer
  for (int i = 0; i < WINDOW_SIZE; i++) {
    ecgBuffer[i] = 0;
  }
  
  Serial.println("Athletic ECG Analysis Starting...");
  Serial.println("Collecting initial data...");
}

void loop() {
  static unsigned long lastSampleTime = 0;
  unsigned long currentTime = micros();
  
  // Sample at the specified rate
  if (currentTime - lastSampleTime >= SAMPLING_PERIOD) {
    lastSampleTime = currentTime;
    
    // Read ECG data (from potentiometer)
    int ecgValue = analogRead(ECG_PIN);
    
    // Store in circular buffer
    ecgBuffer[bufferIndex] = ecgValue;
    bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
    
    if (bufferIndex == 0) {
      bufferFilled = true;
    }
    
    // Detect R peaks (simplified)
    detectRPeaks(ecgValue, currentTime);
    
    // Analyze data if buffer is filled
    if (bufferFilled && bufferIndex % (SAMPLE_RATE / 2) == 0) {
      analyzeECG();
      printResults();
    }
  }
}

void detectRPeaks(int ecgValue, unsigned long currentTime) {
  static int lastValue = 0;
  static int maxValue = 0;
  static unsigned long maxValueTime = 0;
  static bool rising = false;
  const int THRESHOLD = 700;  // Adjust based on your potentiometer range
  
  // Simple peak detection
  if (ecgValue > lastValue) {
    rising = true;
    if (ecgValue > maxValue) {
      maxValue = ecgValue;
      maxValueTime = currentTime;
    }
  } else if (rising && ecgValue < lastValue && maxValue > THRESHOLD) {
    // Peak found
    unsigned long timeSinceLastPeak = maxValueTime - lastPeakTime;
    if (timeSinceLastPeak > 200000) {  // Minimum 200ms between peaks
      lastPeakTime = maxValueTime;
      rPeaks[rPeakIndex] = maxValue;
      rPeakIndex = (rPeakIndex + 1) % 10;
      peakCount++;
    }
    rising = false;
    maxValue = 0;
  }
  
  lastValue = ecgValue;
}

void analyzeECG() {
  // 1. Heart Rate Analysis (Bradycardia)
  float heartRate = calculateHeartRate();
  hasBradycardia = (heartRate < BRADYCARDIA_THRESHOLD && heartRate > 20);
  
  // 2. QRS Duration (Incomplete RBBB) - Simplified estimation
  float qrsDuration = estimateQRSDuration();
  hasIncompleteRBBB = (qrsDuration > QRS_DURATION_THRESHOLD && qrsDuration < 120);
  
  // 3. Early Repolarization
  hasEarlyRepol = detectEarlyRepolarization();
  
  // 4. LVH Voltage Criteria
  hasLVH = checkLVHVoltage();
  
  // 5. T-wave Inversion
  hasTWaveInversion = detectTWaveInversion();
  
  // 6. ST Depression
  hasSTDepression = detectSTDepression();
  
  // 7. PVCs
  hasPVCs = countPVCs() > PVC_THRESHOLD;
}

float calculateHeartRate() {
  if (peakCount < 2) return 0;
  
  // Calculate average RR interval
  unsigned long totalTime = lastPeakTime - 0; // Time since start
  float minutesFraction = totalTime / 60000000.0; // Convert microseconds to minutes
  float heartRate = peakCount / minutesFraction;
  
  return heartRate;
}

float estimateQRSDuration() {
  // Simplified QRS duration estimation
  // In a real implementation, you would analyze the ECG waveform
  // Here we estimate based on signal rise and fall around peaks
  
  if (!bufferFilled) return 0;
  
  int qrsStart = -1;
  int qrsEnd = -1;
  int peakIndex = 0;
  
  // Find the latest peak in our buffer
  for (int i = 0; i < WINDOW_SIZE; i++) {
    int idx = (bufferIndex - i + WINDOW_SIZE) % WINDOW_SIZE;
    if (ecgBuffer[idx] > 700) {  // Threshold for peak
      peakIndex = idx;
      break;
    }
  }
  
  // Look for QRS start (going backward from peak)
  for (int i = 1; i < 50; i++) {  // Look up to 50 samples back
    int idx = (peakIndex - i + WINDOW_SIZE) % WINDOW_SIZE;
    int prevIdx = (peakIndex - i + 1 + WINDOW_SIZE) % WINDOW_SIZE;
    
    if (ecgBuffer[idx] < ecgBuffer[prevIdx] * 0.5) {
      qrsStart = idx;
      break;
    }
  }
  
  // Look for QRS end (going forward from peak)
  for (int i = 1; i < 50; i++) {  // Look up to 50 samples forward
    int idx = (peakIndex + i) % WINDOW_SIZE;
    int prevIdx = (peakIndex + i - 1) % WINDOW_SIZE;
    
    if (ecgBuffer[idx] < ecgBuffer[prevIdx] * 0.5) {
      qrsEnd = idx;
      break;
    }
  }
  
  if (qrsStart != -1 && qrsEnd != -1) {
    int qrsSamples = (qrsEnd - qrsStart + WINDOW_SIZE) % WINDOW_SIZE;
    return (qrsSamples * 1000.0) / SAMPLE_RATE;  // Convert to ms
  }
  
  return 80.0;  // Default value
}

bool detectEarlyRepolarization() {
  // Simplified early repolarization detection
  if (!bufferFilled) return false;
  
  // Look for ST segment elevation after peaks
  for (int i = 0; i < WINDOW_SIZE; i++) {
    int stPoint = (bufferIndex - i + WINDOW_SIZE) % WINDOW_SIZE;
    int jPoint = (stPoint + 10) % WINDOW_SIZE;  // J-point approx 40ms after R peak
    
    if (ecgBuffer[i] > 700) {  // R peak threshold
      float stElevation = (ecgBuffer[jPoint] - 512) * (3.3 / 1024.0);  // Convert to mV
      if (stElevation > EARLY_REPOL_THRESHOLD) {
        return true;
      }
    }
  }
  
  return false;
}

bool checkLVHVoltage() {
  // Simplified LVH check based on R wave amplitude
  if (!bufferFilled) return false;
  
  int maxRWave = 0;
  
  for (int i = 0; i < 10; i++) {
    if (rPeaks[i] > maxRWave) {
      maxRWave = rPeaks[i];
    }
  }
  
  // Convert ADC value to mV
  float maxVoltage = (maxRWave - 512) * (3.3 / 1024.0);
  
  return maxVoltage > LVH_VOLTAGE_THRESHOLD;
}

bool detectTWaveInversion() {
  // Simplified T wave inversion detection
  if (!bufferFilled) return false;
  
  // Look for negative deflection after QRS complex
  for (int i = 0; i < WINDOW_SIZE; i++) {
    if (ecgBuffer[i] > 700) {  // R peak threshold
      // Look ~200-300ms after R peak for T wave
      int tWaveIndex = (i + (SAMPLE_RATE / 4)) % WINDOW_SIZE;  // ~250ms after R peak
      
      // Calculate T wave amplitude
      float tWaveAmplitude = (ecgBuffer[tWaveIndex] - 512) * (3.3 / 1024.0);  // Convert to mV
      
      if (tWaveAmplitude < T_WAVE_INV_THRESHOLD) {
        return true;
      }
    }
  }
  
  return false;
}

bool detectSTDepression() {
  // Simplified ST depression detection
  if (!bufferFilled) return false;
  
  // Look for ST segment depression after peaks
  for (int i = 0; i < WINDOW_SIZE; i++) {
    if (ecgBuffer[i] > 700) {  // R peak threshold
      // Check ~80-120ms after R peak for ST segment
      int stIndex = (i + (SAMPLE_RATE / 10)) % WINDOW_SIZE;  // ~100ms after R peak
      
      // Calculate ST segment level
      float stLevel = (ecgBuffer[stIndex] - 512) * (3.3 / 1024.0);  // Convert to mV
      
      if (stLevel < ST_DEPRESSION_THRESHOLD) {
        return true;
      }
    }
  }
  
  return false;
}

int countPVCs() {
  // Simplified PVC detection based on early beats
  if (peakCount < 3) return 0;
  
  int pvcCount = 0;
  unsigned long lastInterval = 0;
  unsigned long currentInterval = 0;
  
  // Count irregular beats
  for (int i = 0; i < min(peakCount, 10) - 1; i++) {
    if (i > 0) {
      lastInterval = currentInterval;
      currentInterval = rPeaks[(rPeakIndex - i + 10) % 10] - rPeaks[(rPeakIndex - i - 1 + 10) % 10];
      
      // If beat comes more than 20% earlier than previous interval
      if (currentInterval < lastInterval * 0.8) {
        pvcCount++;
      }
    }
  }
  
  return pvcCount;
}

void printResults() {
  Serial.println("\n--- Athletic ECG Analysis Results ---");
  Serial.print("Heart Rate: ");
  Serial.print(calculateHeartRate());
  Serial.println(" bpm");
  
  Serial.println("Athletic ECG Features Detected:");
  
  if (hasBradycardia) {
    Serial.println("+ Sinus Bradycardia");
  }
  
  if (hasIncompleteRBBB) {
    Serial.println("+ Incomplete RBBB");
  }
  
  if (hasEarlyRepol) {
    Serial.println("+ Early Repolarization");
  }
  
  if (hasLVH) {
    Serial.println("+ LVH by Voltage Criteria");
  }
  
  if (hasTWaveInversion) {
    Serial.println("+ T-wave Inversion");
  }
  
  if (hasSTDepression) {
    Serial.println("+ ST Depression");
  }
  
  if (hasPVCs) {
    Serial.println("+ Premature Ventricular Contractions");
  }
  
  if (!hasBradycardia && !hasIncompleteRBBB && !hasEarlyRepol && 
      !hasLVH && !hasTWaveInversion && !hasSTDepression && !hasPVCs) {
    Serial.println("No athletic ECG features detected");
  }
  
  Serial.println("-------------------------------");
}
