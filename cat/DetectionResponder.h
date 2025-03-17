#pragma once

namespace DetectionResponder {
  void setup();
  void respondToDetection(int predicted_class, float confidence);

}