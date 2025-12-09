#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <vector>
#include <SD.h>
#include <cassert>

//extern File myFile; // use same myFile iteration

enum Mode {
    TEMPERATURE, 
    VIBRATION
};

int countFiles(Mode mode);
void writeData(Mode mode, std::vector<float> vector, int cycle_num);

void saveBaseline(Mode mode, std::vector<float> vector);
std::vector<float> readBaseline(Mode mode);
std::vector<float> getVibrationBaseline();
std::vector<float> readVibrationData(int i);

void logPrintln(const String &msg);
void logPrint(const String &msg);


#endif
