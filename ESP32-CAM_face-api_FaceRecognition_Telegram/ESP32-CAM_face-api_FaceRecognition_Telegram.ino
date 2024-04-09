const char* ssid = "Winz 2G";
const char* password = "12ba45678";


//Nhập mật khẩu tài khoản kết nối AP  http://192.168.4.1
const char* apssid = "esp32-cam";
const char* appassword = "12345678";         

String myToken = "6389537037:AAFRCB5MMrNx-g-LgdVtnCRU6BpQNlDRIMA";   
String myChatId = "6089739891";   

int pinDoor = 2;  //Chân relay khóa cửa IO2
long message_id_last = 0;  //Giá trị ban đầu của mã tin nhắn Telegram
int timer = 0;  //Số giây Telegram chờ lệnh nhắn tin
int timerLimit = 10;  //Telegram chờ lệnh nhắn tin vài giây, màn hình video sẽ tạm dừng cho đến khi hết thời gian 10 giây

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"         
#include "soc/soc.h"            
#include "soc/rtc_cntl_reg.h"   
#include <ArduinoJson.h>
      

String Feedback="";   //Trả lời tin nhắn
//Giá trị tham số lệnh
String Command="",cmd="",P1="",P2="",P3="",P4="",P5="",P6="",P7="",P8="",P9="";
//Giá trị trạng thái tháo gỡ lệnh
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


WiFiServer server(80);
WiFiClient client;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  Serial.println();


  //Cài đặt cấu hình video
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  //config.xclk_freq_hz = 5000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  //kích thước khung hình thả xuống để có tốc độ khung hình ban đầu cao hơn
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA  Đặt độ phân giải hình ảnh ban đầu

  //Flash
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);    
  
  WiFi.mode(WIFI_AP_STA);  

  for (int i=0;i<2;i++) {

    WiFi.begin(ssid, password);    //Thực hiện kết nối mạng
  
    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;    //Đợi 10 giây để kết nối
    } 
  
    if (WiFi.status() == WL_CONNECTED) {    //Nếu kết nối thành công
      WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //Đặt SSID để hiển thị IP Client      
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
  
      for (int i=0;i<5;i++) {   //Nếu kết nối với WIFI, đèn flash sẽ nhấp nháy nhanh
        ledcWrite(4,10);
        delay(200);
        ledcWrite(4,0);
        delay(200);    
      }
      break;
    }
  } 

  if (WiFi.status() != WL_CONNECTED) {    //Nếu kết nối không thành công
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);         

    for (int i=0;i<2;i++) {    //Nếu không thể kết nối WIFI, đèn flash sẽ nhấp nháy chậm.
      ledcWrite(4,10);
      delay(1000);
      ledcWrite(4,0);
      delay(1000);    
    }
  } 
  
  //Chỉ định IP AP
  Serial.println("");
  Serial.println("APIP address: ");
  Serial.println(WiFi.softAPIP());  
  Serial.println("");  

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);  

  server.begin();          
}

