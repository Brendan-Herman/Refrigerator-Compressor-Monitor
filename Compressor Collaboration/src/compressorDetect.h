// compressor_detect.h
#ifndef COMPRESSOR_DETECT_H
#define COMPRESSOR_DETECT_H

#include <Arduino.h>
#include <SD.h>
#include <vector>

bool compressorRunning(bool currentState); // call regularly from loop()
std::vector<float> simpleTempDetect();

#endif