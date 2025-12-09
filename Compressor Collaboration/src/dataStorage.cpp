#include <SPI.h>
#include <vector>
#include <SD.h>
#include "dataStorage.h"
#include <cassert>

// count number of files in the folders
int countFiles(Mode mode)
{
  File myFile;
  File dir = SD.open((mode == TEMPERATURE) ? "/temperature" : "/vibration");
  int count = 0;
  myFile = dir.openNextFile();

  while (myFile)
  {
    String filename = String(myFile.name());
    // Only count files that start with "data"
    if (filename.startsWith("data"))
    {
      count++;
    }
    myFile.close();
    myFile = dir.openNextFile();
  }
  myFile.close();
  dir.close();
  return count;
}

// Vibration is vector of floats - frequencies of the fourier
void writeData(Mode mode, std::vector<float> vector, int cycle_num)
{
  File myFile;
  // open file to write to
  if (mode == TEMPERATURE)
  {
    myFile = SD.open("/temperature/data" + String(cycle_num) + ".csv", FILE_WRITE);
    if (myFile)
    {
      // parse through passed temp vector and write to the file
      for (int i = 0; i < vector.size(); i++)
      {
        myFile.println(vector[i]);
      }
    }
    myFile.close();
  }
  if (mode == VIBRATION)
  {
    myFile = SD.open("/vibration/data" + String(cycle_num) + ".csv", FILE_WRITE);
    if (myFile)
    {
      // parse through passed temp vector and write to file
      for (int i = 0; i < vector.size(); i++)
      {
        myFile.println(vector[i]);
      }
    }
    myFile.close();
  }
}

// write baseline storing functions for both
// take vector store in file
void saveBaseline(Mode mode, std::vector<float> vector)
{
  File myFile;
  // save a baseline vector according to mode (VIBRATION or TEMPERATURE)
  if (mode == TEMPERATURE)
  {
    myFile = SD.open("/temperature/tempBaseline.csv", FILE_WRITE);
    for (int i = 0; i < vector.size(); i++)
    {
      myFile.println(vector[i]);
    }
    myFile.close();
  }
  else if (mode == VIBRATION)
  {
    myFile = SD.open("/vibration/vibrationBaseline.csv", FILE_WRITE);
    for (int i = 0; i < vector.size(); i++)
    {
      myFile.println(vector[i]);
    }
    myFile.close();
  }
}

// baseline retrieval
//  - return baseline vector
std::vector<float> readBaseline(Mode mode)
{
  File myFile;
  std::vector<float> vector;
  if (mode == TEMPERATURE)
  {
    if (SD.exists("/temperature/tempBaseline.csv"))
    {
      myFile = SD.open("/temperature/tempBaseline.csv");
      if (!myFile)
      {
        Serial.println("Failed to open file");
        return {};
      }
      if (myFile.size() == 0)
      {
        return {};
      }
      else
      {
        while (myFile.available())
        {
          String line = myFile.readStringUntil('\n');
          line.trim(); // Remove whitespace

          if (line.length() > 0)
          {
            float value = line.toFloat();
            vector.push_back(value);
          }
        }
        myFile.close();
        return vector;
      }
    }
    else
    {
      return {};
    }
  }
  else if (mode == VIBRATION)
  {
    myFile = SD.open("/vibration/vibrationBaseline.csv");
    if (!myFile)
    {
      Serial.println("Failed to open file");
      return {};
    }
    if (myFile.size() == 0)
    {
      return {};
    }
    else
    {
      while (myFile.available())
      {
        String line = myFile.readStringUntil('\n');
        line.trim(); // Remove whitespace

        if (line.length() > 0)
        {
          float value = line.toFloat();
          vector.push_back(value);
        }
      }
      myFile.close();
      return vector;
    }
  }
  return {};
}

// return vectors from first 100 vibration files one at a time line159
//  - storing standard deviation and retrieving it 2048
std::vector<float> getVibrationBaseline()
{
  File myFile;
  bool possible = true;
  for (int i = 0; i < 100; i++)
  {
    if (!SD.exists("/vibration/data" + String(i) + ".csv"))
    {
      possible = false;
      Serial.println("Error in vibration baseline: file " + String(i) + " does not exist");
      break;
    }
  }
  if (possible)
  {
    // Serial.println("Attempting allocation of baseline vector");
    std::vector<float> vector;
    for (int i = 0; i < 2048; i++)
    {
      vector.push_back(0);
    }
    // Serial.println("Baseline vector allocated");
    // int j = 0;
    for (int i = 0; i < 100; i++)
    {
      // Serial.println("Attempting to read file "+String(i));
      myFile = SD.open("/vibration/data" + String(i) + ".csv", FILE_READ);

      while (myFile.available() && vector.size() < 120)
      {
          String line =myFile.readStringUntil('\n');
          line.trim();
          if (line.length() > 0)
          {
            vector.push_back(line.toFloat());
          }else{
            vector.push_back(0.0f);
          }
      }
    }
    // Serial.println("Read all files");
    for (int i = 0; i < vector.size(); i++)
    {
      vector[i] /= 100;
    }
    // Serial.println("Computed Baseline");
    return vector;
  }
  else
  {
    return {{-1}};
  }
}

// std Dev file for single standard deviation
//  be able to retrieve - return std dev or -1 if

// read a vibration data file
std::vector<float> readVibrationData(int i)
{
  File myFile;
  // retrieve a vibration vector
  if (!SD.exists("/vibration/data" + String(i) + ".csv"))
  {
    Serial.println("Vibration File Not Found");
    return {0.0f};
  }
  else
  {
    // initialize an 2048-vector
    std::vector<float> vector;
    vector.reserve(2048);
    myFile = SD.open("/vibration/data" + String(i) + ".csv");
    while (myFile.available() && vector.size() < 2048)
    {
        String line =myFile.readStringUntil('\n');
        line.trim();
        if (line.length() > 0)
        {
          vector.push_back(line.toFloat());
        }else{
          vector.push_back(0.0f);
        }
    }
    myFile.close();
    // Serial.println("Printing");
    // for(int i=0;i<vector.size();i++){
    //   Serial.println(vector[i]);
    // }
    // Serial.println("Done printing");
    //assert(vector.size()==2048);
    // return the vector
    return vector;
  }
}



//write to serial and save to file ast
// #include "logger.h"



std::vector<String> logBuffer;
const int MAX_LOG_BUFFER = 50;

void flushLogs() {
  if (logBuffer.empty()) return;
  
  File logFile = SD.open("/log.txt", FILE_APPEND);
  if (logFile) {
    for (const String& line : logBuffer) {
      logFile.print(line);
    }
    logFile.close();
    logBuffer.clear();
    Serial.println("log saved");
  }
}

void logPrintln(const String &msg) {
  Serial.println(msg);
  //logBuffer.push_back(msg + "\n");
  
  // Only write when buffer is full
  if (logBuffer.size() >= MAX_LOG_BUFFER) {
    flushLogs();
  }
}

void logPrint(const String &msg) {
  Serial.print(msg);
  //logBuffer.push_back(msg);
  
  // Only write when buffer is full
  if (logBuffer.size() >= MAX_LOG_BUFFER) {
    flushLogs();
  }
}

