# ESP-RF-Replay
Record and replay RF433Mhz signals on an ESP8266, using the Arduino IDE.

## Design goal

My goal with this is to have a set of RF codes I can send over and over from an ESP8266. As I'm getting the board to do a lot, I'm also keen to keep memory usage low, and not make it hard to add new codes or modify existing ones. It is not designed to receive codes, except for the recording phase.

I'm using the Arduino IDE, with the excellent ESP8266 libraries from here: https://github.com/esp8266/Arduino/ 

This library is much dumber than rc-switch (https://github.com/sui77/rc-switch), as I found this did not work for my application, and all I needed was a simple record and replay.

## Recording codes

The RF_Record.ino file has the code to record signals on an ESP, it is designed to be flashed, used to record all he codes needed, then never needed again. My goal is to automate parts of my home, including roller blinds and garage door. The basic usage is:

- Flash RF_Record.ino to your ESP
- Connect the ESP up with your RF receiver (more info below on what I used).
- Each time you press a button on your RF remote, it will print a set of codes via the serial console.
- Copy the string of numbers for each button into a .txt file for later use. The name of the file is what is used by the sending library, so call it something you'll remember later.
- I found I got a bit of interference, so you may need to take a few goes until one feels "right". This will typically be the shortest one, but you should have several in a row where the string of numbers is roughly the same.


## Replaying codes

Now you have a bunch of recorded codes, they can be uploaded to the ESP with your main code on it (see the examples, RF_Send.ino is a simple one to start with). The files are uploaded to the SPIFFS file system in a `/signals/` directory. For more information, see here: https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#file-system-object-spiffs  The only thing that's really important is to be calling `SPIFFS.begin();` in your setup. See the previous link for ways to upload files to the filesystem, the web example allow you to do this via the web interface.

If you run RF_Record.ino, you can then flash RF_Send.ino, and it will re-transmit the same signal.

Once uploaded, you can call `sendSignal`. The input to `sendSignal` is the name of the file saved above (without the extension). It will then replay the RF signal, and all going to plan, whatever it is should happen (open the garage, close the blind etc).


# More info

See the wiki for the hardware and setup that I used with this code.

