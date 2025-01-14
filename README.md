# ESP-RF-Replay
Record and replay RF433Mhz signals on an ESP8266 or ESP32, as a PlatformIO library.

## Design goal

My goal with this is to have a set of RF codes I can send over and over from an ESP. As I'm getting the board to do a lot, I'm also keen to keep memory usage low, and not make it hard to add new codes or modify existing ones. It is not designed to receive codes, except for the recording phase.

This library is much dumber than rc-switch (https://github.com/sui77/rc-switch), as I found this did not work for my application, and all I needed was a simple record and replay.

## Recording codes

The RF_Record folder has the code to record signals on an ESP, it is designed to be the basis used to record all the codes needed, then never used again. My goal is to automate parts of my home, including roller blinds and garage door. The basic usage is:

- Create a new PlatfromIO project using the RF_Record folder as the base.
- Update the `platformio.ini` file to match your board.
- Flash to your ESP
- Connect the ESP up with your RF receiver (more info below on what I used).
- Wait for the count down, and press a button on your RF remote, it will print a set of codes via the serial console, including the file name.
- Copy the string of numbers for each button into a .txt file for later use. The name of the file is what is used by the sending library, so call it something you'll remember later (or take note of the file name/number).
- I found I got a bit of interference, so you may need to take a few goes until one feels "right". This will typically be the shortest one, but you should have several in a row where the string of numbers is roughly the same.


## Replaying codes

Now you have a bunch of recorded codes, they can be uploaded to the ESP with your main code on it (see the examples, RF_Send is a simple one to start with). The files are uploaded to the LittleFS file system in a `/signals/` directory. 

If you run RF_Record, you can then flash RF_Send, and it will re-transmit the same signal.

Once uploaded, you can call `sendSignal`. The input to `sendSignal` is the name of the file saved above (without the extension). It will then replay the RF signal, and all going to plan, whatever it is should happen (open the garage, close the blind etc).

# Important

When first initialized, this library will run `LittleFS.begin` and create the `/signals/` directory.

# Examples

For all the following examples, start a new project with the nested folder structure, and update the `platformio.ini` file to match your board. Note these all include this library as a dependency directly from GitHub, so you don't need to do anything else.

- RF_Record: Basic example to record RF signals (for later use)
- RF_Send: Basic example to send RF signals (from the recorded files)
- Web_RF_Record: Basic example to record RF signals, and then download via a web interface
- Web_RF_Send: Basic example to send RF signals, with a web interface to select the signal to send

# Uploading saved files

If you want to upload saved recording files:

- In your PlatformIO project, create a `/data/signals/` directory in the root of the project.
- Copy in all your signal files (including the `.txt` extension).
- Upload the files to the ESP by running `pio run --target uploadfs` from the VScode terminal, the latest version of PlatformIO should automatically us _LittleFS_.

# More info

See the wiki for the hardware and setup that I used with this code.

