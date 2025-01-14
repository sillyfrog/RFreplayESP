#include <Arduino.h>
#include <RF433.h>
#include <LittleFS.h>

#define MAX_SIGNAL_LEN 2000

/* Init
 * rf_rx: the pin the receiver is connected to.
 * rf_tx: the pin the transmitter is connected to.
 *
 * If a pin is not required (i.e.: you just need transmit), use a negative
 * number for the pin.
 */
RF433::RF433(int rf_rx, int rf_tx)
{
    _rx_pin = rf_rx;
    _tx_pin = rf_tx;
}

void RF433::setup()
{
    pinMode(_tx_pin, OUTPUT);
    pinMode(_rx_pin, INPUT);
    LittleFS.begin(true);
    LittleFS.mkdir("/signals");
}

/* Send a recorded signal
 * signame: the name of the signal that has been recorded. This is a file on
 * the SPIFFS file system in the /signals/ directory, with a .txt extension.
 *
 * Will return non-zero on error, currently the only error is an unknown
 * signame (file name).
 */
int RF433::sendSignal(String signame)
{
    if (_tx_pin < 0)
    {
        return 1;
    }
    int state = 0;
    unsigned long ndelay;
    String filename;
    if (signame.startsWith("/signals/"))
    {
        // We have a full file path, so use it
        filename = signame;
    }
    else
    {
        filename = "/signals/" + signame + ".txt";
    }

    File bFile = LittleFS.open(filename, "r");
    size_t fsize = bFile.size();
    yield();

    if (fsize == 0)
    {
        // The file is empty or does not exist, abort
        return 2;
    }

#ifdef RFDEBUG
    Serial.print("Signal file size: ");
    Serial.println(fsize);
#endif

    unsigned long nexttime = micros();
    while (bFile.position() < fsize)
    {
        state = !state;
        digitalWrite(_tx_pin, state);
        ndelay = bFile.parseInt();
        nexttime += ndelay;
        while (micros() < nexttime)
        {
            // pass
        }
    }
    yield();
    digitalWrite(_tx_pin, LOW);
    return 0;
}

/* Record a signal
 * signame: The name of the signal to record to the SPIFFS file system.
 * If RFDEBUG is on, this will also print out the file contents allowing for
 * an easy copy and paste into a file.
 * Will return 0 on success. Errors include a signal that's too long (overflows).
 */
int RF433::recordSignal(String signame)
{
#ifdef RFDEBUG
    Serial.print("Ready to record: ");
    Serial.println(signame);
#endif
    if (_rx_pin < 0)
    {
        return 1;
    }
    int val = 0;
    int lastval = 0;
    // Static is important here otherwise the system will crash.
    // I think this maybe the stack overflowing, but not sure
    static unsigned long sig[MAX_SIGNAL_LEN + 1];
    unsigned long lasttime;
    int siglength = 0;
    unsigned long aborttime = micros() + (5 * 1000 * 1000); // Abort after 5 seconds

    yield();
    while (1)
    {
        unsigned long mnow = micros();
        // Invert the digitalRead should the receiver be inverted.
        // ie: high when no signal.
        // val = !digitalRead(_rx_pin);
        val = digitalRead(_rx_pin);
        if (val != lastval)
        {
            sig[siglength] = mnow;
            lasttime = mnow;
            siglength += 1;
            lastval = val;
        }
        if (mnow > aborttime)
        {
            return 2;
        }
        yield();
        // if (siglength == MAX_SIGNAL_LEN) {
        //     return 3;
        // }
        if ((siglength > 1) && ((mnow - lasttime) > 30000 || siglength == MAX_SIGNAL_LEN))
        {
            if (siglength < 10)
            {
                // Too small, assume a failure
                return 1;
            }
#ifdef RFDEBUG
            Serial.print("\nCapture Complete. Signal Length: ");
            delay(5);
            Serial.println(siglength);
            Serial.println("======= Start Signal Below Here ==========");
            for (int i = 1; i < siglength; i++)
            {
                Serial.print(sig[i] - sig[i - 1]);
                Serial.print(", ");
            }
            Serial.println("");
            Serial.println("======= End Signal Here ==========");
#endif
            yield();
            File bFile = LittleFS.open("/signals/" + signame + ".txt", "w");
            for (int i = 1; i < siglength; i++)
            {
                bFile.print(sig[i] - sig[i - 1]);
                bFile.print(", ");
                yield();
            }
            yield();
#ifdef RFDEBUG
            Serial.print("File saved to: ");
            Serial.println("/signals/" + signame + ".txt");
#endif
            return 0;
        }
    }
}
