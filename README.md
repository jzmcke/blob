# blob
A generic data serialization and transmission module. Send ints, unsigned ints, floats between C, node, python, javascript, using websockets or UDP. More x-platform and communication protocols coming soon.

# Key use-cases
- Stream data easily between networked devices
- Log data from networked embedded devices into structured datasets (csv, netCDF) incredibly easily
- Live-plot information dense content at relatively high datarates.


# Demo

Below, we have a python script streaming audio from my PC to an embedded ESP32 smart speaker.

1. The python script reads the audio streaming to my output device, uses the blob python API to transmit it to the blob server.

2. The blob server forwards the packet to the ESP32 which has the blob C++ API retreiving the packets.

3. The ESP32 is then performing a real-time RMS calculation on the device, and playing the audio out of its speakers.

4. The ESP32 then uses the blob C++ API to transmit the RMS information back to the server.

5. The server is simultaneously forwarding both the computer's output audio and the ESP32's calculated RMS to a webbrowser interface, which is used to easily plot these variables in real-time.

https://user-images.githubusercontent.com/24628972/235375371-c40adc4f-1375-4707-8d84-fbb4d6fb64f6.mov

# Support

- C/C++ development on Linux and OSX.
- C/C++ development on ESP32. Tested specifically on ESP32-S3 devices*.
- Python development on all platforms.
- Javascript (browser) via websockets
- Javscript (node) via websockets and UDP

* There is a current limitation on a given received UDP packet size < 1472 bytes for ESP32 devices. This is due to ESP32 AsyncUDP not handling UDP fragmentation.
