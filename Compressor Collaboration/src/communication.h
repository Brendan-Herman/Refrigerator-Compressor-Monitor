#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

extern Preferences preferences;

// WiFi & Telegram credentials
extern const char* ssid;
extern const char* password;
extern String botToken;
extern String chatID;

// Message IDs
extern long msgStatusId;
extern long msgTempId;
extern long msgVibId;
extern long msgAlertId;
extern long lastUpdateId;

// Thresholds
extern float TEMP_CRIT_THRESHOLD;
extern float TEMP_WARNING_THRESHOLD;
extern float VIBR_CRIT_THRESHOLD;
extern float VIBR_WARNING_THRESHOLD;

// Status tracking
extern bool failureStatus;
extern long critCounter;
extern long warnCounter;

extern String lastStatus;
extern String lastTempStr;
extern String lastVibStr;

// Function prototypes
void statusCheck(float zTemp, float zVibr, String& lStatus);
void updateDataMessages(String TStr, String& lTStr, String VStr, String& lVStr);
void preferencesStartup(bool isNew);

void checkTelegram();

void sendWarningAlert();
void sendCriticalAlert();

long sendTelegramMessage(String message);
void updateMessage(long messageId, String newText);
void deleteMessage(long messageId);

#endif