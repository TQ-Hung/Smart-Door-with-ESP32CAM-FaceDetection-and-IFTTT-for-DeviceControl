# Smart-Door-with-ESP32CAM-FaceDetection-and-IFTTT-for-ControlDevice
In this project, we are focusing on developing ESP32_Camera for the purpose of door opening through facial recognition and device control capabilities via Google Assistant (IFTTT) voice commands.
# Description: 
We are using two separate ESP32s for two purposes, ESP32_CAM for facial recognition and door opening, and a regular ESP32 for device control
- 1.The primary objective of ESP32_CAM is facial recognition and door opening, along with other tasks such as interacting with the chatbox tool on Telegram to execute functions like remote door opening and capturing images from the camera to enhance security and safety.
- 2.The system allows users to use voice commands to execute requests through their phones with support from Google Assistant and IFTTT (Blynk database), thereby creating convenience in daily life. It enables remote device control without the need for extensive physical movement, and offers relatively fast response times
# Limitations: 
- When power is lost, the security system will not function.
- Without internet or Wi-Fi, remote device control is not possible, and device status updates may not be accurate.
- Modules are not yet properly linked together, meaning that successful activation of the facial recognition block to unlock does not necessarily trigger the voice-controlled block to operate.
# Demo:
<p align="center">
  <a href="https://www.youtube.com/watch?v=y_2RpiJP538">
    <img src="https://img.youtube.com/vi/y_2RpiJP538/0.jpg" alt="Demo video" width="50%">
  </a>
</p>
