#include <Arduino.h>
#include <RF433.h>

#ifdef ESP32
RF433 rf433(15, -1);
#else
RF433 rf433(D1, -1);
#endif

void setup()
{
    Serial.begin(115200);

    // This will setup the LittleFS file system
    rf433.setup();
}

int file_id = 0;
void loop()
{
    Serial.print("\n\nReady to receive, in 3  ");
    delay(1000);
    Serial.print("2  ");
    delay(1000);
    Serial.print("1  ");
    delay(1000);
    Serial.println("\nStarting receive...");
    String filename = "test_" + String(file_id);
    int result = rf433.recordSignal(filename);
    if (result == 0)
    {
        Serial.println("Receive completed successfully, file written to: " + filename);
        file_id++;
    }
    else
    {
        Serial.print("Receive errored, no file was written, return code: ");
        Serial.println(result);
    }
    delay(2000);
}
