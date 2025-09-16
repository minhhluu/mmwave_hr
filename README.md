# 🫀 mmWave Heart Rate Monitor on ESP32-C6

This project demonstrates how to monitor **heart rate** using a **mmWave sensor** with the **ESP32-C6 kit**, built on **ESP-IDF**.  
It was migrated from an Arduino IDE prototype to a fully IDF-based workflow for better control, stability, and scalability.

<p align="center">
  <img src="https://s1.ezgif.com/tmp/ezgif-1007165549ed3b.gif" alt="mmWave heart rate demo" width="500"/>
</p>

## ✨ Features
- Heart rate monitoring with mmWave radar sensor  
- ESP32-C6 kit running on ESP-IDF  
- Transition from Arduino IDE to ESP-IDF for professional-grade development  
- Ready for future integration with cloud dashboards or real-time displays


## 🚀 Flashing 
1. Clone this repository:  
   ```bash
   git clone git@github.com:minhhluu/mmwave_hr.git
   cd mmwave_hr
   ```
2. Build & flash the project with ESP-IDF:
   ```bash
   idf.py flash
   ```
3. Monitor serial output:
   ```bash
   idf.py monitor
   ```
