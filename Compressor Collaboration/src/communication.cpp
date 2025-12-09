#include "communication.h"

Preferences preferences;

const char* ssid = NONE;              //Network Name
const char* password = NONE;     //Network Password

String botToken = NONE;     //ID of the Bot 

String chatID = NONE;      //ID of the Telegram chat

long lastUpdateId = 0;

// Message IDs
long msgStatusId = 0;
long msgTempId = 0;
long msgVibId = -1;
long msgAlertId = 0;

// Thresholds
float TEMP_CRIT_THRESHOLD = 3;
float TEMP_WARNING_THRESHOLD = 2;
float VIBR_CRIT_THRESHOLD = 3;
float VIBR_WARNING_THRESHOLD = 2;

// Status tracking
bool failureStatus = false;
long critCounter = 0;
long warnCounter = 0;

String lastStatus = "✅ Normal operation.";
String lastTempStr = "";
String lastVibStr = "";


// --------------------- STATUS CHECK -------------------------
void statusCheck(float zTemp, float zVibr, String& lStatus) {

    bool doubleWarn = 
        (zTemp >= TEMP_WARNING_THRESHOLD && zTemp < TEMP_CRIT_THRESHOLD) &&
        (zVibr >= VIBR_WARNING_THRESHOLD && zVibr < VIBR_CRIT_THRESHOLD);

    String newStatus;

    if ((zTemp >= TEMP_CRIT_THRESHOLD || zVibr >= VIBR_CRIT_THRESHOLD) || doubleWarn) {//either both z scores irregular or a double warning warrants a critical failure notification
        failureStatus = true;
        newStatus = "❌ CRITICAL: Compressor overheating and vibrating too much.";
        critCounter++;

        if (lStatus != newStatus) {
            sendCriticalAlert();
            preferences.putLong("msgAlertId", msgAlertId);
        }
    }
    else if (zTemp >= TEMP_WARNING_THRESHOLD || zVibr >= VIBR_WARNING_THRESHOLD) {//single irregularity
        failureStatus = true;
        newStatus = "⚠️ Warning: Elevated temperature or vibration.";
        warnCounter++;

        if (lStatus != newStatus) {
            sendWarningAlert();
            preferences.putLong("msgAlertId", msgAlertId);
        }
    }
    else {
        failureStatus = false;
        newStatus = "✅ Normal operation.";

        if (lStatus != newStatus) {
            deleteMessage(msgAlertId);
        }
    }

    if (newStatus != lastStatus) {
        updateMessage(msgStatusId, "Fridge Compressor 1 Status: " + newStatus);
        lastStatus = newStatus;
        delay(200);
    }
}


// --------------------- UPDATE DATA -------------------------
void updateDataMessages(String TStr, String& lTStr, String VStr, String& lVStr) {

    if (TStr != lTStr) {
        updateMessage(msgTempId, "Temperature: " + TStr);
        lTStr = TStr;
        //delay(200);
    }

    if (VStr != lVStr) {
        updateMessage(msgVibId, "Vibration: " + VStr);
        lVStr = VStr;
        //delay(200);
    }
}

void preferencesStartup(bool isNew){

    preferences.begin("msgIDs", false);

    if (isNew){

        msgStatusId = sendTelegramMessage("Fridge Compressor 1 Status: Program Setup");
        delay(200);
        msgTempId = sendTelegramMessage("Temperature: -- °C");
        delay(200);
        msgVibId = sendTelegramMessage("Vibration: -- Hz");
        delay(200);
        

        preferences.putLong("msgStatusId", msgStatusId);
        preferences.putLong("msgTempId", msgTempId);
        preferences.putLong("msgVibId", msgVibId);
    }
    else
    {
        msgStatusId = preferences.getLong("msgStatusId", 0);
        msgTempId = preferences.getLong("msgTempId", 0);
        msgVibId = preferences.getLong("msgVibId", 0);
        msgAlertId = preferences.getLong("msgAlertId", 0);
    }
}