void loop() {
  Feedback="";Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
  client = server.available();

  if (client) { 
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        
        getCommand(c);   
                
        if (c == '\n') {
          if (currentLine.length() == 0) {    
            
            if (cmd=="getstill") {  //Nhận ảnh chụp màn hình video
              getStill(); 
            }
            else if (cmd=="status") {    
              status();           
            }              
            else {  
              mainpage();
            }
                        
            Feedback="";
            break;
          } else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1)) {
          if (Command.indexOf("stop")!=-1) {  
            client.println();
            client.println();
            client.stop();
          }
          currentLine="";
          Feedback="";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void ExecuteCommand() {
  if (cmd!="getstill") {
    Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
    Serial.println("");
  }

  //Khối lệnh tùy chỉnh 
  if (cmd=="your cmd") {
  } else if (cmd=="ip") {  
    Feedback="AP IP: "+WiFi.softAPIP().toString();    
    Feedback+="<br>";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  } else if (cmd=="mac") {  
    Feedback="STA MAC: "+WiFi.macAddress();
  } else if (cmd=="restart") { 
    ESP.restart();
  } else if (cmd=="digitalwrite") {  
    ledcDetachPin(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  } else if (cmd=="digitalread") {  
    Feedback=String(digitalRead(P1.toInt()));
  } else if (cmd=="analogwrite") {  
    if (P1=="4") {
      ledcAttachPin(4, 4);  
      ledcSetup(4, 5000, 8);
      ledcWrite(4,P2.toInt());     
    } else {
      ledcAttachPin(P1.toInt(), 9);
      ledcSetup(9, 5000, 8);
      ledcWrite(9,P2.toInt());
    }
  }       
  else if (cmd=="analogread") {  
    Feedback=String(analogRead(P1.toInt()));
  } else if (cmd=="touchread") {  
    Feedback=String(touchRead(P1.toInt()));
  } else if (cmd=="restart") {  
      ESP.restart();
  } else if (cmd=="flash") {  
    ledcAttachPin(4, 4);  
    ledcSetup(4, 5000, 8);   
    int val = P1.toInt();
    ledcWrite(4,val);  
  } else if(cmd=="servo") {  
    ledcAttachPin(P1.toInt(), 3);
    ledcSetup(3, 50, 16);
     
    int val = 7864-P2.toInt()*34.59; 
    if (val > 7864)
       val = 7864;
    else if (val < 1638)
      val = 1638; 
    ledcWrite(3, val);
  } else if (cmd=="relay") {  
    pinMode(P1.toInt(), OUTPUT);  
    digitalWrite(P1.toInt(), P2.toInt());  
  } else if (cmd=="uart") {  
    Serial.print(P1);

    //Telegram bot
    if (P1=="unknown") {  //Người lạ
      sendCapturedImage2Telegram(myToken, myChatId);
      String keyboard = "{\"keyboard\":[[{\"text\":\"/open\"},{\"text\":\"/still\"}], [{\"text\":\"/ledon\"},{\"text\":\"/ledoff\"}]],\"one_time_keyboard\":false}";
      sendMessage2Telegram(myToken, myChatId, "Stranger", keyboard);
      getTelegramMessage(myToken);
    } else {  //Chủ nhà
      sendMessage2Telegram(myToken, myChatId, "Welcome back! " + P1, "");
      telegramCommand("/open");
    }
  } else if (cmd=="resetwifi") {  //Đặt lại kết nối mạng  
    for (int i=0;i<2;i++) {
      WiFi.begin(P1.c_str(), P2.c_str());
      Serial.print("Connecting to ");
      Serial.println(P1);
      long int StartTime=millis();
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          if ((StartTime+5000) < millis()) break;
      } 
      Serial.println("");
      Serial.println("STAIP: "+WiFi.localIP().toString());
      Feedback="STAIP: "+WiFi.localIP().toString();

      if (WiFi.status() == WL_CONNECTED) {
        WiFi.softAP((WiFi.localIP().toString()+"_"+P1).c_str(), P2.c_str());
        for (int i=0;i<2;i++) {    //Nếu không thể kết nối WIFI, đèn flash sẽ nhấp nháy chậm.
          ledcWrite(4,10);
          delay(300);
          ledcWrite(4,0);
          delay(300);    
        }
        break;
      }
    }
  } else if (cmd=="framesize") {
    int val = P1.toInt();
    sensor_t * s = esp_camera_sensor_get(); 
    s->set_framesize(s, (framesize_t)val);    
  } else if (cmd=="quality") { //Chất lượng hình ảnh
    sensor_t * s = esp_camera_sensor_get();
    s->set_quality(s, P1.toInt());     
  } else if (cmd=="contrast") {  //Độ tương phản
    sensor_t * s = esp_camera_sensor_get();
    s->set_contrast(s, P1.toInt());          
  } else if (cmd=="brightness") {  //Độ sáng
    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, P1.toInt());   
  } else if (cmd=="saturation") {  //Bão hòa
    sensor_t * s = esp_camera_sensor_get();
    s->set_saturation(s, P1.toInt());          
  } else if (cmd=="special_effect") {  //Hiệu ứng đặc biệt
    sensor_t * s = esp_camera_sensor_get();
    s->set_special_effect(s, P1.toInt());  
  } else if (cmd=="hmirror") {  //Phản chiếu ngang
    sensor_t * s = esp_camera_sensor_get();
    s->set_hmirror(s, P1.toInt());  
  } else if (cmd=="vflip") {  //Lật theo chiều dọc
    sensor_t * s = esp_camera_sensor_get();
    s->set_vflip(s, P1.toInt());  
  } else {
    Feedback="Command is not defined.";
  }
  if (Feedback=="") Feedback=Command;  
}

void getCommand(char c)
{
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) P1=P1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) P2=P2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) P3=P3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) P4=P4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) P5=P5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) P6=P6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) P7=P7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) P8=P8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) P9=P9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}

