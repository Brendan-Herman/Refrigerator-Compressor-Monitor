#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <OneWire.h>
#include "esp_system.h"

// include header files
#include "getTemp.h"
#include "tempClean.h"
#include "tempAnalysis.h"
#include "dataStorage.h"
#include "communication.h"
#include "vibration.h"
#include "compressorDetect.h"
#include <SafeQueue.h>
#include <mutex>

// sd card pins
#define CS_PIN 5
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
// temperature pin
#define TEMP_PIN 4
// piezo sensor pin
#define PIEZO_PIN 32

#define LED 2

// Global
int state = 1; // TODO change back
int cycleNum = 0;

// Communication Task Controls
enum communicationControl
{
  UPDATEMESSAGE,
  CHECKTELEGRAM,
  UPDATEDATAMESSAGES,
  STATUSCHECK
};
SafeQueue<communicationControl> com_control_queue;
SafeQueue<float> vibrationQueue;
SafeQueue<float> tempQueue;
SafeQueue<String> messageQueue;
TaskHandle_t comm_handle;
mutex control_lock;

bool vibrationBaselineExists = false;
bool temperatureBaselineExists = false;

// For temperatures
OneWire ds(TEMP_PIN);
int temperature_sensor_timer = 0;
int total_time = 0;
std::vector<float> baselineSlopes;
std::vector<float> temperatures;
bool tempCleaned = 0;
float tempZScore = 0;
std::vector<float> temperatureChecking;

// For Storage
// File myFile;

// For vibration
CArray data;
vector<float> vibrationBaseline;
float vibrationSTD = -1.0f;
int TotalVibrationCycles = 0; // number of vibration cycles total
float vibRead;
float cycleSTD = 0.0f;
int numVibrationCyclesInSTDCalculation = 0; // number of vibration cycles in current standard deviation calculation

void communicationTask(void *args)
{
  float temperature = 0;
  float vibration = 0;
  String tempStr;
  String vibStr;
  control_lock.lock(); // the thread must own the lock before unlocking it, otherwise the entire program could be corrupted due to UB
  while (true)
  {
    control_lock.unlock(); // release the lock upon completion
    communicationControl c = com_control_queue.dequeue();
    control_lock.lock(); // aquire the lock immediately for the task
    switch (c)
    { // execute the correct task
    case UPDATEMESSAGE:
      // update the status message
      updateMessage(msgStatusId, messageQueue.dequeue());
      break;
    case CHECKTELEGRAM:
      // check telegram for something
      //  checkTelegram();
      break;
    case UPDATEDATAMESSAGES:
      // update the data messages
      tempStr = messageQueue.dequeue();
      vibStr = messageQueue.dequeue();
      updateDataMessages(tempStr, lastTempStr, vibStr, lastVibStr);
      break;
    case STATUSCHECK:
      temperature = tempQueue.dequeue();
      vibration = vibrationQueue.dequeue();
      statusCheck(temperature, vibration, lastStatus);
      break;
    }
  }
}

void setup()
{
  // Serial for debugging
  delay(500);
  Serial.begin(115200);
  while (!Serial)
  {
  }

  // attempt SPI
  Serial.println("Initializing SPI...");
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  // attempt to mount SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(CS_PIN))
  {
    Serial.println("Card mount failed");
  }

  logPrintln("free heap:" + String(esp_get_free_heap_size()));

  logPrintln("\n\n=== ESP32 RESTARTED ===");
  logPrintln("Reason: " + String(esp_reset_reason()));
  logPrintln("free heap:" + String(esp_get_free_heap_size()));

  // Wifi Setup
  WiFi.begin(ssid, password);

  logPrint("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    logPrint(".");
  }
  logPrintln("\nConnected!");

  preferencesStartup(false); // true - new , false - not new

  logPrintln("status: " + String(msgStatusId) + " temp: " + String(msgTempId) + " vib: " + String(msgVibId) + " alert: " + String(msgAlertId));
  // end of WiFi setup

  // initialize pins
  pinMode(TEMP_PIN, INPUT);
  pinMode(PIEZO_PIN, INPUT);
  pinMode(LED, OUTPUT); // for debuging

  logPrintln("\n=== SD Card Diagnostics ===");
  String words = "Free heap: %d bytes\n" + String(ESP.getFreeHeap());
  logPrint(words);
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  words = "SD Card Size: %llu MB\n" + String(cardSize);
  logPrint(words);
  logPrintln("Card Mount Successful");
  logPrintln("free heap:" + String(esp_get_free_heap_size()));
  // check if baselines exist
  File myFile;
  if (!SD.exists("/temperature/tempBaseline.csv"))
  {
    myFile = SD.open("/temperature/tempBaseline.csv", FILE_WRITE);
    myFile.close();
  }
  if (!SD.exists("/vibration/vibrationBaseline.csv"))
  {
    myFile = SD.open("/vibration/vibrationBaseline.csv", FILE_WRITE);
    myFile.close();
  }
  // load standard deviations if baselines exist (i.e. after power failure, recompute standard deviations). If the standard deviation file doesn't exist, return -1
  // if (!SD.exists("/vbration/stdDev.csv")) {
  //   myFile = SD.open("/vibration/stdDev.csv", FILE_WRITE);
  //   myFile.close();
  // }
  xTaskCreatePinnedToCore(communicationTask, "COMMS", 8192, NULL, 0, &comm_handle, 0);
  // count the number of data files to find how many cycles have occurred
  TotalVibrationCycles = countFiles(VIBRATION);
  cycleNum = countFiles(TEMPERATURE);
  vibrationBaseline = readBaseline(VIBRATION);
  if (vibrationBaseline.size() > 0)
  {
    vibrationBaselineExists = true;
  }
}