// --------------------- TELEGRAM GET UPDATES -------------------------
/*void checkTelegram() {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken +
                 "/getUpdates?offset=" + String(lastUpdateId + 1);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode <= 0) {
        Serial.println("getUpdates failed, code: " + String(httpCode));
        http.end();
        return;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(4096);

    if (deserializeJson(doc, payload)) {
        Serial.println("JSON Error");
        http.end();
        return;
    }

    JsonArray results = doc["result"].as<JsonArray>();

    for (JsonObject msg : results) {
        long updateId = msg["update_id"].as<long>();
        lastUpdateId = updateId;

        if (!msg.containsKey("message")) continue;

        String text = msg["message"]["text"].as<String>();

        if (text == "/status") {

            deleteMessage(msgStatusId);
            deleteMessage(msgTempId);
            deleteMessage(msgVibId);

            msgStatusId = sendTelegramMessage("Fridge Compressor 1 Status: " + lastStatus);
            delay(200);
            msgTempId = sendTelegramMessage("Temperature: " + String(tempNow, 1) + "°C");
            delay(200);
            msgVibId = sendTelegramMessage("Vibration: " + String(vibNow, 3) + "g");
            delay(200);

            preferences.putLong("msgStatusId", msgStatusId);
            preferences.putLong("msgTempId", msgTempId);
            preferences.putLong("msgVibId", msgVibId);
        }

        else if (text == "/clear") {
            deleteMessage(msgStatusId);
            deleteMessage(msgTempId);
            deleteMessage(msgVibId);
            delay(300);

            msgStatusId = sendTelegramMessage("Fridge Compressor 1 Status: ❌");
            delay(200);
            msgTempId = sendTelegramMessage("Temperature: --°C");
            delay(200);
            msgVibId = sendTelegramMessage("Vibration: --");
            delay(200);

            preferences.putLong("msgStatusId", msgStatusId);
            preferences.putLong("msgTempId", msgTempId);
            preferences.putLong("msgVibId", msgVibId);

            lastStatus = "";
            lastTempStr = "";
            lastVibStr = "";
        }
    }

    http.end();
}
*/

// --------------------- ALERTS -------------------------
void sendWarningAlert() {
    deleteMessage(msgAlertId);
    msgAlertId = sendTelegramMessage(
        "⚠️ WARNING: COMPRESSOR 1 IS SHOWING SIGNS OF FAILURE ⚠️ Count: " +
        String(warnCounter)
    );
}

void sendCriticalAlert() {
    deleteMessage(msgAlertId);
    msgAlertId = sendTelegramMessage(
        "❌ CRITICAL: COMPRESSOR 1 IS IN CRITICAL CONDITION ❌ Count: " +
        String(critCounter)
    );
}


// --------------------- TELEGRAM SEND -------------------------
// long sendTelegramMessage(String message) {
//     if (WiFi.status() != WL_CONNECTED) return -2;

//     HTTPClient http;
//     String url = "https://api.telegram.org/bot" + botToken +
//                  "/sendMessage?chat_id=" + chatID +
//                  "&text=" + message;

//     http.begin(url);
//     int code = http.GET();

//     long messageId = -3;

//     if (code == 200) {
//         DynamicJsonDocument doc(1024);
//         deserializeJson(doc, http.getString());
//         messageId = doc["result"]["message_id"].as<long>();
//     } else {
//         Serial.println("ERROR Response: " + http.getString());
//     }
     
//     Serial.println(String(code));
    
//     http.end();

//     return messageId;
// }

// --------------------chat telegram send ---------------------- //

long sendTelegramMessage(String message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return 0;
    }

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Use JSON body instead of URL parameters
    String payload = "{\"chat_id\":\"" + chatID + "\",\"text\":\"" + message + "\"}";
    
    int code = http.POST(payload);
    
    Serial.println("Message: " + message);
    //Serial.println("HTTP Response code: " + String(code));
    
    long messageId = 0;

    if (code == 200) {
        String response = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        messageId = doc["result"]["message_id"].as<long>();
    } else {
        Serial.println("ERROR Response: " + http.getString());
    }

    http.end();
    return messageId;
}


// --------------------- TELEGRAM EDIT -------------------------
void updateMessage(long messageId, String newText) {
    if (messageId == 0) return;

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken +
                 "/editMessageText?chat_id=" + chatID +
                 "&message_id=" + String(messageId) +
                 "&text=" + newText;

    http.begin(url);
    http.GET();
    http.end();
}


// --------------------- TELEGRAM DELETE -------------------------
void deleteMessage(long messageId) {
    if (messageId == 0) return;

    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken +
                 "/deleteMessage?chat_id=" + chatID +
                 "&message_id=" + String(messageId);

    http.begin(url);
    http.GET();
    http.end();
}
