// compressor_detect.cpp
#include "compressorDetect.h"
#include "dataStorage.h"
#include "getTemp.h"

#define PIEZO_PIN 32

// Parameters you can tune
static const int WINDOW_MS = 500;           // window length used to compute RMS (ms)
static const int SAMPLE_INTERVAL_US = 1000; // sample spacing (us) -> 1 kHz sampling
static const float EMA_ALPHA = 0.01f;       // baseline EMA smoothing factor (0..1). small=slow adapt
static const float THRESH_ON_MULT = 2.0f;   // turn ON if RMS > baseline * THRESH_ON_MULT
static const float THRESH_OFF_MULT = 1.5f;  // turn OFF if RMS < baseline * THRESH_OFF_MULT
static const int REQ_ON_COUNTS = 3;         // consecutive windows needed to declare ON
static const int REQ_OFF_COUNTS = 3;        // consecutive windows needed to declare OFF
static const char *BASELINE_PATH = "/baseline.bin";

static float baselineRms = -1.0f; // if <0 -> not initialized
static int onCounter = 0;
static int offCounter = 0;
static unsigned long lastPersist = 0;
static const unsigned long PERSIST_INTERVAL_MS = 60 * 1000UL; // persist baseline every minute

// helper: compute RMS over short window
static float sampleWindowRms()
{
    const int samples = WINDOW_MS * 1000 / SAMPLE_INTERVAL_US;
    long sumSq = 0;
    long sum = 0;
    for (int i = 0; i < samples; i++)
    {
        unsigned long start = micros();
        int v = analogRead(PIEZO_PIN);
        sum += v;
        sumSq += (long)v * (long)v;

        // Wait for remainder of interval
        unsigned long elapsed = micros() - start;
        if (elapsed < SAMPLE_INTERVAL_US)
        {
            delayMicroseconds(SAMPLE_INTERVAL_US - elapsed);
        }
    }
    float mean = sum / (float)samples;
    // compute variance via E[x^2] - mean^2 but implement safely:
    float ex2 = sumSq / (float)samples;
    float var = ex2 - mean * mean;
    if (var < 0)
        var = 0; // numerical safety
    float rms = sqrt(var);
    return rms;
}

// persistence
static void loadBaselineFromSd()
{
    if (!SD.exists(BASELINE_PATH))
        return;
    File f = SD.open(BASELINE_PATH, FILE_READ);
    if (!f)
        return;
    if (f.size() >= sizeof(float))
    {
        f.read((uint8_t *)&baselineRms, sizeof(float));
    }
    f.close();
}

static void persistBaselineToSd()
{
    SD.remove(BASELINE_PATH); // Delete first
    File f = SD.open(BASELINE_PATH, FILE_WRITE);
    if (!f)
        return;
    f.write((uint8_t *)&baselineRms, sizeof(float));
    f.close();
}

// The exported function
bool compressorRunning(bool currentState)
{
    // ensure baseline loaded first time
    if (baselineRms < 0)
    {
        loadBaselineFromSd();
        if (baselineRms < 0)
        {
            logPrintln("Baseline not present on SD, will calibrate from first window.");
        }
        else
        {
            logPrintln("Loaded baseline RMS: " + String(baselineRms));
        }
    }

    float rms = sampleWindowRms();

    // If baseline unknown, initialize it to the first measured RMS (conservative)
    if (baselineRms < 0)
    {
        lastPersist = millis();
        baselineRms = rms;
        persistBaselineToSd();
        logPrintln("Calibrating OFF baseline: " + String(baselineRms));
        return false;
    }

    // update adaptive baseline slowly (EMA) *only when we believe compressor is OFF*
    if (!currentState)
    {
        baselineRms = (1.0f - EMA_ALPHA) * baselineRms + EMA_ALPHA * rms;
    }

    if (!currentState && rms < baselineRms * 1.2f)
    {
        baselineRms = baselineRms * (1 - EMA_ALPHA) + EMA_ALPHA * rms;
    }

    float onThreshold = baselineRms * THRESH_ON_MULT;
    float offThreshold = baselineRms * THRESH_OFF_MULT;

    logPrintln("RMS: " + String(rms, 2) + " baseline: " + String(baselineRms, 2) +
               " thr_on: " + String(onThreshold, 2) + " thr_off: " + String(offThreshold, 2));

    if (millis() - lastPersist > PERSIST_INTERVAL_MS)
    {
        persistBaselineToSd();
        lastPersist = millis();
        logPrintln("Baseline persisted: " + String(baselineRms));
    }

    // decision logic with hysteresis & required consecutive windows
    if (!currentState)
    {
        if (rms >= onThreshold)
        {
            onCounter++;
            offCounter = 0;
        }
        else
        {
            onCounter = 0;
        }
        if (onCounter >= REQ_ON_COUNTS)
        {
            onCounter = 0;
            offCounter = 0;
            lastPersist = millis();
            return true;
        }
        return false;
    }
    else
    {
        // compressor is currently ON
        if (rms <= offThreshold)
        {
            offCounter++;
            onCounter = 0;
        }
        else
        {
            offCounter = 0;
        }
        if (offCounter >= REQ_OFF_COUNTS)
        {
            offCounter = 0;
            onCounter = 0;
            return false;
        }
        return true;
    }
}

