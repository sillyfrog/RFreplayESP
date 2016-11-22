#ifndef rf433mhz_h
#define rf433mhz_h

#include "Arduino.h"
#include "FS.h"

class RF433
{
    public:
        RF433(int rf_rx, int rf_tx);
        void setup();
        int sendSignal(String signame);
        int recordSignal(String signame);
    private:
        int _rx_pin;
        int _tx_pin;
};

#endif
