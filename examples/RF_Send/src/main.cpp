#include <Arduino.h>
#include <RF433.h>

#ifdef ESP32
RF433 rf433(-1, 16);
#else
RF433 rf433(-1, D2);
#endif

void setup()
{
    Serial.begin(115200);

    // This will setup the LittleFS file system
    rf433.setup();
}

void loop()
{
    Serial.println("\n\nStarting Send...");
    int result = rf433.sendSignal("test_0");
    Serial.print("Complete, return code: ");
    Serial.println(result);
    delay(5000);
}