std::vector<float> tempvect;
std::vector<float> savedReadings;
static int readings = 30;        //TODO change back to 30
static const int window = 5000; // window length used to compute RMS (ms)
//static int onCounter = 0;
//static int offCounter = 0;
static int timeCounter = 0;
// static const int SAMPLE_INTERVAL_US = 10000; // sample spacing (us) -> 1 kHz sampling

std::vector<float> simpleTempDetect()
{
    if (tempvect.size() < readings)
    {
        tempvect.push_back(getTemp());
    }
    else
    {
        tempvect.push_back(getTemp());
        tempvect.erase(tempvect.begin());
        tempvect.shrink_to_fit();
        float delta = (tempvect.back() - tempvect[readings * 0.666]);
        logPrint("Temp Delta: " + String(delta) + ", " + String(tempvect[readings * 0.666]) + ", " + String(tempvect.back()));
        logPrint("\n");
        delay(window);
        timeCounter++;
        if (timeCounter == 6) // TODO change back to 6
        {
            timeCounter = 0;
            if (delta > 0)
            {
                onCounter++;
                offCounter = 0;
                for (float i : tempvect)
                {
                    savedReadings.push_back(i);
                }
                Serial.println("onCounter incremented to: " + String(onCounter));
            }
            else if (delta < 0) 
            {
                offCounter++;
                onCounter = 0;
                savedReadings.clear();
                savedReadings.shrink_to_fit();
                Serial.println("offCounter incremented to: " + String(offCounter));
            }
            else
            {
                onCounter = 0;
                offCounter = 0;
                savedReadings.clear();
                savedReadings.shrink_to_fit();
                Serial.println("Delta in middle range, resetting counters");
            }
            Serial.println("Checking: onCounter=" + String(onCounter) + ", offCounter=" + String(offCounter));
            if (onCounter == 4)
            {
                onCounter = 0;
                Serial.println("onDetected");
                tempvect.clear();
                tempvect.shrink_to_fit();
                return tempvect;
            }
            if (offCounter == 4)
            {
                offCounter = 0;
                Serial.println("offDetected");
                tempvect.clear();
                tempvect.shrink_to_fit();
                return {-1};
            }
        }
    }
    return {0};
}

