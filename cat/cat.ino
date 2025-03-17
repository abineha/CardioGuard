#include <Arduino.h>
#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "model.h"

// TensorFlow Lite Globals
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
tflite::AllOpsResolver resolver;
constexpr int kTensorArenaSize = 50 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}

const int ecgPin = A0;
const int sampleSize = 300;
float ecgData[sampleSize];

void preprocessECG(float* data) {
    // Normalize ECG data (min-max scaling)
    float minVal = data[0], maxVal = data[0];
    for (int i = 1; i < sampleSize; i++) {
        if (data[i] < minVal) minVal = data[i];
        if (data[i] > maxVal) maxVal = data[i];
    }
    
    for (int i = 0; i < sampleSize; i++) {
        data[i] = (data[i] - minVal) / (maxVal - minVal);
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting real-time ECG analysis...");

    // Initialize TensorFlow Lite
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;
    model = tflite::GetModel(model_tflite);
    if (!model) {
        Serial.println("ERROR: Model failed to load!");
        return;
    }

    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("ERROR: Tensor allocation failed!");
        return;
    }
    Serial.println("Model ready!");
}

void loop() {
    Serial.println("Reading ECG data...");
    for (int i = 0; i < sampleSize; i++) {
        ecgData[i] = analogRead(ecgPin) / 1023.0;  // Read & normalize (0-1)
        delay(4);  // 250Hz sampling rate
    }
    
    preprocessECG(ecgData);
    
    // Set input tensor
    float* input_tensor = interpreter->input(0)->data.f;
    for (int i = 0; i < sampleSize; i++) {
        input_tensor[i] = ecgData[i];
    }

    // Run inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("ERROR: Inference failed!");
        return;
    }

    // Read output
    float* output = interpreter->output(0)->data.f;
    int predicted_class = -1;
    float max_confidence = -1.0f;
    
    Serial.println("Arrhythmia classification:");
    for (int i = 0; i < 5; i++) {
        Serial.print("Class "); Serial.print(i);
        Serial.print(": "); Serial.println(output[i], 4);
        
        if (output[i] > max_confidence) {
            max_confidence = output[i];
            predicted_class = i;
        }
    }

    Serial.print("Predicted arrhythmia class: ");
    Serial.println(predicted_class);
    
    delay(1000);  // Wait before the next reading
}