//Giao diện quản lý trang chủ web tùy chỉnh
static const char PROGMEM INDEX_HTML[] = R"rawliteral(<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>ESP32 OV2460</title>
        <style>
            body {
                font-family: Arial,Helvetica,sans-serif;
                background: #181818;
                color: #EFEFEF;
                font-size: 16px
            }
            h2 {
                font-size: 18px
            }
            section.main {
                display: flex
            }
            #menu,section.main {
                flex-direction: column
            }
            #menu {
                display: none;
                flex-wrap: nowrap;
                min-width: 340px;
                background: #363636;
                padding: 8px;
                border-radius: 4px;
                margin-top: -10px;
                margin-right: 10px;
            }
            #content {
                display: flex;
                flex-wrap: wrap;
                align-items: stretch
            }
            figure {
                padding: 0px;
                margin: 0;
                -webkit-margin-before: 0;
                margin-block-start: 0;
                -webkit-margin-after: 0;
                margin-block-end: 0;
                -webkit-margin-start: 0;
                margin-inline-start: 0;
                -webkit-margin-end: 0;
                margin-inline-end: 0
            }
            figure img {
                display: block;
                width: 100%;
                height: auto;
                border-radius: 4px;
                margin-top: 8px;
            }
            @media (min-width: 800px) and (orientation:landscape) {
                #content {
                    display:flex;
                    flex-wrap: nowrap;
                    align-items: stretch
                }
                figure img {
                    display: block;
                    max-width: 100%;
                    max-height: calc(100vh - 40px);
                    width: auto;
                    height: auto
                }
                figure {
                    padding: 0 0 0 0px;
                    margin: 0;
                    -webkit-margin-before: 0;
                    margin-block-start: 0;
                    -webkit-margin-after: 0;
                    margin-block-end: 0;
                    -webkit-margin-start: 0;
                    margin-inline-start: 0;
                    -webkit-margin-end: 0;
                    margin-inline-end: 0
                }
            }
            section#buttons {
                display: flex;
                flex-wrap: nowrap;
                justify-content: space-between
            }
            #nav-toggle {
                cursor: pointer;
                display: block
            }
            #nav-toggle-cb {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }
            #nav-toggle-cb:checked+#menu {
                display: flex
            }
            .input-group {
                display: flex;
                flex-wrap: nowrap;
                line-height: 22px;
                margin: 5px 0
            }
            .input-group>label {
                display: inline-block;
                padding-right: 10px;
                min-width: 47%
            }
            .input-group input,.input-group select {
                flex-grow: 1
            }
            .range-max,.range-min {
                display: inline-block;
                padding: 0 5px
            }
            button {
                display: block;
                margin: 5px;
                padding: 0 12px;
                border: 0;
                line-height: 28px;
                cursor: pointer;
                color: #fff;
                background: #ff3034;
                border-radius: 5px;
                font-size: 16px;
                outline: 0
            }
            button:hover {
                background: #ff494d
            }
            button:active {
                background: #f21c21
            }
            button.disabled {
                cursor: default;
                background: #a0a0a0
            }
            input[type=range] {
                -webkit-appearance: none;
                width: 100%;
                height: 22px;
                background: #363636;
                cursor: pointer;
                margin: 0
            }
            input[type=range]:focus {
                outline: 0
            }
            input[type=range]::-webkit-slider-runnable-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }
            input[type=range]::-webkit-slider-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                -webkit-appearance: none;
                margin-top: -11.5px
            }
            input[type=range]:focus::-webkit-slider-runnable-track {
                background: #EFEFEF
            }
            input[type=range]::-moz-range-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }
            input[type=range]::-moz-range-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer
            }
            input[type=range]::-ms-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: 0 0;
                border-color: transparent;
                color: transparent
            }
            input[type=range]::-ms-fill-lower {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }
            input[type=range]::-ms-fill-upper {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }
            input[type=range]::-ms-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                height: 2px
            }
            input[type=range]:focus::-ms-fill-lower {
                background: #EFEFEF
            }
            input[type=range]:focus::-ms-fill-upper {
                background: #363636
            }
            .switch {
                display: block;
                position: relative;
                line-height: 22px;
                font-size: 16px;
                height: 22px
            }
            .switch input {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }
            .slider {
                width: 50px;
                height: 22px;
                border-radius: 22px;
                cursor: pointer;
                background-color: grey
            }
            .slider,.slider:before {
                display: inline-block;
                transition: .4s
            }
            .slider:before {
                position: relative;
                content: "";
                border-radius: 50%;
                height: 16px;
                width: 16px;
                left: 4px;
                top: 3px;
                background-color: #fff
            }
            input:checked+.slider {
                background-color: #ff3034
            }
            input:checked+.slider:before {
                -webkit-transform: translateX(26px);
                transform: translateX(26px)
            }
            select {
                border: 1px solid #363636;
                font-size: 14px;
                height: 22px;
                outline: 0;
                border-radius: 5px
            }
            .image-container {
                position: relative;
                min-width: 160px
            }
            .close {
                position: absolute;
                right: 5px;
                top: 5px;
                background: #ff3034;
                width: 16px;
                height: 16px;
                border-radius: 100px;
                color: #fff;
                text-align: center;
                line-height: 18px;
                cursor: pointer
            }
            .hidden {
                display: none
            }
        </style>
        <script src='https:\/\/raw.githubusercontent.com/VoHoangDinhKha/DinhKha.github.io/main/Face-api/face-api.min.js'></script> 
    </head>
    <body>
    ESP32-CAM IP：<input type="text" id="ip" size="20" value="192.168.">&nbsp;&nbsp;<input type="button" value="Set" onclick="start();">    
        <figure>
            <div id="stream-container" class="image-container hidden">
              <div class="close" id="close-stream">×</div>
              <img id="stream" src="" style="display:none" >
              <canvas id="canvas" width="0" height="0"></canvas>
            </div>
        </figure>     
        <section class="main">
            <div id="logo">
                <label for="nav-toggle-cb" id="nav-toggle">&#9776;&nbsp;&nbsp;Toggle OV2640 settings</label>
            </div>
            <div id="content">
                <div id="sidebar">
                    <input type="checkbox" id="nav-toggle-cb" checked="checked">
                    <nav id="menu">
                        <section id="buttons">
                            <button id="restart">Restart board</button>
                            <button id="stop-still">Stop</button>
                            <button id="get-still">Get Still</button>
                            <button id="toggle-stream" style="display:none">Start Stream</button>                                                     
                        </section>
                        <div class="input-group" id="uart-group">
                            <label for="uart">Recognize face</label>
                            <div class="switch">
                                <input id="uart" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="uart"></label>
                            </div>
                        </div>
                        <div class="input-group" id="distancelimit-group">
                            <label for="distancelimit">Distance Limit</label>
                            <div class="range-min">0</div>
                            <input type="range" id="distancelimit" min="0" max="1" value="0.4" step="0.1" class="default-action">
                            <div class="range-max">1</div>
                        </div>                                                          
                        <div class="input-group" id="flash-group">
                            <label for="flash">Flash</label>
                            <div class="range-min">0</div>
                            <input type="range" id="flash" min="0" max="255" value="0" class="default-action">
                            <div class="range-max">255</div>
                        </div>
                        <div class="input-group" id="framesize-group">
                            <label for="framesize">Resolution</label>
                            <select id="framesize" class="default-action">
                                <option value="10">UXGA(1600x1200)</option>
                                <option value="9">SXGA(1280x1024)</option>
                                <option value="8">XGA(1024x768)</option>
                                <option value="7">SVGA(800x600)</option>
                                <option value="6">VGA(640x480)</option>
                                <option value="5">CIF(400x296)</option>
                                <option value="4" selected="selected">QVGA(320x240)</option>
                                <option value="3">HQVGA(240x176)</option>
                                <option value="0">QQVGA(160x120)</option>
                            </select>
                        </div>
                        <div class="input-group" id="quality-group">
                            <label for="quality">Quality</label>
                            <div class="range-min">10</div>
                            <input type="range" id="quality" min="10" max="63" value="10" class="default-action">
                            <div class="range-max">63</div>
                        </div>
                        <div class="input-group" id="brightness-group">
                            <label for="brightness">Brightness</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="brightness" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="contrast-group">
                            <label for="contrast">Contrast</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="contrast" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="saturation-group">
                            <label for="saturation">Saturation</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="saturation" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="special_effect-group">
                            <label for="special_effect">Special Effect</label>
                            <select id="special_effect" class="default-action">
                                <option value="0" selected="selected">No Effect</option>
                                <option value="1">Negative</option>
                                <option value="2">Grayscale</option>
                                <option value="3">Red Tint</option>
                                <option value="4">Green Tint</option>
                                <option value="5">Blue Tint</option>
                                <option value="6">Sepia</option>
                            </select>
                        </div>
                        <div class="input-group" id="hmirror-group">
                            <label for="hmirror">H-Mirror</label>
                            <div class="switch">
                                <input id="hmirror" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="hmirror"></label>
                            </div>
                        </div>
                        <div class="input-group" id="vflip-group">
                            <label for="vflip">V-Flip</label>
                            <div class="switch">
                                <input id="vflip" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="vflip"></label>
                            </div>
                        </div>
                        <div class="input-group" id="servo-group">
                            <label for="servo">Servo</label>
                            <div class="range-min">0</div>
                            <input type="range" id="servo" min="0" max="180" value="90" class="default-action">
                            <div class="range-max">180</div>
                            <select id="pinServo" width="30"><option value="14" selected>IO2</option><option value="12">IO12</option><option value="13">IO13</option><option value="2">IO14</option><option value="15">IO15</option></select>
                        </div>
                        <div class="input-group" id="relay-group">
                            <label for="relay">Relay</label>
                            <div class="switch">
                                <input id="relay" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="relay"></label>
                            </div>
                            <select id="pinRelay" width="30"><option value="2">IO2</option><option value="12">IO12</option><option value="13" selected>IO13</option><option value="14">IO14</option><option value="15">IO15</option></select>
                        </div>                         
                    </nav>
                </div>
            </div>
        </section>
        Result：<input type="checkbox" id="chkResult" checked>
        <div id="message" style="color:red">Please wait for loading model.<div>
                
        <script>
        //Nhận dạng hình ảnh Chủ nhà
        const aiView = document.getElementById('stream')
        const aiStill = document.getElementById('get-still')
        const canvas = document.getElementById('canvas')     
        var context = canvas.getContext("2d");  
        const message = document.getElementById('message');
        const uart = document.getElementById('uart');
        const chkResult = document.getElementById('chkResult');
        const distancelimit = document.getElementById('distancelimit')
        var res = "";

        const faceImagesPath = 'https://raw.githubusercontent.com/VoHoangDinhKha/DinhKha.github.io/main/Face-api/facelist/';   //Đường dẫn thư mục ảnh mẫu được đặt theo tên người
        const faceLabels = ['Kha', 'Hung'];    //Danh sách các thư mục được đặt theo tên của mọi người
        faceImagesCount = 2 ;                 //Số lượng ảnh trong thư mục được đặt tên theo từng người và tệp JPG được đặt tên theo số sê-ri 1.jpg, 2.jpg...
        
        const modelPath = 'https://fustyles.github.io/webduino/TensorFlow/Face-api/';   //Đường dẫn tệp mô hình
        let displaySize = { width:320, height: 240 }
        let labeledFaceDescriptors;
        let faceMatcher; 

        //Tải mô hình
        Promise.all([
          faceapi.nets.faceLandmark68Net.load(modelPath),
          faceapi.nets.faceRecognitionNet.load(modelPath),
          faceapi.nets.ssdMobilenetv1.load(modelPath)      
        ]).then(function(){
          message.innerHTML = "";
          aiStill.click();  //Nhận hình ảnh video
        })   
        
        async function DetectImage() {  //Thực hiện nhận dạng khuôn mặt
          canvas.setAttribute("width", aiView.width);
          canvas.setAttribute("height", aiView.height); 
          context.drawImage(aiView,0,0,canvas.width,canvas.height);
          if (!chkResult.checked) message.innerHTML = "";
                      
          if (!labeledFaceDescriptors) {
            message.innerHTML = "Loading face images...";      
            labeledFaceDescriptors = await loadLabeledImages();  
            message.innerHTML = "";
          }
                      
          if (uart.checked) {
            let displaySize = { width:canvas.width, height: canvas.height }     
            faceMatcher = new faceapi.FaceMatcher(labeledFaceDescriptors, Number(distancelimit.value))  //Giới hạn trên của khoảng cách. Nếu vượt quá giá trị này, không xác định sẽ được hiển thị. Nếu không, tên của người đó sẽ được hiển thị.
            const detections = await faceapi.detectAllFaces(canvas).withFaceLandmarks().withFaceDescriptors();
            const resizedDetections = faceapi.resizeResults(detections, displaySize);
            const results = resizedDetections.map(d => faceMatcher.findBestMatch(d.descriptor));
            if (chkResult.checked) message.innerHTML = JSON.stringify(results);
            
            res = "";
            results.forEach((result, i) => {
                if (uart.checked) {
                  //Khi nhận diện một khuôn mặt
                  var query = document.location.origin+'?uart='+result.label;
                  fetch(query)
                    .then(response => {
                      console.log(`request to ${query} finished, status: ${response.status}`)
                    })
                }
                
                res+= i+","+result.label+","+result.distance+"<br>";      
                const box = resizedDetections[i].detection.box
                var drawBox = new faceapi.draw.DrawBox(box, { label: result.toString()})
                drawBox.draw(canvas);
              })  
              uart.checked = false;  //Vì mỗi lần nhận dạng tiêu tốn một chút bộ nhớ nên quá trình nhận dạng sẽ bị tạm dừng sau khi quá trình nhận dạng hoàn tất.
              if (chkResult.checked) message.innerHTML = res;            
          }
          aiStill.click();
        }

        function loadLabeledImages() {  //Đọc ảnh mẫu
          return Promise.all(
            faceLabels.map(async label => {
              const descriptions = []
              for (let i=1;i<=faceImagesCount;i++) {
                const img = await faceapi.fetchImage(faceImagesPath+label+'/'+i+'.jpg')
                const detections = await faceapi.detectSingleFace(img).withFaceLandmarks().withFaceDescriptor();
                descriptions.push(detections.descriptor)
              }
              return new faceapi.LabeledFaceDescriptors(label, descriptions)
            })
          )
        }        
        
        aiView.onload = function (event) {
          try { 
            document.createEvent("TouchEvent");
            setTimeout(function(){DetectImage();},250);
          } catch(e) { 
            setTimeout(function(){DetectImage();},150);
          } 
        }
        
        //Chức năng chính thức
        function start() {
          var baseHost = 'http://'+document.getElementById("ip").value;  //var baseHost = document.location.origin
          const hide = el => {
            el.classList.add('hidden')
          }
          const show = el => {
            el.classList.remove('hidden')
          }
          const disable = el => {
            el.classList.add('disabled')
            el.disabled = true
          }
          const enable = el => {
            el.classList.remove('disabled')
            el.disabled = false
          }
          const updateValue = (el, value, updateRemote) => {
            updateRemote = updateRemote == null ? true : updateRemote
            let initialValue
            if(!el) return;
            if (el.type === 'checkbox') {
              initialValue = el.checked
              value = !!value
              el.checked = value
            } else {
              initialValue = el.value
              el.value = value
            }
        
            if (updateRemote && initialValue !== value) {
              updateConfig(el);
            } 
          }
        
          function updateConfig (el) {
            let value
            switch (el.type) {
              case 'checkbox':
                value = el.checked ? 1 : 0
                break
              case 'range':
              case 'select-one':
                value = el.value
                break
              case 'button':
              case 'submit':
                value = '1'
                break
              default:
                return
            }
        
            if (el.id =="flash") {  //Đã thêm lệnh tùy chỉnh flash
              var query = baseHost+"?flash=" + String(value);
            } else if (el.id =="servo") {  //Đã thêm lệnh tùy chỉnh servo
              var query = baseHost+"?servo=" + pinServo.value + ";" + String(value);
            } else if (el.id =="relay") {  //Đã thêm hướng dẫn chuyển tiếp tùy chỉnh
              var query = baseHost+"?relay=" + pinRelay.value + ";" + Number(relay.checked);
            } else if (el.id =="uart") {  //Đã thêm lệnh tùy chỉnh uart
              return;     
            } else if (el.id =="distancelimit") {  //Đã thêm lệnh tùy chỉnh khoảng cách giới hạn
              return;                           
            } else {
              var query = `${baseHost}/?${el.id}=${value}`
            }
        
            fetch(query)
              .then(response => {
                console.log(`request to ${query} finished, status: ${response.status}`)
              })
          }
        
          document
            .querySelectorAll('.close')
            .forEach(el => {
              el.onclick = () => {
                hide(el.parentNode)
              }
            })
        
          const view = document.getElementById('stream')
          const viewContainer = document.getElementById('stream-container')
          const stillButton = document.getElementById('get-still')
          const enrollButton = document.getElementById('face_enroll')
          const closeButton = document.getElementById('close-stream')
          const stopButton = document.getElementById('stop-still')            
          const restartButton = document.getElementById('restart')           
          const flash = document.getElementById('flash')                     
          const servo = document.getElementById('servo')                    
          const pinServo = document.getElementById('pinServo');              
          const relay = document.getElementById('relay')                      
          const pinRelay = document.getElementById('pinRelay');                       
          const uart = document.getElementById('uart')                      
          var myTimer;
          var restartCount=0;    
          var streamState = false;
          
          stopButton.onclick = function (event) {   
            window.stop();
            message.innerHTML = "";
          }    
           
          // Đính kèm hành động vào các nút
          stillButton.onclick = () => {
            view.src = `${baseHost}/?getstill=${Date.now()}`
            show(viewContainer);     
          }
          
          closeButton.onclick = () => {
            hide(viewContainer)
          }
          
          //Đã thêm sự kiện nhấn nút nguồn khởi động lại 
          restartButton.onclick = () => {
            fetch(baseHost+"/?restart");
          }    
                
          //Đính kèm mặc định về hành động thay đổi
          document
            .querySelectorAll('.default-action')
            .forEach(el => {
              el.onchange = () => updateConfig(el)
            })
        
          framesize.onchange = () => {
            updateConfig(framesize)
          }
          
          //đọc giá trị ban đầu 
          fetch(`${baseHost}/?status`)
          .then(function (response) {
            return response.json()
          })
          .then(function (state) {
            document
            .querySelectorAll('.default-action')
            .forEach(el => {
              if (el.id=="flash") {  //Đã thêm giá trị mặc định của cài đặt flash = 0
                flash.value=0;
                var query = baseHost+"?flash=0";
                fetch(query)
                  .then(response => {
                    console.log(`request to ${query} finished, status: ${response.status}`)
                  })
              } else if (el.id=="servo") {  //Đã thêm giá trị mặc định của cài đặt servo = 90 độ
                servo.value=90;
                /*
                var query = baseHost+"?servo=" + pinServo.value + ";90";
                fetch(query)
                  .then(response => {
                    console.log(`request to ${query} finished, status: ${response.status}`)
                  })
                */
              } else if (el.id=="relay") {  //Đã thêm giá trị mặc định cài đặt Relay = 0
                relay.checked = false;
                /*
                var query = baseHost+"?relay=" + pinRelay.value + ";0";
                fetch(query)
                  .then(response => {
                    console.log(`request to ${query} finished, status: ${response.status}`)
                  })
                */
              } else if (el.id=="uart") {  //Đã thêm giá trị mặc định của cài đặt uart = 0
                uart.checked = false;
              } else if (el.id=="distancelimit") {  //Đã thêm giá trị mặc định của cài đặt giới hạn khoảng cách = 0,4
                distancelimit.value = 0.4;                                  
              } else {    
                updateValue(el, state[el.id], false)
              }
            })
          })
        }
        
        var href=location.href;
        if (href.indexOf("?")!=-1) {
          ip.value = location.search.split("?")[1].replace(/http:\/\//g,"");
          start();
        }
        else if (href.indexOf("http")!=-1) {
          ip.value = location.host;
          start();
        }
          
    </script>        
    </body>
</html>
)rawliteral";

