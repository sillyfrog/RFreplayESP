
#include <RF433.h>
#include "FS.h"

RF433 rf433(-1, D2);

void setup() {
  Serial.begin(115200);

  Serial.print("\nSetup file system:");
  Serial.println(SPIFFS.begin());
  
  rf433.setup();

}

void loop() {
  Serial.println("\n\nStarting Send...");
  int result = rf433.sendSignal("test");
  Serial.print("Complete, return code: ");
  Serial.println(result);
  delay(5000);
}
