# Refrigerator-Compressor-Monitor
Code for an ESP32-based device which monitors vibrations and temperature of a refrigerator compressor to determine if it is failing based on statistical tests comparing its current state to its original state.

# Credits
* Grant Leeman - Data storage
* Olivia Anderson - Housing
* Josh Gabel - Telegram Bot
* James Barnett - Temperature Sensing
* Brendan Herman - Vibration Sensing
___
# Build Procedure
<details>
<summary><h2>Components</h2></summary>

* ESP32 Dev Kit V1
* DS1AB20 Temperature Probe
* SD Card Module
* 128GB SD Card
* Piezoelectric Vibration Sensor with signal processing board
    * 5.2 V Zener Diode
    * 105 Ohm Pull-Down Resistor
* 10 ft USB-A to USB-B cable
* 4 10-ft solid-core wires
* Various Waygo connectors and breadboard jumper wire
</details>

<details>
   <summary><h2>System Diagram</h2></summary>

   
</details>
<details>
   <summary><h2>Installation</h2></summary>

   
</details>

___

# Code
<details>
   <summary>Notes</summary>

   * Use the PlatformIO VSCode extension
</details>

The system runs two tasks. The networking is performed in the background on ESP32's core 0, while the main code is executed on core 1.
