# 📟 Wearable Health Monitoring System

A comprehensive health monitoring project using ESP32, MPU6050 sensor, heart rate sensor, and an OLED display. It tracks steps, detects falls, measures temperature and heart rate, and provides real-time data via a web server.

---

## ⚙️ Features

- 🚶‍♂️ **Step Counting** with debounce and threshold filtering  
- 🤕 **Fall Detection** using accelerometer magnitude threshold  
- 🌡️ **Temperature Monitoring** via MPU6050 sensor  
- ❤️ **Heart Rate Monitoring** with periodic averaging  
- 🕒 **Real-Time Clock** synced with NTP server  
- 📱 **Web Server Interface** serving JSON data for remote monitoring  
- 🖥️ **OLED Display** shows time, steps, temperature, heart rate, and notifications  
- 🔘 **Button Control** to cycle display screens and activate health check notifications  
- 🚨 **Emergency Notification** on fall detection with blinking alerts  

---

## 🛠️ Hardware Components

- ESP32 Development Board  
- MPU6050 Gyroscope and Accelerometer Sensor  
- Heart Rate Sensor (analog input)  
- SSD1306 OLED Display (128x64)  
- Push Button (for UI control)  
- WiFi Network for connectivity  

---

## 📡 Software Details

- Uses **Adafruit_SSD1306** and **Adafruit_GFX** libraries for OLED display  
- **MPU6050** library for motion sensing  
- **WiFi** and **WebServer** libraries for network communication  
- **NTPClient** for accurate time synchronization  
- Implements filtering and thresholding for reliable step and fall detection  
- Provides a JSON API endpoint `/` for external data access  

---

## 🔧 How It Works

- Initializes sensors and connects to WiFi  
- Continuously reads MPU6050 gyro and accelerometer data  
- Filters gyro Z-axis data to detect steps and counts them  
- Detects falls when acceleration magnitude exceeds a set threshold  
- Reads heart rate sensor every 5 seconds and averages over 5 minutes  
- Displays current data on OLED; button press cycles through views:  
  - Time ⏰  
  - Step count 🚶‍♂️  
  - Temperature 🌡️  
  - Heart rate ❤️  
- Long button press activates a 5-minute health check summary screen  
- Web server hosts real-time JSON data for remote monitoring  

---

## 📋 API Endpoint

- `GET /`  
  Returns JSON with current sensor data:  
{
"stepCount": 1234,
"temperature": 36.5,
"heartRate": 72,
"time": "12:34:56",
"fallDetected": false,
"warning": false
}


---

## 🔌 Pin Configuration

| Component         | Pin Number | Description                  |
|-------------------|------------|------------------------------|
| Heart Rate Sensor  | 34         | Analog input for heart rate  |
| Button            | 18         | Input with pull-up resistor  |
| OLED Display      | I2C (SDA/SCL) | I2C communication           |
| MPU6050           | I2C (SDA/SCL) | I2C communication           |

---

## 📥 Setup Instructions

1. Connect hardware components as per pin configuration.  
2. Update WiFi credentials in the code (`ssid` and `password`).  
3. Upload the code to ESP32 using Arduino IDE.  
4. Open Serial Monitor to see connection status.  
5. Access the device IP on port 80 to get JSON data.  
6. Use the button to navigate OLED display screens.  

---

## 🤖 Usage Tips

- Adjust thresholds (`gyroThreshold`, `accelThreshold`, `fallThreshold`) for sensitivity tuning.  
- Ensure heart rate sensor calibration for accurate BPM mapping.  
- Use long button press to view health summary every 5 minutes.  
- Monitor fall alerts on OLED and web interface for emergency response.  

---

## 📈 Future Improvements

- Add SMS or push notifications on fall detection  
- Integrate GPS for location tracking  
- Store historical data for trend analysis  
- Add more health parameters like SpO2  

---

Made with ❤️ using ESP32 and sensors to empower personal health monitoring! 🚀
