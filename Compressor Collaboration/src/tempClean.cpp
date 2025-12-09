#include "tempClean.h"

float tempClean(std::vector<float>& temperatures) {
  
  // Calculate mean of first 5 temps
  float lowMean;
  float sum = 0.0;
  for (int i = 0; i < 5; i++) {
    sum += temperatures[i];
  }
  lowMean = (sum / 5);
  
  // Calculate mean of last 5 temps 
  float highMean;
  sum = 0.0;
  for (int i = temperatures.size() - 5; i < temperatures.size(); i++) {
      sum += temperatures[i];
  }
  highMean = (sum / 5);
  
  //Calculate slope
  float slope = (highMean - lowMean) / (temperatures.size() - 5);

  //Return the slope
  return slope;

}