//Đặt giá trị ban đầu của menu để truy xuất định dạng json
void status(){
  sensor_t * s = esp_camera_sensor_get();
  String json = "{";
  json += "\"framesize\":"+String(s->status.framesize)+",";
  json += "\"quality\":"+String(s->status.quality)+",";
  json += "\"brightness\":"+String(s->status.brightness)+",";
  json += "\"contrast\":"+String(s->status.contrast)+",";
  json += "\"saturation\":"+String(s->status.saturation)+",";
  json += "\"special_effect\":"+String(s->status.special_effect)+",";
  json += "\"vflip\":"+String(s->status.vflip)+",";
  json += "\"hmirror\":"+String(s->status.hmirror);
  json += "}";
  
  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
  client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
  client.println("Content-Type: application/json; charset=utf-8");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  
  for (int Index = 0; Index < json.length(); Index = Index+1024) {
    client.print(json.substring(Index, Index+1024));
  }
}

//Trả về trang chủ HTML hoặc nội dung biến Phản hồi
void mainpage() {
  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
  client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  
  String Data="";
  if (cmd!="")
    Data = Feedback;
  else
    Data = String((const char *)INDEX_HTML);
  
  for (int Index = 0; Index < Data.length(); Index = Index+1024) {
    client.print(Data.substring(Index, Index+1024));
  } 
}

