
#include <RF433.h>
#include "FS.h"

RF433 rf433(D1, -1);

void setup() {
  Serial.begin(115200);

  Serial.print("\nSetup file system:");
  Serial.println(SPIFFS.begin());

  rf433.setup();

}

void loop() {
  Serial.println("\n\nStarting receive...");
  int result = rf433.recordSignal("test");
  if (result == 0) {
    Serial.println("Receive completed successfully!");
  } else {
    Serial.print("Receive errored, no file was written, return code: ");
    Serial.println(result);
  }
  delay(5000);
}