/*

// bool compressorRunning(bool stateNow)
// {
//     const int SAMPLES = 1000;  // 1 second at 1ms intervals
//     const float THRESHOLD_MULTIPLIER = 1.0;  // 3x baseline variance
//     static float baselineVariance = 5000;//80; // 900; //809.11;// 857.69; //-1;  // Computed once when off //434.42 whencompressor running
//     static float lowVarianceBasleine = 40;
//     static int consecutiveDetections = 0;
//     const int REQUIRED_DETECTIONS = 4;

//     // Collect samples
//     int readings[SAMPLES];
//     long sum = 0;

//     for (int i = 0; i < SAMPLES; i++)
//     {
//         readings[i] = analogRead(PIEZO_PIN);
//         sum += readings[i];
//         delay(1);  // 1ms between samples
//     }

//     // Calculate mean
//     float mean = sum / (float)SAMPLES;

//     // Calculate variance (measure of vibration intensity)
//     float variance = 0;
//     for (int i = 0; i < SAMPLES; i++)
//     {
//         float diff = readings[i] - mean;
//         variance += diff * diff;
//     }
//     variance = variance / SAMPLES;

//     // First time: establish baseline (when compressor is off)
//     //TODO change this to be accurate to the first time that the compressor turns on.
//     if (baselineVariance < 0)
//     {
//         baselineVariance = variance;
//         logPrintln("Baseline variance set: " + String(baselineVariance));
//         return false;
//     }
//     // Check if current variance exceeds threshold
//     if (stateNow == false) //compressor off looking for on
//     {
//         logPrintln("Current variance: " + String(variance) + " (baseline: " + String(baselineVariance) + ")");
//         if (variance > baselineVariance * THRESHOLD_MULTIPLIER)
//         {
//             consecutiveDetections++;
//             if (consecutiveDetections >= REQUIRED_DETECTIONS)
//             {
//                 return false; //TODO change so it happens
//             }
//         }
//         else
//         {
//             consecutiveDetections = 0;  // Reset if we miss one
//         }
//         return false;
//     }
//     else    //compressor on looking for off
//     {
//         logPrintln("Current variance: " + String(variance) + " (baseline: " + String(lowVarianceBasleine) + ")");
//         if (variance < lowVarianceBasleine)
//         {
//             consecutiveDetections++;
//             if (consecutiveDetections >= REQUIRED_DETECTIONS)
//             {
//                 return false;
//             }
//         }
//         else
//         {
//             consecutiveDetections = 0;  // Reset if we miss one
//         }
//         return true;
//     }

// }
bool compressorRunning(bool stateNow)
{
    const int SAMPLES = 400;       // Faster detection, ~400ms
    const int REQUIRED_DETECTIONS = 3;

    static bool calibratedOff = false;
    static bool calibratedOn = false;

    static float offBaseline = 0;  // true OFF variance (~50–200)
    static float onBaseline = 0;   // true ON variance (~3000–14000)

    static int confirmCount = 0;

    // ---------------------------------------
    // Collect samples
    // ---------------------------------------
    long sum = 0;
    int readings[SAMPLES];

    for (int i = 0; i < SAMPLES; i++)
    {
        readings[i] = analogRead(PIEZO_PIN);
        sum += readings[i];
        delayMicroseconds(800); // ~1ms sampling
    }

    float mean = sum / float(SAMPLES);

    float variance = 0;
    for (int i = 0; i < SAMPLES; i++)
    {
        float d = readings[i] - mean;
        variance += d * d;
    }
    variance /= SAMPLES;

    logPrintln("Variance: " + String(variance) + " baseline: " + String(offBaseline));

    // ---------------------------------------
    // OFF baseline calibration (first startup)
    // ---------------------------------------
    if (!calibratedOff && !stateNow)
    {
        offBaseline = (offBaseline == 0) ? variance : (offBaseline * 0.8 + variance * 0.2);

        if (offBaseline > 0 && variance < offBaseline * 1.5)
            calibratedOff = true;

        logPrintln("Calibrating OFF baseline: " + String(offBaseline));
        return false;
    }

    // ---------------------------------------
    // ON baseline calibration (first detection)
    // ---------------------------------------
    if (!calibratedOn && stateNow)
    {
        onBaseline = (onBaseline == 0) ? variance : (onBaseline * 0.8 + variance * 0.2);

        if (onBaseline > offBaseline * 3)
            calibratedOn = true;

        logPrintln("Calibrating ON baseline: " + String(onBaseline));
        return true;
    }

    // ---------------------------------------
    // Thresholding Logic
    // ---------------------------------------
    float onThreshold = offBaseline * 4;  // OFF <200, ON >800 typically
    float offThreshold = onBaseline * 0.2; // ON >2000, OFF <400 typically

    if (!stateNow) // searching for ON
    {
        if (variance > onThreshold)
        {
            if (++confirmCount >= REQUIRED_DETECTIONS)
                return true;
        }
        else confirmCount = 0;

        return false;
    }
    else // searching for OFF
    {
        if (variance < offThreshold)
        {
            if (++confirmCount >= REQUIRED_DETECTIONS)
                return false;
        }
        else confirmCount = 0;

        return true;
    }
}
*/