void getStill() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");              
  client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
  client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
  client.println("Content-Type: image/jpeg");
  client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\""); 
  client.println("Content-Length: " + String(fb->len));             
  client.println("Connection: close");
  client.println();
  
  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n=0;n<fbLen;n=n+1024) {
    if (n+1024<fbLen) {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    }
    else if (fbLen%1024>0) {
      size_t remainder = fbLen%1024;
      client.write(fbBuf, remainder);
    }
  }  
  esp_camera_fb_return(fb);

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);              
}

//Nhận tin tức mới nhất từ ​​Telegram
void getTelegramMessage(String token) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = ""; 
  JsonObject obj;
  DynamicJsonDocument doc(1024);
  String result;
  long update_id;
  String message;
  long message_id;
  String text;  

  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   
  Serial.println("Connect to " + String(myDomain));
  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    timer = 0;
    if (client_tcp.connected()) { 
      while (timer<timerLimit) {  
        getAll = "";
        getBody = "";
  
        String request = "limit=1&offset=-1&allowed_updates=message";
        client_tcp.println("POST /bot"+token+"/getUpdates HTTP/1.1");
        client_tcp.println("Host: " + String(myDomain));
        client_tcp.println("Content-Length: " + String(request.length()));
        client_tcp.println("Content-Type: application/x-www-form-urlencoded");
        client_tcp.println("Connection: keep-alive");
        client_tcp.println();
        client_tcp.print(request);
        
        int waitTime = 5000;   // timeout 5 giây
        long startTime = millis();
        boolean state = false;
        
        while ((startTime + waitTime) > millis()) {
          delay(100);      
          while (client_tcp.available()) {
              char c = client_tcp.read();
              if (c == '\n') {
                if (getAll.length()==0) state=true; 
                getAll = "";
              } else if (c != '\r')
                getAll += String(c);
              if (state==true) getBody += String(c);
              startTime = millis();
           }
           if (getBody.length()>0) break;
        }
  
        deserializeJson(doc, getBody);
        obj = doc.as<JsonObject>();
        message_id = obj["result"][0]["message"]["message_id"].as<String>().toInt();
        text = obj["result"][0]["message"]["text"].as<String>();
  
        if (message_id!=message_id_last&&message_id) {
          int id_last = message_id_last;
          message_id_last = message_id;
          if (id_last==0) {
            message_id = 0;
          } else {
            Serial.println(getBody);
            Serial.println();
          }
          
          if (text!="") {
            Serial.println("["+String(message_id)+"] "+text);
            telegramCommand(text);  //Thực hiện hướng dẫn
          }
        }
        delay(1000);
        timer++;
      }
    }
  }
}

