#include "tempAnalysis.h"
#include <cmath>

float tempAnalysis(std::vector<float>& slopes, std::vector<float>& newTemp) {
  
  // Calculate mean of first 5 temps in newTemp
  float newLowMean = 0.0;
  for (int i = 0; i < 5; i++) {
    newLowMean += newTemp[i];
  }
  newLowMean /= 5;
  
  // Calculate mean of last 5 temps in newTemp
  float newHighMean = 0.0;
  for (int i = newTemp.size() - 5; i < newTemp.size(); i++) {
    newHighMean += newTemp[i];
  }
  newHighMean /= 5;
  
  // Calculate slope for newTemp
  float newSlope = (newHighMean - newLowMean) / (newTemp.size() - 5);
  
  // Calculate mean of slopes
  float slopesMean = 0.0;
  for (float slope : slopes) {
    slopesMean += slope;
  }
  slopesMean /= slopes.size();
  
  // Calculate standard deviation of slopes
  float variance = 0.0;
  for (float slope : slopes) {
    float diff = slope - slopesMean;
    variance += diff * diff;
  }
  variance /= slopes.size();
  float slopesStdDev = std::sqrt(variance);
  
  // Calculate z-score
  float zScore = abs((newSlope - slopesMean) / slopesStdDev);
  
  return zScore;
}