void loop()
{
  // ------------------------STATE 1------------------------ //

  if (state == 1) // rest state, waiting for compressor to turn on
  {
    // TODO remove debug
    // vib = analogRead(PIEZO_PIN);
    // logPrint("Vibration:     ");
    // logPrint(vib);
    // logPrint("      \r");
    temperatureChecking = simpleTempDetect();
    if (temperatureChecking.size() != 1) // TODO see if this temp function works and maybe change back compressorRunning(false)
    // keep the line below commented or else who knows what could happen
    //  updateMessage(msgStatusId, "Fridge Compressor 1 Status: " + lastStatus + " (collecting)");
    {

      control_lock.lock();
      logPrintln(lastStatus);
      com_control_queue.enqueue(UPDATEMESSAGE);
      messageQueue.enqueue("Fridge Compressor 1  Status: " + lastStatus + " (collecting).");
      logPrintln(String(msgStatusId) + " " + lastStatus);
      control_lock.unlock();
      state = 2;
      digitalWrite(LED, HIGH);
      logPrintln("Changed to State 2");
      logPrint("Temp vector returned: ");
      for (int i = 0; i < temperatureChecking.size(); i++)
      {
        logPrint(String(temperatureChecking[i]) + ", ");
      }
    }
    temperatureChecking.clear();
    temperatureChecking.shrink_to_fit();
  }

  // ------------------------STATE 2------------------------ //

  else if (state == 2) // data collecting state, collecing data unticmpressor turns off
  {
    // record vibration every 5ms
    //  temperature_sensor_timer=1000000;
    if (temperature_sensor_timer >= 5000)
    { // record temperature every 5 seconds
      float temp = getTemp();
      while (temp < 30 || temp > 175)
      {
        temp = getTemp(); // might cause issues
      }
      temperatures.push_back(temp);
      temperature_sensor_timer = 0;
    }
    if (store_vibration())
    {
      // store the vibrations to the data vector and compute the FFT when ready
      fft(data);
      vector<float> transform = magnitude(data);
      erase();
      logPrintln("Transform calculated");

      // find max for the transform
      float Max = 0.0;
      int j = 0;
      for (int i = 20; i < transform.size() / 2; i++)
      {
        if (transform[i] > Max)
        {
          j = i;
          Max = transform[i];
        }
      }
      String tempStr = String(getTemp(), 1) + "°F";           // one decimal
      String vibStr = String((((float)j) / 2048 * 200) + 56) + "Hz"; // three decimals  j*1/2048*200
      // updateDataMessages(tempStr, lastTempStr, vibStr, lastVibStr);
      control_lock.lock();
      messageQueue.enqueue(tempStr);
      messageQueue.enqueue(vibStr);
      com_control_queue.enqueue(UPDATEDATAMESSAGES);
      control_lock.unlock();
      logPrintln("Updating vibration messages");

      // save the data
      writeData(VIBRATION, transform, TotalVibrationCycles);
      logPrintln("Transform saved as file" + String(TotalVibrationCycles));
      // The data vector is ready for another cycle, and the transform vector is ready for analysis
      if (vibrationBaselineExists)
      {
        // compare current vibration to baseline
        if (vibrationSTD < 0)
        {
          // compare old files and compute vibration standard deviation. This can be recomputed every time the system restarts
          vector<float> old;
          old.reserve(2048);
          logPrintln("Attempting VSTD calc");
          for (int i = 0; i < 100; i++)
          {
            old.clear();
            // logPrintln("Attempting to read file "+String(i));
            old = readVibrationData(i);
            if (old.size() != 2048)
            {
              logPrintln("Vibration vector was not of the correct size");
              continue;
            }
            vibrationSTD += compare(vibrationBaseline, old);
            // logPrintln(String(vibrationSTD));
          }
          vibrationSTD = vibrationSTD > 0 ? pow(vibrationSTD / 100, 0.5f) : -1;
          logPrintln("Vibration Standard Deviation Calculated: " + String(vibrationSTD));
        }
        cycleSTD += vectorsize(vectordifference(vibrationBaseline, transform)) / vibrationSTD;
        numVibrationCyclesInSTDCalculation++;
        logPrintln("Vibration std dev of current recording amassed in running total");
      }
      else
      {
        // construct baseline and save
        if (TotalVibrationCycles >= 100)
        {
          // logPrintln("Attemptint to calculate the vibration baseline");
          vibrationBaseline = getVibrationBaseline();
          if (vibrationBaseline.size() > 2)
          {
            // logPrintln("Attempting to save the vibration baseline");
            vibrationBaselineExists = true;
            saveBaseline(VIBRATION, vibrationBaseline);
          }
          logPrintln("Vibration baseline found and saved");
        }
      }
      TotalVibrationCycles++;
    }
    temperature_sensor_timer += 5;
    // send to state 3 when enough temperature data is collected
    // logPrintln(String(temperatures.size()));
    // logPrintln(temperature_sensor_timer);
    if (temperatures.size() >= 120)
    {
      state = 3;
      logPrintln("switching to state 3");
      erase();
    }
    delay(5); // five millisecond delay
  }

  // ------------------------STATE 3------------------------ //

  else if (state == 3)
  {
    // read temp baseline
    baselineSlopes = readBaseline(TEMPERATURE);
    // this code might be causing issues with race conditions in HTTP requests
    //  control_lock.lock();
    //  com_control_queue.enqueue(UPDATEMESSAGE);
    //  messageQueue.enqueue("Fridge Compressor 1  Status: " + lastStatus + " (analyzing).");
    //  logPrintln(String(msgStatusId) + " " + lastStatus);
    //  control_lock.unlock();
    //  Analyze temp data
    if (baselineSlopes.size() >= 100) // TODO change back 100
    {
      tempZScore = tempAnalysis(baselineSlopes, temperatures);
    }
    // Make and store temp baseline
    if (baselineSlopes.size() < 800)
    {
      logPrintln("tempClean for last cycle: " + String(tempClean(temperatures)));
      baselineSlopes.push_back(tempClean(temperatures));
      // print the baseline
      logPrintln("baseline vector: ");
      for (int i = 0; i < baselineSlopes.size(); i++)
      {
        logPrint(String(baselineSlopes[i]) + ", ");
      }
      // store the baseline
      saveBaseline(TEMPERATURE, baselineSlopes);
    }
    else if (baselineSlopes.size() == 800)
    {
      saveBaseline(TEMPERATURE, baselineSlopes);
    }

    // vibration analysis
    cycleSTD = cycleSTD / numVibrationCyclesInSTDCalculation;

    // Save the temp
    writeData(TEMPERATURE, temperatures, cycleNum);
    logPrintln("Temperature data saved as file " + String(cycleNum));
    cycleNum++;
    temperatures.clear();
    File temporaryFile = SD.open("/cyclenumbers.csv", FILE_WRITE);
    temporaryFile.println(cycleNum);
    temporaryFile.close();

    control_lock.lock();
    // statusCheck(tempZScore, vibrZScore, lastStatus);
    logPrintln("Vibration Z: " + String(cycleSTD) + " Temp Z: " + String(tempZScore));
    vibrationQueue.enqueue(cycleSTD);
    tempQueue.enqueue(tempZScore);
    com_control_queue.enqueue(STATUSCHECK);
    control_lock.unlock();

    delay(500); // This is the one line you can't remove. It eliminates a potential race condition between the main and comms processes.

    // return to rest state
    logPrintln("Switching to state 4");
    state = 4;
    // updateMessage(msgStatusId, "Fridge Compressor 1 Status: " + lastStatus + " (inactive)");
    control_lock.lock();
    com_control_queue.enqueue(UPDATEMESSAGE);
    messageQueue.enqueue("Fridge Compressor 1 Status: " + lastStatus + " (resting)");
    control_lock.unlock();

    // Reset vibration cycle statistics variables
    numVibrationCyclesInSTDCalculation = 0;
    cycleSTD = 0;

    // Mantainence calls
    // TotalVibrationCycles=countFiles(VIBRATION);
    erase();
    com_control_queue.enqueue(CHECKTELEGRAM);
    control_lock.lock();
    com_control_queue.enqueue(UPDATEMESSAGE);
    messageQueue.enqueue("Fridge Compressor 1 Status: " + lastStatus + " (resting)");
    control_lock.unlock();
  }

  // ------------------------STATE 4------------------------ //
  else if (state == 4)
  {
    temperatureChecking = simpleTempDetect();
    if (temperatureChecking.size() == 1 && temperatureChecking[0] == -1)
    {
      state = 1;
      logPrintln("Switching to state 1");
      control_lock.lock();
      logPrintln(lastStatus);
      control_lock.unlock();
      digitalWrite(LED, LOW);
    }
  }
}