//Gửi hình ảnh tới bot Telegram
String sendCapturedImage2Telegram(String token, String chat_id) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   
  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful"); 
    String head = "--Taiwan\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--Taiwan\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Taiwan--\r\n";
    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
    client_tcp.println("POST /bot"+token+"/sendPhoto HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Taiwan");
    client_tcp.println();
    client_tcp.print(head);
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client_tcp.write(fbBuf, remainder);
      }
    }  
    
    client_tcp.print(tail);
    esp_camera_fb_return(fb);
    int waitTime = 10000;   // timeout 10 seconds
    long startTime = millis();
    boolean state = false;
    
    while ((startTime + waitTime) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) {
          char c = client_tcp.read();
          if (state==true) getBody += String(c);        
          if (c == '\n') {
            if (getAll.length()==0) state=true; 
            getAll = "";
          } 
          else if (c != '\r')
            getAll += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    client_tcp.stop();
    Serial.println();
    Serial.println(getBody);
  } else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);   
  
  return getBody;
}

//Gửi tin nhắn văn bản Telegram và các nút lệnh
String sendMessage2Telegram(String token, String chat_id, String text, String keyboard) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";
  String request = "parse_mode=HTML&chat_id="+chat_id+"&text="+text;
  if (keyboard!="") request += "&reply_markup="+keyboard;
  
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   
  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    client_tcp.println("POST /bot"+token+"/sendMessage HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(request.length()));
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Connection: keep-alive");
    client_tcp.println();
    client_tcp.print(request);
    
    int waitTime = 5000;   // timeout 5 seconds
    long startTime = millis();
    boolean state = false;
    
    while ((startTime + waitTime) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) {
          char c = client_tcp.read();
          if (state==true) getBody += String(c);      
          if (c == '\n') {
            if (getAll.length()==0) state=true; 
            getAll = "";
          } else if (c != '\r')
            getAll += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    client_tcp.stop();
    Serial.println();
    Serial.println(getBody);
  } else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  return getBody;     
}

//Thực hiện lệnh nhắn tin Telegram
void telegramCommand(String text) {
  if (!text||text=="") return;
  timer = 0;  
  if (text=="/still") {         //Nhận ảnh chụp màn hình video
    sendCapturedImage2Telegram(myToken, myChatId);
  } else if (text=="/ledon") {  //Bật đèn nháy
    ledcDetachPin(4);
    pinMode(4 , OUTPUT);
    digitalWrite(4, HIGH);
    sendMessage2Telegram(myToken, myChatId, "Turn on the flash", "");
  } else if (text=="/ledoff") {  //Tắt đèn nháy
    ledcDetachPin(4);
    pinMode(4 , OUTPUT);
    digitalWrite(4, LOW);
    sendMessage2Telegram(myToken, myChatId, "Turn off the flash", "");
  } else if (text=="/open") {  //mở khóa cửa
    pinMode(pinDoor , OUTPUT);
    digitalWrite(pinDoor, HIGH);
    delay(2000);
    digitalWrite(pinDoor, LOW);
  }
}
