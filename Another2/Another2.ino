

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include <ESPAsyncWebServer.h>
//#include "esp_http_server.h"
#include <HTTPClient.h>
#include <ESP32_FTPClient.h>
#include "octocat.h"
#include "SPIFFS.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include <ReadLines.h>
#include <ESPmDNS.h>
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
// define the number of bytes you want to access
#define EEPROM_SIZE 2
int pictureNumber = 0;
boolean telegram = false;     //if you do not want to use telegram
boolean web = true;           //if you want to use web
boolean stationactive = false;
 
RTC_DATA_ATTR int bootCount = 0;

String defaultFile = "/config.txt";
String defaultFile2 = "/config2.txt";

#define PART_BOUNDARY "123456789000000000000987654321"

// This project was tested with the AI Thinker Model, M5STACK PSRAM Model and M5STACK WITHOUT PSRAM
#define CAMERA_MODEL_AI_THINKER
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM

// Not tested with this model
//#define CAMERA_MODEL_WROVER_KIT

#include "camera_pins.h"

boolean defaultFileAvailable = false;
boolean apsetup = false;
boolean staticup = false;
boolean staup = false;
boolean loopProceed = true;

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


int gpioPIR = 2;   //PIR Motion Sensor
int gpioLED = 15;

IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


String payload = "";
String payload2 = "";
String payload3 = "";
String espMacID = "";
String espIP = "192.168.8.11";

boolean takeNewPhoto = false;
boolean picNoRec = false;

int counter = 0;
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

unsigned long lastTime2 = 0;
unsigned long timerDelay2 = 3000;

boolean gotPublicIP = false;
boolean streamInProgress = false;
uint8_t lastFileNo=0;

const char video_index[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>IQ-JOY's Eucalyptus</title>
  <meta name="Real-Time Monitoring device coded by Ayoade Adetunji" content="Eucalyptus Server Code">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="css/styles.css?v=1.0">
  <link rel="icon" href="data:,">
<style>

body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }

h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}

h2 {
  color: #d1633c;
  font-size: 1em;
}
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}

.button {
        display: inline-block;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        color: #ffffff;
        background-color: #7aa8b7;
        border-radius: 6px;
        outline: none;
      }

figure {
  border: 1px #cccccc solid;
  padding: 4px;
  margin: auto;
}

figcaption {
  background-color: black;
  color: white;
  font-style: italic;
  padding: 2px;
  text-align: center;
}

</style>
</head>

<body>
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="Black Goose logo"><br>Eucalyptus smart cctv </h1>
    
<p>What do you want to do?</p>
      <a class="button" href="/capture">Sneekpeek</a>
      <a class="button" href="/stopstream">Stop LiveStream</a>
      <a class="button" href="/sensed">Sensed Images</a>
      <a class="button" href="/settings">Settings</a>
      <a class="button" href="/reset">Restart</a>
<p></p>
  
  <figure>
  <img src="stream" alt="Streaming Video" style="width:100%">
  <figcaption>Streaming Live</figcaption>
    </figure>

    <p></p>
    <h2></h2>
<script src="js/scripts.js"></script>
</body>
</html>)rawliteral";



const char stop_video_index[] PROGMEM = R"rawliteral(
<<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>IQ-JOY's Eucalyptus</title>
  <meta name="Real-Time Monitoring device coded by Ayoade Adetunji" content="Eucalyptus Server Code">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
<style>

body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }

h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}

h2 {
  color: #d1633c;
  font-size: 1em;
}
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}

.button {
        display: inline-block;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        color: #ffffff;
        background-color: #7aa8b7;
        border-radius: 6px;
        outline: none;
      }

figure {
  border: 1px #cccccc solid;
  padding: 4px;
  margin: auto;
}

figcaption {
  background-color: black;
  color: white;
  font-style: italic;
  padding: 2px;
  text-align: center;
}

</style>
</head>

<body>
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="IQ-JOY logo"><br>Eucalyptus smart cctv </h1>
    
<p>What do you want to do?</p>
      <a class="button" href="/capture">Sneekpeek</a>
      <a class="button" href="/streamer">Watch Live</a>
      <a class="button" href="/sensed">Sensed Images</a>
      <a class="button" href="/settings">Settings</a>
      <a class="button" href="/reset">Restart</a>
<p></p>
  
  <figure>
  <img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="Streaming complete" style="width:100%">
  <figcaption>Streaming Live</figcaption>
    </figure>

    <p></p>
    <h2></h2>
<script src="js/scripts.js"></script>
</body>
</html>)rawliteral";

const char capture_index[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>IQ-JOY's Eucalyptus</title>
  <meta name="Real-Time Monitoring device coded by Ayoade Adetunji" content="Eucalyptus Server Code">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
<style>
body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }
h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}
h2 {
  color: #d1633c;
  font-size: 1em;
}
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}
.button {
        display: inline-block;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        color: #ffffff;
        background-color: #7aa8b7;
        border-radius: 6px;
        outline: none;
      }
figure {
  border: 1px #cccccc solid;
  padding: 4px;
  margin: auto;
}
figcaption {
  background-color: black;
  color: white;
  font-style: italic;
  padding: 2px;
  text-align: center;
}
</style>
</head>

<body>
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="You are not connected to Internet"><br>Eucalyptus smart cctv </h1>
<p>What do you want to do?</p>   
      <a class="button" href="/capture">Sneekpeek</a>
      <a class="button" href="/streamer">Watch Live</a>
      <a class="button" href="/sensed">Sensed Images</a>
    <a class="button" href="/settings">Settings</a> 
    <a class="button" href="/reset">Restart</a>
<br>
  <figure>
   <img src="lastcaptured" alt="Last captured image" style="width:100%">
  <figcaption>Captured Image</figcaption>
    </figure>
    <br>
    <h2>Current connection settings</h2>
    <br>
    <a> ~PLACEHOLDER_NETWORK_NOTE~</a>
    <br><br>
    <p>Copyright Ayoade Adetunji 2021</p>

</body>
</html>)rawliteral";


String lastCaptured = "";
String lastCap = "";

 
const char motion_index[] PROGMEM = R"rawliteral(
<!doctype html>

<html lang="en">
<head>
  <meta charset="utf-8">
  <title>IQ-JOY's Eucalyptus</title>
  <meta name="Real-Time Monitoring device coded by Ayoade Adetunji" content="Eucalyptus Server Code">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
    
<style>

body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }

h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}

h2 {
  color: #d1633c;
  font-size: 1em;
}

h3 {
  color: #09c1dc;
  font-size: 0.8em;
  border-bottom: 1px solid #09c1dc;
}
    
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}

.button {
        display: inline-block;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        color: #ffffff;
        background-color: #7aa8b7;
        border-radius: 6px;
        outline: none;
      }

figure {
  border: 1px #cccccc solid;
  padding: 4px;
  margin: auto;
}

figcaption {
  background-color: black;
  color: white;
  font-style: italic;
  padding: 2px;
  text-align: center;
}

.flex-container {
    display: flex;
}

.flex-child {
    flex: 1;
}  

.flex-child:first-child {
    margin-right: 20px;
} 

</style>
</head>

<body>
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="Black Goose logo"><br>Eucalyptus smart cctv </h1>
    
<p>What do you want to do?</p>
    
      <a class="button" href="/capture">Sneekpeek</a>
      <a class="button" href="/streamer">Watch Live</a>
    <a class="button" href="/sensed">Sensed Images</a>
    <a class="button" href="/settings">Settings</a>
    <a class="button" href="/reset">Restart</a>
    
<br><br>  
<div class="flex-container">

  <div class="flex-child magenta">
    ~PLACEHOLDER_CAPTURED_IMAGE~
  </div>
  
  <div class="flex-child green">
  
     <figure>
      <img src="https://i-margin.com/wp-content/uploads/2019/09/intru.jpg" alt="captured image" style="width:100%" id="selectedImg">
  <figcaption>Captured Image</figcaption>
    </figure>

  </div>
  
</div>
    
<br><br>

<script>
function changeImage(x) {
    var t = document.getElementById('selectedImg')
    var mySrc = t.getAttribute('');
    var img = "/clicked?photo="+x.id;
    console.log("img:"+img);
    if( mySrc == img ){
        t.setAttribute('src',img);
        t.setAttribute('alt',img);
        } else {
        t.setAttribute('src',img);
        t.setAttribute('alt',img);
        }
 }
</script>

</body>
</html>)rawliteral";


const char settings_index[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<style>

body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }

h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}

h2 {
  color: #d1633c;
  font-size: 1em;
}

h3 {
  color: #09c1dc;
  font-size: 0.8em;
  border-bottom: 1px solid #09c1dc;
}
    
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}
    
* {
  box-sizing: border-box;
}
    
input[type=text], select, textarea {
  width: 100%;
  padding: 12px;
  border: 1px solid #ccc;
  border-radius: 4px;
  resize: vertical;
}

label {
  padding: 12px 12px 12px 0;
  display: inline-block;
}

input[type=submit] {
  background-color: #4CAF50;
  color: white;
  padding: 12px 20px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  float: right;
}

input[type=submit]:hover {
  background-color: #45a049;
}

.container {
  border-radius: 5px;
  background-color: #f2f2f2;
  padding: 20px;
}

.col-25 {
  float: left;
  width: 25%;
  margin-top: 6px;
}

.col-75 {
  float: left;
  width: 75%;
  margin-top: 6px;
}

/* Clear floats after the columns */
.row:after {
  content: "";
  display: table;
  clear: both;
}

/* Responsive layout - when the screen is less than 600px wide, make the two columns stack on top of each other instead of next to each other */
@media screen and (max-width: 600px) {
  .col-25, .col-75, input[type=submit] {
    width: 100%;
    margin-top: 0;
  }
}
</style>
</head>
    
<body onload="radioButtonCheck()"> 
    
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="IQ-JOY logo"><br>Eucalyptus smart cctv </h1>

<h2>Settings</h2>
<p>Change connection parameters. Wrong settings entered here could damage your device and make it non functional. You must be sure before you enter any settings here.</p>

<div class="container">
  <form action="/entersettings" onsubmit="return submitResult();">
      
<h3>Server Credentials</h3>
  <div class="row">
    <div class="col-25">
      <label for="apssid">Access Point SSID</label>
    </div>
    <div class="col-75">
      <input type="text" id="apssid" name="apSSID" placeholder="Enter the local network name of your Smart CCTV" value="" onblur="myFunction(this.id)">
    </div>
  </div>
  <div class="row">
    <div class="col-25">
      <label for="appass">Access Point Password</label>
    </div>
    <div class="col-75">
      <input type="text" id="appass" name="apPASS" placeholder="Enter local network password of your Smart CCTV" value="" onblur="myFunction(this.id)">
    </div>
  </div>

<br>
      
 <h3>Client Credentials</h3>
        <div class="row">
    <div class="col-25">
      <label for="rssid">Router SSID</label>
    </div>
    <div class="col-75">
      <input type="text" id="rssid" name="rSSID" placeholder="Enter your WIFI Router name" value="" 
             onblur="myFunction(this.id)">
    </div>
  </div>
  <div class="row">
    <div class="col-25">
      <label for="rpass">Router Password</label>
    </div>
    <div class="col-75">
      <input type="text" id="rpass" name="rPASS" placeholder="Enter your WIFI Password" value="" onblur="myFunction(this.id)">
    </div>
  </div>

<br>
      
<h3>Port Forwarding</h3>
<p>You would need to enter a fixed/static IP for port forwarding purposes. This will allow you connect to the Camera via internet from any location. You will have to access your router and forward port 80 to this IP Address you are entering. Details on port forwarding is here:
   <a href="www.iq-joy.com/port_forwarding.html">How to Port Forward</a>
</p>
  <div class="row">
    <div class="col-25">
      <label for="statip">Static IP</label>
    </div>
    <div class="col-75">
      <input type="text" id="statip" name="statIP" placeholder="Enter Camera static IP Address" value="" 
             onblur="myFunction(this.id)">
    </div>
  </div>
      
  <div class="row">
    <div class="col-25">
      <label for="gatew">GateWay</label>
    </div>
    <div class="col-75">
      <input type="text" id="gatew" name="gateW" placeholder="Enter WIFI Gateway" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
  <div class="row">
    <div class="col-25">
      <label for="subnet">Subnet</label>
    </div>
    <div class="col-75">
      <input type="text" id="subnet" name="subNET" placeholder="Enter Subnet" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
<br>
      
<h3>File Transfer Protocol</h3>
<p>Do not change the FTP Server parameters unless you receive directives from us in respect to it. Any changes you make to these parameters will affect the operations of your Camera
</p>
  <div class="row">
    <div class="col-25">
      <label for="ftpserv">FTP Server</label>
    </div>
    <div class="col-75">
      <input type="text" id="ftpserv" name="ftpSERV" placeholder="Enter FTP Address" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
  <div class="row">
    <div class="col-25">
      <label for="ftpuser">FTP Username</label>
    </div>
    <div class="col-75">
      <input type="text" id="ftpuser" name="ftpUSER" placeholder="Enter FTP Username" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
  <div class="row">
    <div class="col-25">
      <label for="ftppass">FTP Password</label>
    </div>
    <div class="col-75">
      <input type="text" id="ftppass" name="ftpPASS" placeholder="Enter Subnet" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
<br>
      
<h3>HTTP Server</h3>
<p>Do not change the HTTP Server parameters unless you receive directives from us in respect to it. Any changes you make to these parameters will affect the operations of your Camera
</p>
  <div class="row">
    <div class="col-25">
      <label for="httpserv">HTTP Server</label>
    </div>
    <div class="col-75">
      <input type="text" id="httpserv" name="httpSERV" placeholder="Enter HTTP Address" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
  <div class="row">
    <div class="col-25">
      <label for="publicip">Public IP</label>
    </div>
    <div class="col-75">
      <input type="text" id="publicip" name="publicIP" placeholder="Enter Public IP Address" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
<br>

    
<h3>Activity Alerts</h3>
<p>Select mode of alert for motion detection around your cam.</p>
      
<label class="container">Web and Phone App
  <input type="radio" name="alert" id ="webphone" value="webradio" checked="checked" onclick = "webPhoneCheck()">
  <span class="checkmark"></span>
</label>
<label class="container">Telegram
  <input type="radio" name="alert" value= "telegramradio" id="telegram" onclick = "telegramCheck()">
  <span class="checkmark"></span>
</label>
      
      
  <div class="row">
    <div class="col-25">
      <label for="teltoken" id="toke">Telegram Token</label>
    </div>
    <div class="col-75">
      <input type="text" id="teltoken" name="telTOKEN" placeholder="Enter Telegram Token" value="" onblur="myFunction(this.id)">
    </div>
  </div>
      
      
  <div class="row">
    <div class="col-25">
      <label for="telid" id="tele">Telegram ID</label> 
    </div>
    <div class="col-75">
      <input type="text" id="telid" name="telID" placeholder="Enter Telegram ID" value="" onblur="myFunction(this.id)">
    </div>
  </div>
<br>

<h3>Mode of Alert</h3>
<p>It is recommended to use Pictures as Video recording upload is slow and may fail due to video size. You should rather livestream video</p>
<label class="container">Pictures
  <input type="radio" checked="checked" name="capture" value="imagecapture">
  <span class="checkmark"></span>
</label>
<label class="container">Video
  <input type="radio" name="capture" value="videocapture">
  <span class="checkmark"></span>
</label>
     
      <br><br>
<div class="row">
<input type="submit" value="ENTER SETTINGS" id = "submitButton" >
 <a class="button" href="/reset">Restart</a>
</div>
</form>
    
</div>
    
    
<script>
function submitResult(){
    var apSid = document.getElementById("apssid");
    var parameters = 0;
    if(apSid.value.length<6){
        apSid.value = "X"
        apSid.style.backgroundColor = "orange"
        parameters--;
    }else{
        parameters++;
    }
    
    
    var apPas = document.getElementById("appass");
    if(apPas.value.length<6){apPas.value = "X";
    apPas.style.backgroundColor = "orange";
    parameters--;
    }else{
        parameters++;
    }
    
    var rSid = document.getElementById("rssid");
    if(rSid.value.length<6){
    rSid.value = "X"
    rSid.style.backgroundColor = "orange"     
    parameters--;
    }else{
        parameters++;
    }
    
    var rPas = document.getElementById("rpass")
    if(rPas.value.length<6){
    rPas.value = "X"
    rPas.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var staIp = document.getElementById("statip");
    if(staIp.value.length<6){
    staIp.value = "X"
    staIp.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var gatW = document.getElementById("gatew");
    if(gatW.value.length<6){
    gatW.value = "X"
    gatW.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var subN = document.getElementById("subnet");
    if(subN.value.length<6){
    subN.value = "X"
    subN.style.backgroundColor = "orange"   
    parameters--;
    }else{
        //change to ESP IP format
        parameters++;
    }
    
    var ftpS = document.getElementById("ftpserv");
    if(ftpS.value.length<8){
    ftpS.value = "X"
    ftpS.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var ftpU = document.getElementById("ftpuser");
    if(ftpU.value.length<6){
    ftpU.value = "X"
    ftpU.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var ftpP = document.getElementById("ftppass");
    if(ftpP.value.length<6){
    ftpP.value = "X"
    ftpP.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    var httpS = document.getElementById("httpserv");
    if(httpS.value.length<10){
      httpS.value = "X"
      httpS.style.backgroundColor = "orange"   
      parameters--;
    }else{
        parameters++;
    }
    
    var publK = document.getElementById("publicip");
    if(publK.value.length<10){
    publK.value = "X"
    publK.style.backgroundColor = "orange"   
    parameters--;
    }else{
        parameters++;
    }
    
    console.log('parameter=' + parameters);
    
    if(parameters != 12){
        if ( confirm("You did not enter some important information in the fields highlighted in orange. If you do not enter these info, the default values will be used. If these default values are not compatible with your new info, it would affect the behavior of your device. Do you want to skip them?")== true ) {
            if ( confirm("Are you sure you wish to submit these settings?") == false ) {
              return false ;
           } else {
              return true ;
           }
        }else return false ;
     
    }
}
</script>

<script>
function myFunction(id) {
  var x = document.getElementById(id);
    //console.log('id=' + id);
   switch (id) {
  case "apssid":
    if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    break;
  case "appass":
    if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    break;
  case "rssid":
     if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"
    break;
  case "rpass":
     if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    break;
  case "statip":
           
     if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
        x.style.backgroundColor = "orange";
         alert("Enter a valid static ip address. Something like this: 123.456.789.101")
    }
    break;
  case "gatew":
     if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
         x.style.backgroundColor = "orange";
         alert("Enter a valid gateway address. Something like this: 123.456.789.1")
    }
    break;
  case "subnet":
    if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
         x.style.backgroundColor = "orange";
         alert("Enter a valid subnet address. Something like this: 255.255.255.0")
    }
    break;
  case "ftpserv":
    if(x.value.length<8){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
         x.style.backgroundColor = "orange";
         alert("Enter a valid FTP address")
    }
    break;
  case "ftpuser":
     if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    break;
  case "ftppass":
    if(x.value.length<6){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    break;
  case "httpserv":
     if(x.value.length<10){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
        x.style.backgroundColor = "orange";
         alert("Enter a valid Server address")
    }
    break;
  case "publicip":
    if(x.value.length<8){
        x.style.backgroundColor = "orange";
    }else x.style.backgroundColor = "#4CAF50"

    if(!x.value.includes(".")){
        x.style.backgroundColor = "orange";
         alert("Enter a valid Public IP address")
    }
    break;
  case "teltoken":
    if(x.value.length<10){
        alert("Telegram tokens cannot not be less than 10 characters")
    }
    break;
  case "telid":
    if(x.value.length<6){
        alert("Telegram IDs cannot not be less than 6 characters")
    }
    break;
       default:
           break;
}
}
</script>

<script>
    function radioButtonCheck(){
            
    if (document.getElementById("webphone").checked = true){
        document.getElementById("teltoken").style.display = "none";
        document.getElementById("telid").style.display = "none";
        document.getElementById("tele").style.display = "none";
        document.getElementById("toke").style.display = "none";
    }else{
        document.getElementById("teltoken").style.display = "block"
        document.getElementById("telid").style.display = "block";
        document.getElementById("tele").style.display = "block"
        document.getElementById("toke").style.display = "block";
    }
}
    </script>
    
<script>
    function webPhoneCheck(){
            
    if (document.getElementById("webphone").checked = true){
        document.getElementById("teltoken").style.display = "none";
        document.getElementById("telid").style.display = "none";
        document.getElementById("tele").style.display = "none";
        document.getElementById("toke").style.display = "none";
    }else{
        document.getElementById("teltoken").style.display = "block"
        document.getElementById("telid").style.display = "block";
        document.getElementById("tele").style.display = "block"
        document.getElementById("toke").style.display = "block";
    }
}
    </script>
    
<script>
    function telegramCheck(){
            
    if (document.getElementById("telegram").checked = true){
        document.getElementById("teltoken").style.display = "block";
        document.getElementById("telid").style.display = "block";
        document.getElementById("tele").style.display = "block";
        document.getElementById("toke").style.display = "block";
    }else{
        document.getElementById("teltoken").style.display = "none"
        document.getElementById("telid").style.display = "none";
        document.getElementById("tele").style.display = "none"
        document.getElementById("toke").style.display = "none";
    }
}
    </script>

<script>
function escapeRegExp(string){
    return string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
    
function replaceAll(str, term, replacement) {
    return str.replace(new RegExp(escapeRegExp(term), 'g'), replacement);
}
</script>

</body>
</html>
)rawliteral";


const char settings_entered_index[] PROGMEM = R"rawliteral(
<!doctype html>

<html lang="en">
<head>
  <meta charset="utf-8">
  <title>IQ-JOY's Eucalyptus</title>
  <meta name="Real-Time Monitoring device coded by Ayoade Adetunji" content="Eucalyptus Server Code">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="css/styles.css?v=1.0">
  <link rel="icon" href="data:,">
    
<style>

body {
  background-color: #faf2e4;
  margin: 0 15%;
  font-family: sans-serif;
  }

h1 {
  text-align: center;
  font-family: serif;
  font-weight: normal;
  text-transform: uppercase;
  border-bottom: 1px solid #57b1dc;
  margin-top: 30px;
}

h2 {
  color: #d1633c;
  font-size: 1em;
}

h3 {
  color: #09c1dc;
  font-size: 0.8em;
  border-bottom: 1px solid #09c1dc;
}
    
.center {
  display: block;
  margin: auto;
  width: 50%;
  height: 50%;
}

.button {
        display: inline-block;
        padding: 10px 20px;
        text-align: center;
        text-decoration: none;
        color: #ffffff;
        background-color: #7aa8b7;
        border-radius: 6px;
        outline: none;
      }

figure {
  border: 1px #cccccc solid;
  padding: 4px;
  margin: auto;
}

figcaption {
  background-color: black;
  color: white;
  font-style: italic;
  padding: 2px;
  text-align: center;
}

.flex-container {
    display: flex;
}

.flex-child {
    flex: 1;
}  

.flex-child:first-child {
    margin-right: 20px;
} 

</style>
</head>

<body>
<h1><img src="https://iq-joy.com/wp/wp-content/uploads/2020/12/cropped-IQJOY_logo-1.png" alt="Black Goose logo"><br>Eucalyptus smart cctv </h1>
    
<p>What do you want to do?</p>
    
      <a class="button" href="/capture">Sneekpeek</a>
      <a class="button" href="/stream">Watch Live</a>
    <a class="button" href="/sensed">Sensed Images</a>
    <a class="button" href="/stream">Settings</a>
     <a class="button" href="/reset">Restart</a>
    
<br><br>  
<h2> SETTINGS ENTERED!</h2>
    <h3> PLEASE RESET OR REBOOT YOUR DEVICE</h3>
<br><br>

<script src="js/scripts.js"></script>
</body>
</html>)rawliteral";




void startCameraServer();

String ftp_server;
String ftp_user;
String ftp_pass;

const char* APBroadcastSSID;
const char* APBroadcastPW;
String apSSID="k";
String apPASS="k";
String defaultSSID = "t";
String defaultPW = "t";

String statIP;
String gatew;
String subt;

String serverName;
String networPublikIP;
String telegramToken;
String telegramId;

char useTelegram;
char useWeb;

 String useVideo = "0";
 String useCapture = "1";


boolean startAsAP = true;

AsyncWebServer server(80);
AsyncWebSocket asyncWs("/ws");

String selectedimage = "";

boolean readCheckFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
 
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return false;
    }

    Serial.println("- read config1 from file:");
    RL.readLines(file, [](char* line, int index) {
        Serial.println(String(line) + " " + String(index));
    });
    return true;
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println(String(path)+"- file deleted");
    } else {
        Serial.println(String(path)+"- delete failed");
    }
}

String deleteFirstTwenty(fs::FS &fs, const char * dirname){

    String items ="";
    int p=0;
    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return items;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return items;
    }

    File file = root.openNextFile();
    while(file){
            String fileName = file.name();
            if(fileName.startsWith( String("/picture"))){
                   if(p<=20){
                    deleteFile(SPIFFS, fileName.c_str());
                    p++;
                   }
            }
        
        file = root.openNextFile();
    }
    return items;
}

char* convert(String command){
 
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void startAP(const char* APSSID, const char* APPW ){
  Serial.println("Received:"+String(APSSID)+" and pw:"+String(APPW));
  if(sizeof(APSSID)>3 && sizeof(APPW)>3 && !apsetup){
          WiFi.mode(WIFI_AP_STA);
          Serial.println("APBroadcastSSID:"+apSSID);
          Serial.println("APBroadcastPW:"+apPASS);
          WiFi.softAP(APBroadcastSSID, APBroadcastPW);  //Start HOTspot removing password will disable security
          apsetup = true;
      }
}

int getIpBlock(int index, String str) {
    char separator = ',';
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = str.length()-1;
  
    for(int i=0; i<=maxIndex && found<=index; i++){
      if(str.charAt(i)==separator || i==maxIndex){
          found++;
          strIndex[0] = strIndex[1]+1;
          strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
    }
   // Serial.println(String(found>index ? str.substring(strIndex[0], strIndex[1]).toInt() : 0));
    return found>index ? str.substring(strIndex[0], strIndex[1]).toInt() : 0;
}

IPAddress str2IP(String str) {

    IPAddress ret( getIpBlock(0,str),getIpBlock(1,str),getIpBlock(2,str),getIpBlock(3,str) );
    return ret;

}

void configureWIFI(String staticIP, String gateway,  String subnet,
                      const char* defaultSSID, const char* defaultPW){
          // Configures static IP address
          if(staticIP.length()>6 && gateway.length()>6 
            && subnet.length()>6 && !staticup){
               Serial.println("staticIP:"+staticIP+" gateway:"+gateway+
               " subnet:"+subnet);
            if (!WiFi.config(str2IP(staticIP), str2IP(gateway), str2IP(subnet), primaryDNS, secondaryDNS)) {
              Serial.println("STA Failed to configure");
            }else{
              Serial.println("Static IP assigned");
              staticup = true;
            }
          }

          if(staticup){
          if(strlen(defaultSSID)>3 && strlen(defaultPW)>4 && !staup){
          Serial.println("Here defaultSSID:"+String(defaultSSID));
          Serial.println("here defaultPW:"+String(defaultPW));
            
            WiFi.begin(defaultSSID, defaultPW);
            int aa = 0;
            while (WiFi.status() != WL_CONNECTED) {
              delay(500);
              aa++;
              Serial.print(String(aa)+", ");
              if(aa == 20){
                stationactive = false;
                Serial.println("Breaking Station:"+aa);
                 EEPROM.write(1, 0);
                 EEPROM.commit();
                  
                break;
              }
            }

            if(WiFi.status()== WL_CONNECTED){
            Serial.println("");
            Serial.print("WiFi connected on:");
            Serial.println(WiFi.localIP());
            staup = true;
            stationactive = true;
            }else{
              Serial.println("WiFi NOT connected as Station. You should use as local or reset");
              stationactive = false;
              ESP.restart();
            }
          }
          }
}

void reconnectWIFI(const char* defaultSSID, const char* defaultPW){
        if(strlen(defaultSSID)>3 && strlen(defaultPW)>4 && !staup){
          Serial.println("Here defaultSSID:"+String(defaultSSID));
          Serial.println("here defaultPW:"+String(defaultPW));
            
            WiFi.begin(defaultSSID, defaultPW);
            int aa = 0;
            while (WiFi.status() != WL_CONNECTED) {
              delay(500);
              aa++;
              Serial.print(".");
              if(aa == 30){
                stationactive = false;
                break;
              }
            }

            if(WiFi.status()== WL_CONNECTED){
            Serial.println("");
            Serial.print("WiFi connected on:");
            Serial.println(WiFi.localIP());
            staup = true;
            stationactive = true;
            }else{
              Serial.println("WiFi NOT connected as Station. You should use as local or reset");
              stationactive = false;
            }
          }
}

String defaultParams(){
  //when functioning as AP
  String APBroadcastSSIDx = "EUCACAM";
  String APBroadcastPWx = "EUCACAM123";
  //when functioning as STA
  String defaultSSIDx = "ARISEPLAY";
  String defaultPWx = "ariseplay>";
  String staticIPx = "192,168,8,11";
  String gatewayx = "192, 168, 8, 1";
  String subnetx = "255,255,255,0";


  String defaultParams = APBroadcastSSIDx+"\n"+APBroadcastPWx+"\n"+defaultSSIDx+"\n"+defaultPWx
                         +"\n"+staticIPx+"\n"+gatewayx+"\n"+subnetx;

  return defaultParams;
}

String defaultParams2(){
  
  //http & ftp Servers
  String serverNamex = "http://eucalyptus.iq-joy.com/joseph1/eucabasic1.php";
  String ftpHostx = "ftp.iq-joy.com";
  String ftpUserx = "joseph1@eucalyptus.iq-joy.com";
  String ftpPWx = "Angel1985#20X";

  String networPublicIPx = "http://checkip.amazonaws.com";
  String telegramTokenx = "1515348043:AAHlREypfhZeCTuv3xEmUzCs6H4Q8VZwwhI";
  String telegramIdx = "546145510";

  String useTelegramx = "0";
  String useWebx = "1";

  String useVideo = "0";
  String useCapture = "1";
  
  return serverNamex+"\n"+ftpHostx+"\n"+ftpUserx+"\n"+ftpPWx+"\n"+networPublicIPx
  +"\n"+telegramTokenx +"\n"+telegramIdx+"\n"+useTelegramx+"\n"+useWebx
  +"\n"+useVideo+"\n"+useCapture;
}

boolean writeFile(fs::FS &fs, const char * path){
    Serial.printf("Writing file: %s\r\n", path);
    
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return false;
    }

    String def = defaultParams();
    if(file.print(def.c_str())){
        Serial.println("- file written");
        return true;
    } else {
        Serial.println("- frite failed");
        return false;
    }
}

boolean writeFile2(fs::FS &fs, const char * path){
    Serial.printf("Writing file: %s\r\n", path);
    
    
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return false;
    }

    String def = defaultParams2();
    if(file.print(def.c_str())){
        Serial.println("- file written");
        return true;
    } else {
        Serial.println("- frite failed");
        return false;
    }
}

void filterParams(char* line, int index){

  switch(index){

    case 0:
     APBroadcastSSID = line;
     apSSID = String(line);
     if(!startAsAP)startAP(APBroadcastSSID, APBroadcastPW);   //only start AP if it has not been started already
      break;
      
    case 1:
    APBroadcastPW = line;  
    apPASS = String(line);
    if(!startAsAP) startAP(APBroadcastSSID, APBroadcastPW);
      break;
      
    case 2:
    defaultSSID = String(line);
    Serial.println("line defaultSSID:"+defaultSSID);
      break;
      
    case 3:
    defaultPW = String(line);
    Serial.println("line defaultPW:"+defaultPW);
      break;
      
    case 4:
    statIP = String(line);
      break;
      
    case 5:
    gatew = String(line);
      break;
      
    case 6:
    subt = String(line);
      break;
      
    default:
      break;
  }
}

void filterParams2(char* line, int index){

  switch(index){
      
    case 0:
    serverName = String(line);
      break;
      
    case 1:
    ftp_server = String(line);
    
      break;
      
    case 2:
    ftp_user = String(line);
    
      break;
      
    case 3:
    ftp_pass = String(line);
    
      break;
      
    case 4:
    networPublikIP = String(line);
      break;
      
    case 5:
    telegramToken = String(line);
      break;
      
    case 6:
    telegramId = String(line);
      break;
      
    case 7:
    useTelegram = line[0];
    if(useTelegram == '1') telegram = true;
       else
        telegram = false;

      break;
      
    case 8:
     useWeb = line[0];
     if(useWeb == '1') web = true;
       else
        web = false;
      break;
      
    default:
      break;
  }
}

boolean fileReadSuccessful = false;
boolean readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    fileReadSuccessful = false;
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return false;
    }

    RL.readLines(file, [](char* line, int index) {
        Serial.println(String(line) + " " + String(index));
        filterParams(line, index); 
         
    });
     
    if(!startAsAP)configureWIFI(statIP, gatew, subt, defaultSSID.c_str(), defaultPW.c_str());

    fileReadSuccessful = true;
    return fileReadSuccessful;
}

boolean fileReadSuccessful2 = false;
boolean readFile2(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    fileReadSuccessful2 = false;
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return false;
    }

    Serial.println("- read config2 from file:");
    RL.readLines(file, [](char* line, int index) {
        Serial.println(String(line) + " " + String(index));
        filterParams2(line, index);
       if(index == 1){
          String ftpServer = String(line);
          if(ftpServer.equals("ftp.iq-joy.com")){
          Serial.println("FTP Server Correct");
          fileReadSuccessful2 = true;
        }
        }
    });


    return fileReadSuccessful2;
}

boolean listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  boolean defaultFileAvail= false;

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return false;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return false;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            String fileName = file.name();
            if(fileName.equals(defaultFile)){
              defaultFileAvail = true;
            }
            
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            String fileName = file.name();
            if(fileName.equals(defaultFile) && file.size()>10){
              defaultFileAvail = true;
            }
        }
        file = root.openNextFile();
    }

    return defaultFileAvail;
}

boolean listDir2(fs::FS &fs, const char * dirname, uint8_t levels){
  boolean defaultFileAvail= false;

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return false;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return false;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            String fileName = file.name();
            if(fileName.equals(defaultFile2)){
              defaultFileAvail = true;
            }
            
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            String fileName = file.name();
            if(fileName.equals(defaultFile2) && file.size()>10){
              defaultFileAvail = true;
            }
        }
        file = root.openNextFile();
    }

    return defaultFileAvail;
}

String getPicFiles(fs::FS &fs, const char * dirname, uint8_t levels){

    String items ="";
    int p=0;
    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return items;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return items;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            String fileName = file.name();
            if(fileName.startsWith( String("/picture"))){
             // <h3><a onclick="changeImage(this)" id="">Saved Photo1 </a></h3>
              fileName.replace("/", "");
              items+= "<h3><a onclick=\"changeImage(this)\" id="+fileName+"> "+fileName+"</a></h3> ";
              p++;
            }
        }
        file = root.openNextFile();
    }

    return items;
}

String processor(const String& var){
  Serial.println("var received:"+var);
 
  if(var == "PLACEHOLDER_CAPTURED_IMAGE"){
   String images = "<div class=\"fixed\"> ";
    String items = getPicFiles(SPIFFS, "/", 0);
    images+=items;
    images+=" </div>";
    Serial.println("List of Images:"+ images);
    return images;
  }
 
  else if(var == "PLACEHOLDER_NETWORK_NOTE"){
    //String clicked = "<img src=\"/clicked\" alt="+selectedimage+" style=\"width:100%\">";
    
    String clicked = "Wifi Name:"+defaultSSID+"<br>Wifi IP:"+statIP
                    +"<br>Cam ID:"+espMacID+"<br>Hotspot:"+apSSID+"<br>Hotspot Pass:"+apPASS;
    return clicked;
  }
 
  return String();
}

boolean checkIfSPIFFready(){
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFF is not ready");
    return false;
  }
  else {
    Serial.println("SPIFFS mounted successfully");
    return true;
  }
}

boolean formatSPIFF(){
          bool formatted = SPIFFS.format();
         
          if(formatted){
            Serial.println("\n\nSuccess formatting");
            return true;
          }else{
            Serial.println("\n\nError formatting");
            return false;
          }
          delay(3000);
}

boolean loadDefaults1(){
  if(listDir(SPIFFS, "/", 0)){  //if default file available, check if it is loaded
    Serial.println("Config file found, trying to read config file");
    if(readFile(SPIFFS, "/config.txt")){
      Serial.println("config read successful");
      return true;
    }
    else {
      Serial.println("config read unsuccessful");
      return false;
    }

  }else{  //create default file
    Serial.println("writing new parameters to config file");
    if(writeFile(SPIFFS, "/config.txt")){
      Serial.println("config write successful");
      return true;
    }else{
      Serial.println("config write unsuccessful");
      return false;
    }
  }
  
}

boolean loadDefaults2(){
  if(listDir2(SPIFFS, "/", 0)){  //if default file available, check if it is loaded
    Serial.println("Config2 file found, trying to read config file");

  if(readFile2(SPIFFS, "/config2.txt")){
    Serial.println("cconfig2 read successful");
    return true;
  }else{
    Serial.println("config2 read unsuccessful");
    return false;
  }
  }else{  //create default file
    Serial.println("writing new parameters to config2 file");
    if(writeFile2(SPIFFS, "/config2.txt")){
      Serial.println("config2 write successful");
      return true;
    }else{
      Serial.println("config2 write unsuccessful");
      return false;
    }
  }
  
}

boolean configureCamera(){
  Serial.println("Configuring Camera...");
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
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

    // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }


 sensor_t * s = esp_camera_sensor_get();
 s->set_framesize(s, FRAMESIZE_VGA);  
 s->set_contrast(s, 2);       // -2 to 2
 s->set_saturation(s, -2);     // -2 to 2
Serial.println("Camera configured!");
 return true;
}

void signalConnection(){
   //signal that you are connected by flashin built-in led
            if (WiFi.status() != WL_CONNECTED) {
             Serial.println("Going into Hotspot mode...");
            
            ledcAttachPin(4, 3);
            ledcSetup(3, 5000, 8);
            ledcWrite(3,10);
            delay(200);
            ledcWrite(3,0);
            delay(200);    
            ledcDetachPin(3);
            delay(1000);
        
          }else 
          {
            ledcAttachPin(4, 3);
            ledcSetup(3, 5000, 8);
            for (int i=0;i<5;i++) {
              ledcWrite(3,10);
              delay(200);
              ledcWrite(3,0);
              delay(200);    
            }
            ledcDetachPin(3);      
          }
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs, String picName) {
  File f_pic = fs.open( picName );
  unsigned int pic_sz = f_pic.size();
  Serial.print("check size: ");
  Serial.println(pic_sz);
  return ( pic_sz > 100 );
}

boolean ok = false;
String saveToSPIFF(camera_fb_t * fb){  
  EEPROM.begin(EEPROM_SIZE);
  if(!picNoRec){  //if it is first login, the last picture no mark is recalled
    lastFileNo = EEPROM.read(0);
    picNoRec = true;
  }
  pictureNumber = EEPROM.read(0) + 1;
  if(pictureNumber>100){
    pictureNumber = 1;
    EEPROM.write(0, pictureNumber);
    delay(100);
    lastFileNo = EEPROM.read(0);
    picNoRec = true;
  }
  Serial.println("Picture no now at:"+String(pictureNumber));
  // Path where new picture will be saved in SD Card
  lastCaptured = "/picture" + String(pictureNumber) +".jpg";
  lastCap = "/picture" + String(pictureNumber);
  Serial.println("Picture name:"+lastCaptured);
  
  File file = SPIFFS.open(lastCaptured, FILE_WRITE);

  ok = false;
  
  do{
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
       Serial.println("An Error has occurred while mounting SPIFFS");
      //formatSPIFF();
    }
    else {
      int result = file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("file.write() result: ");
      Serial.println(result);
      Serial.print("The picture has been saved in ");
      Serial.print(lastCaptured);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
      if (fb->format == PIXFORMAT_JPEG) {
        Serial.println("fb->format is jpeg");
      } else {
        Serial.println("fb->format is NOT jpeg");
      }
    }
    // Close the file
    file.close();
    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS, lastCaptured);
  }while(!ok);

  EEPROM.write(0, pictureNumber);
  EEPROM.commit();
    Serial.print("pic File is "+ok);
    return lastCaptured;
}

void checkMemory(){
    unsigned int totalBytes = SPIFFS.totalBytes();
    unsigned int usedBytes = SPIFFS.usedBytes();
 
    Serial.println("===== File system info =====");
    Serial.print("Total space:      ");
    Serial.print(totalBytes);
    Serial.println("byte");
    Serial.print("Total space used: ");
    Serial.print(usedBytes);
    Serial.println("byte");

    if(totalBytes< (usedBytes+200000)){
      Serial.println("SPIFF Memory is FULL");
      deleteFirstTwenty(SPIFFS, "/");
    }
 
    Serial.println();
}

void setupHttpRoutes() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    loopProceed = false;
    checkMemory();
    request->send_P(200, "text/html", capture_index, processor);
  });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {
    checkMemory();
    request->send_P(200, "text/html", capture_index, processor);
    ESP.restart();
  });

  server.on("/stopstream", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", stop_video_index);
  });
  
   server.on("/sensed", HTTP_GET, [](AsyncWebServerRequest * request) {
    loopProceed = false;
    selectedimage= "https://i-margin.com/wp-content/uploads/2019/09/intru.jpg";
    request->send_P(200, "text/html", motion_index, processor);
  });


   server.on("/motion", HTTP_GET, [](AsyncWebServerRequest * request) {
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    const char* PARAM_INPUT_1 = "photo";
    if (request->hasParam(PARAM_INPUT_1)) {
      selectedimage = request->getParam(PARAM_INPUT_1)->value();
      Serial.println("selectedimage="+selectedimage);
     request->send_P(200, "text/html", motion_index, processor);
    }else{
      request->redirect("/sensed");
    }
      loopProceed = true;
  });

/*
 /entersettings?apSSID=X&amp;apPASS=X&amp;rSSID=X&amp;rPASS=X&amp;statIP=X&amp;gateW=X&amp;
 subNET=X&amp;ftpSERV=X&amp;ftpUSER=X&amp;ftpPASS=X&amp;httpSERV=X&amp;publicIP=X&amp;
 alert=webradio&amp;telTOKEN=&amp;telID=&amp;capture=imagecapture
 */
     server.on("/entersettings", HTTP_GET, [](AsyncWebServerRequest * request) {
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    loopProceed = false;

    const char* param1 = "apSSID";
    const char* param2 = "apPASS";
    const char* param3 = "rSSID";
    const char* param4 = "rPASS";
    const char* param5 = "statIP";
    const char* param6 = "gateW";
    const char* param7 = "subNET";
    const char* param8 = "ftpSERV";
    const char* param9 = "ftpUSER";
    const char* param10 = "ftpPASS";
    const char* param11 = "httpSERV";
    const char* param12 = "publicIP";
    const char* param13 = "alert";
    const char* param14 = "telToken";
    const char* param15 = "telID";
    const char* param16 = "capture";


    //Access Point details
    if (request->hasParam(param1) && request->hasParam(param2)) {
      String A = request->getParam(param1)->value();
      String B = request->getParam(param2)->value();
      if(!A.equals("X") || !B.equals("X")){
        apSSID = (request->getParam(param1)->value()); 
        apPASS = request->getParam(param2)->value(); 
      }
    }

    //Client details
    if (request->hasParam(param3) && request->hasParam(param4)) {
      String A = request->getParam(param3)->value();
      String B = request->getParam(param4)->value();

      if(!A.equals("X") || !B.equals("X")){
        defaultSSID = request->getParam(param3)->value(); 
        defaultPW = request->getParam(param4)->value(); 
      }
    }

    //static ip details
    if (request->hasParam(param5) && request->hasParam(param6) && request->hasParam(param7)) {
      String A = request->getParam(param5)->value();
      String B = request->getParam(param6)->value();
      String C = request->getParam(param7)->value();
      
      if(!A.equals("X") || !B.equals("X") || !C.equals("X")){
        statIP = request->getParam(param5)->value();
        statIP.replace(".", ",");
        gatew = request->getParam(param6)->value();
        gatew.replace(".", ",");
        subt = request->getParam(param7)->value();
        subt.replace(".", ",");
      }
    }

    //ftp details
    if (request->hasParam(param8) && request->hasParam(param9) && request->hasParam(param10)) {
      String A = request->getParam(param8)->value();
      String B = request->getParam(param9)->value();
      String C = request->getParam(param10)->value();
      
      if(!A.equals("X") || !B.equals("X") || !C.equals("X")){
        ftp_server = request->getParam(param8)->value(); 
        ftp_user = request->getParam(param9)->value(); 
        ftp_pass = request->getParam(param10)->value(); 
      }
    }
  
    //http server & public i[
    if (request->hasParam(param11) && request->hasParam(param12)) {
      String A = request->getParam(param11)->value();
      String B = request->getParam(param12)->value();

      if(!A.equals("X") || !B.equals("X")){
        serverName = request->getParam(param11)->value(); 
        networPublikIP = request->getParam(param12)->value(); 
        }
    }

    //alert type
    if (request->hasParam(param13)) {
      String A = request->getParam(param13)->value();

      if(!A.equals("X")){
        String alertType = request->getParam(param13)->value();
        if(alertType.equals("webradio")){
          useWeb =  '1';
          useTelegram = '0';
        }else{
          useWeb =  '0';
          useTelegram = '1';
        }
      }
    }

    //telegram details
    if (request->hasParam(param14) && request->hasParam(param15)) {
      String A = request->getParam(param14)->value();
      String B = request->getParam(param15)->value();

      if(!A.equals("X") || !B.equals("X")){
        telegramToken = request->getParam(param14)->value(); 
        telegramId = request->getParam(param15)->value(); 
      }
    }

    if (request->hasParam(param16)) {
       String A = request->getParam(param16)->value();

      if(!A.equals("X")){
        String use = request->getParam(param16)->value(); 
        if(use.equals(String("imagecapture"))){
          useVideo = "0";
          useCapture = "1";
        }else{
          useVideo = "1";
          useCapture = "0";
        }
      }
    }

    String defaultParams = apSSID+"\n"+apPASS+"\n"+defaultSSID+"\n"+defaultPW
                         +"\n"+String(statIP)+"\n"+String(gatew)+"\n"+String(subt);

    
   String defaultParams2 = serverName+"\n"+ftp_server+"\n"+ftp_user+"\n"+ftp_pass+"\n"+networPublikIP
    +"\n"+telegramToken +"\n"+telegramId+"\n"+useTelegram+"\n"+useWeb
    +"\n"+useVideo+"\n"+useCapture;

    Serial.println("New Settings1:"+defaultParams);
    Serial.println("New Settings2:"+defaultParams2);

    const char * path1 = "/config.txt";
    const char * path2 = "/config2.txt";
    Serial.printf("Reading file: %s\r\n", path1);
     File deffile = SPIFFS.open(path1, FILE_WRITE);
     File deffile2 = SPIFFS.open(path2, FILE_WRITE);
     
    if(!deffile && !deffile2){
        Serial.println("- failed to open file for writing");
        return false;
    }


    Serial.printf("Accessing defile1 & 2: %s\r\n", path2);
    if(deffile.print(defaultParams.c_str())){
        Serial.println("Config file1 settings updated");
    } else {
        Serial.println("update 1 failed");
    }

    readCheckFile(SPIFFS, path1);

    if(deffile2.print(defaultParams2.c_str())){
            Serial.println("Config file2 settings updated");
        } else {
            Serial.println("update 2 failed");
        }

      readCheckFile(SPIFFS, path2);
      
      deffile.close();
      deffile2.close();
      request->send_P(200, "text/html", settings_entered_index);
      loopProceed = true;
      delay(1000);
      //ESP.restart();
  });

  server.on("/clicked", HTTP_GET, [](AsyncWebServerRequest * request){
    const char* PARAM_INPUT_1 = "photo";
    if (request->hasParam(PARAM_INPUT_1)) {
      selectedimage = "/"+request->getParam(PARAM_INPUT_1)->value();
      Serial.println("selectedimage="+selectedimage);
      request->send(SPIFFS, selectedimage, "image/jpg", false);
    }else{
      request->send_P(200, "text/html", capture_index);
    }
     
   });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    checkMemory();
    camera_fb_t * frame = NULL;
    frame = esp_camera_fb_get();
    esp_camera_fb_return(frame);
    saveToSPIFF(frame);
    Serial.println("Recaptured in /capture:"+lastCap);
    request->send_P(200, "text/html", capture_index, processor);
    //request->send_P(200, "image/jpeg", (const uint8_t *)frame->buf, frame->len);
  });
  

  server.on("/lastcaptured", HTTP_GET, [](AsyncWebServerRequest * request) { 
    Serial.println("This picture will be sent to client:"+lastCaptured);
    camera_fb_t * frame = NULL;
    frame = esp_camera_fb_get();
    esp_camera_fb_return(frame);
    request->send_P(200, "image/jpeg", (const uint8_t *)frame->buf, frame->len);
    loopProceed = true;
  });

 server.on("/settings", HTTP_GET, [](AsyncWebServerRequest * request) {
    //loopProceed = false;
    Serial.println("Settings index requested:"+lastCaptured);
    request->send_P(200, "text/html", settings_index, processor);
  });
  
  // TODO: refactor this repitition out
  server.on("/saved-photo1", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->redirect("/saved-photo");
  });

  server.on("/saved-photo2", HTTP_GET, [](AsyncWebServerRequest * request) {
    IPAddress serverIp = MDNS.queryHost("camera2");
    Serial.print("camera2: ");
    Serial.println(serverIp);
    if (serverIp == INADDR_NONE) {
      return;
    }
    request->redirect("http://" + serverIp.toString() + "/saved-photo");
    /*
    request->redirect("/saved-photo");
    */
  });

  server.on("/saved-photo3", HTTP_GET, [](AsyncWebServerRequest * request) {
    IPAddress serverIp = MDNS.queryHost("camera3");
    Serial.print("camera3: ");
    Serial.println(serverIp);
    if (serverIp == INADDR_NONE) {
      return;
    }
    request->redirect("http://" + serverIp.toString() + "/saved-photo");
  });

  server.on("/saved-photo4", HTTP_GET, [](AsyncWebServerRequest * request) {
    IPAddress serverIp = MDNS.queryHost("camera4");
    Serial.print("camera4: ");
    Serial.println(serverIp);
    if (serverIp == INADDR_NONE) {
      return;
    }
    request->redirect("http://" + serverIp.toString() + "/saved-photo");
    /*
    request->redirect("/saved-photo");
    */
  });

  server.on("/streamer", HTTP_GET, [](AsyncWebServerRequest * request) {
     request->send_P(200, "text/html", video_index, processor);
   });

  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest * request) {
      Serial.println("Lets stream it up!");
      streamInProgress = true;
      camera_fb_t * fb = NULL;
      size_t fb_buffer_sent = 0;

      AsyncWebServerResponse *response = request->beginChunkedResponse(_STREAM_CONTENT_TYPE, 
      [fb, fb_buffer_sent](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
        uint8_t *end_of_buffer = buffer;
        size_t remaining_space = maxLen;

        if (!fb) {
          fb = esp_camera_fb_get();
          if (!fb) {
            Serial.println("Camera capture failed");
            streamInProgress = false;
            return 0;
          }

          //res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
          size_t len = snprintf((char *)end_of_buffer, remaining_space, _STREAM_BOUNDARY);
          end_of_buffer += len;
          remaining_space -= len;

          size_t hlen = snprintf((char *)end_of_buffer, remaining_space, _STREAM_PART, fb->len);
          end_of_buffer += hlen;
          remaining_space -= hlen;

        }

        //res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        //TODO: only send max len and later finish sending the buffer
        size_t fb_bytes_to_send = min(remaining_space, fb->len-fb_buffer_sent);
        memcpy((char *)end_of_buffer, fb->buf+fb_buffer_sent, fb_bytes_to_send);
        end_of_buffer += fb_bytes_to_send;
        remaining_space -= fb_bytes_to_send;
        fb_buffer_sent += fb_bytes_to_send;

        if(fb && fb_buffer_sent == fb->len){
          esp_camera_fb_return(fb);
          fb = NULL;
          fb_buffer_sent = 0;
        }

        return maxLen - remaining_space;
      });
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
      
      streamInProgress = false;
  });
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  // TODO: this was all copied from an example and most of it can probably be deleted
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n\r", server->url(), client->id());
    //client->printf("Hello Client %u :)", client->id());
    //client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n\r", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n\r", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n\r", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n\r",msg.c_str());

      if(info->opcode == WS_TEXT) {
        //client->text("I got your text message");
      } else {
        //client->binary("I got your binary message");
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n\r", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n\r", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n\r",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n\r", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n\r", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT) {
            //client->text("I got your text message");
          } else {
            //client->binary("I got your binary message");
          }
        }
      }
    }
  }
}

void uploadSPIFFImages(){
  
  if(lastFileNo<pictureNumber){
    char* ftp_serv = convert(ftp_server);
    char* ftp_use = convert(ftp_user);
    char* ftp_pas = convert(ftp_pass);
    Serial.println("ftp_server:"+String(ftp_serv));
    Serial.println("ftp_user:"+String(ftp_use));
    Serial.println("ftp_pass:"+String(ftp_pas));
              
    ESP32_FTPClient ftp(ftp_serv,ftp_use,ftp_pas, 5000, 2);
    
     int z = lastFileNo;
     Serial.println("lastfileno: " + String(lastFileNo));
     
     for( uint8_t k = lastFileNo; k <=pictureNumber; k++){
      
        ftp.OpenConnection();
          Serial.println("pictureno: " + String(pictureNumber));
          String fille = "/picture" + String(k) +".jpg";
          String refille = "picture" + String(k) +".jpg";
          Serial.println("fille: " + fille);
          const char * fileName =  refille.c_str();
          
          unsigned int fileSize = 0;

        File file2 = SPIFFS.open(fille);
    
       
        if(!file2){
            Serial.println("Failed to open file for reading");
            return;
        }else{
          fileSize = file2.size();
           // Print file size
            Serial.println("\nFile size is: " + String(fileSize));
        }

      if(fileSize >0){
        //Dynammically alocate buffer
        unsigned char * image_file = (unsigned char *) malloc(fileSize);
        Serial.println("fileSize sent: " + fileSize);


        ftp.InitFile("Type I");
        ftp.NewFile( fileName );
        
        ftp.WriteData(image_file, fileSize);
        ftp.CloseFile();
        file2.close();
        Serial.println("FTP write completed");
      }
      ftp.CloseConnection();
   }

   lastFileNo = pictureNumber;
   delay(1000);
  }
  
}

boolean dirAvailable = false;
void ftpUpload(camera_fb_t * fb, String macID){
    Serial.println("ftp_server:"+ftp_server);
    Serial.println("ftp_user:"+ftp_user);
    Serial.println("ftp_pass:"+ftp_pass);

              
    ESP32_FTPClient ftp(const_cast<char*>(ftp_server.c_str()),const_cast<char*>(ftp_user.c_str()),
                        const_cast<char*>(ftp_pass.c_str()), 5000, 2);
              
             EEPROM.begin(EEPROM_SIZE);
              if(!picNoRec){  //if it is first login, the last picture no mark is recalled
                lastFileNo = EEPROM.read(0);
                picNoRec = true;
              }
              pictureNumber = EEPROM.read(0) + 1;
              EEPROM.write(0, pictureNumber);
              EEPROM.commit();

                
            uint8_t *fbBuf = fb->buf;
            size_t fbLen = fb->len;
            Serial.println("*fbBuf: " + String(*fbBuf));
            Serial.println("fbLen: " + String(fbLen));
            
            ftp.OpenConnection();
            String picName = "picture" + String(pictureNumber) +".jpg";
            Serial.println("picName: " + picName);
            
            String direc = "ESP32_"+macID;
            // Get directory content
            ftp.InitFile("Type A");
            String list[128];
            ftp.ContentList("", list);
            Serial.println("\nDirectory info: ");
            
            for(int i = 0; i < sizeof(list); i++){
              
              if(list[i].length() > 0){
                
              String dirList = list[i];
              Serial.println(dirList);
                if(dirList.indexOf(direc) > 0){
                  dirAvailable = true;
                  Serial.println("Directory is available");
                  break;
                }
              
              } else
                break;
                
            }

           
             // Make a new directory
            if(!dirAvailable){
              ftp.InitFile("Type A");
              ftp.MakeDir(direc.c_str());
            }
            // Create the new file and send the image
            ftp.ChangeWorkDir(direc.c_str());
            
            ftp.InitFile("Type I");
            ftp.NewFile( picName.c_str() );
            
            ftp.WriteData( fb->buf, fbLen);
            ftp.CloseFile();
            Serial.println("FTP write completed");
          
            ftp.CloseConnection();
}

String alerts2Telegram(String token, String chat_id, camera_fb_t * fb) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";

  

WiFiClientSecure client_tcp;
  
  if (client_tcp.connect(myDomain, 443)) 
{
    Serial.println("Connected to " + String(myDomain));
    
    String head = "--India\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--India\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--India--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    client_tcp.println("POST /bot"+token+"/sendPhoto HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=India");
    client_tcp.println();
    client_tcp.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;


    for (size_t n=0;n<fbLen;n=n+1024)
 {

      if (n+1024<fbLen) 
{
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) 
{
        size_t remainder = fbLen%1024;
        client_tcp.write(fbBuf, remainder);
      }
    }  
    
    client_tcp.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTime = millis();
    boolean state = false;
    
    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) 
      {
          char c = client_tcp.read();
          if (c == '\n') 
          {
            if (getAll.length()==0) state=true; 
            getAll = "";
          } 
          else if (c != '\r')
            getAll += String(c);
          if (state==true) getBody += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to telegram failed.";
    Serial.println("Connection to telegram failed.");
  }
  
  return getBody;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);
  //Serial.setDebugOutput(false);
  pinMode(gpioPIR, INPUT_PULLUP);
  pinMode(gpioLED, OUTPUT);
  
  digitalWrite(gpioLED, LOW);
 
  delay(10);

  EEPROM.begin(EEPROM_SIZE);
  int lastState = EEPROM.read(1);
  Serial.println("Last state:"+String(lastState));
  if(lastState ==0) startAsAP = true;
  else startAsAP = false;
   
 if(!configureCamera()){
  Serial.println("Could not configure camera. Restarting...");
  ESP.restart();
  }

  Serial.println("Checking fresh setup");
  if(!checkIfSPIFFready()){
    Serial.println("It's a fresh setup");
     if(formatSPIFF()){
      Serial.println("Formating Spiff");
      if(loadDefaults1()){
        Serial.println("Default parameters written to memory");
      }else{
        Serial.println("Could not write default parameters");
      }

       if(loadDefaults2()){
        Serial.println("Default parameters written to memory");
      }else{
        Serial.println("Could not write default parameters");
      }
     }
  }

  const char * path1 = "/config.txt";
  const char * path2 = "/config2.txt";

  File deffile = SPIFFS.open(path1);
  File deffile2 = SPIFFS.open(path2);
 
if(!startAsAP){
   //then start as Hotspot  
    Serial.printf("Reading file: %s\r\n", path1);
     
     if( deffile.size()>10 && deffile2.size()>10){
      Serial.println("Default files available and contains content");  
         Serial.print("\tSIZE: ");
         Serial.println(deffile.size());
         Serial.println("Loading defaults");  
         Serial.println(deffile2.size());
         Serial.println("Loading defaults");  
     if(loadDefaults1()){
            Serial.println("Default parameters written to config1");
          }else{
            Serial.println("Could not write default parameters to config1... restarting");
            ESP.restart();
          }
    
    if(loadDefaults2()){
            Serial.println("Default parameters written to config2");
          }else{
            Serial.println("Could not write default parameters to config2... restarting");
            ESP.restart();
          }
     }else{
      Serial.println("Default files may not be available");  
     Serial.print("\tSIZE: ");
     Serial.println(deffile.size());  
     
     if(formatSPIFF()){
       if(loadDefaults1()){
              Serial.println("Default parameters written to config1");
            }else{
              Serial.println("Could not write default parameters to config1... restarting");
              
            }
      
      if(loadDefaults2()){
              Serial.println("Default parameters written to config2");
            }else{
              Serial.println("Could not write default parameters to config2... restarting");
              
            }
        ESP.restart();
       }else{
          Serial.println("Could not format SPIFF");
          ESP.restart();
       }
     }
  }else{ //start as AP
  //configure AP MODE
      WiFi.softAP("EUCACAM", "EUCACAM123");
    
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      EEPROM.write(1, 1);
      EEPROM.commit();
    
      if( deffile.size()>10 && deffile2.size()>10){
         Serial.println("Default files available and contains content");  
         Serial.print("\tSIZE: ");
         Serial.println(deffile.size());
         Serial.println("Loading defaults");  
         Serial.println(deffile2.size());
        
      }else{
        
         Serial.print("\tSIZE: ");
         Serial.println(deffile.size());
    
         Serial.println(deffile2.size());
        Serial.println("Configuration files not ok"); 
        
      }
    
      if(loadDefaults1()){
            Serial.println("Default parameters written to config1");
          }else{
            Serial.println("Could not write default parameters to config1... restarting");
            ESP.restart();
          }
    
    if(loadDefaults2()){
            Serial.println("Default parameters written to config2");
          }else{
            Serial.println("Could not write default parameters to config2... restarting");
            ESP.restart();
          }
}
Serial.println("Signal connection");
  signalConnection();
  Serial.println("Check Memory");
  checkMemory();

  // Start streaming web server
  //startCameraServer();
            if(!MDNS.begin("camera1")) {
                 Serial.println("Error starting mDNS");
                 return;
              }
              MDNS.addService("http", "tcp", 80);
              
              setupHttpRoutes();
            
              asyncWs.onEvent(onWsEvent);
              server.addHandler(&asyncWs);
            
              // Start server
              DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
              server.begin();
  
  espIP = String(WiFi.localIP());
  espMacID = WiFi.macAddress();

}

int notConnect = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 2000;    
int ledState = LOW; 

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(gpioLED, ledState);
  }


  if(WiFi.status()== WL_CONNECTED){
  if(!gotPublicIP){
    Serial.println("Trying to get Public IP");
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
    
          HTTPClient http;
       
          http.begin("http://checkip.amazonaws.com/"); //Specify the URL
          int httpCode = http.GET();                                        //Make the request
       
          if (httpCode > 0) { //Check for the returning code
       
              payload = http.getString();
              Serial.println(httpCode);
              Serial.println(payload);
              gotPublicIP=true;
            }else {
            Serial.println("Error on HTTP request");
          }

          if(gotPublicIP){
            HTTPClient http2;
            payload.trim();
            String serverPath = serverName + "?ESPName="+espMacID+"&IP="+payload;
            Serial.print("serverPath:");
            Serial.println(serverPath);
            
            http2.begin(serverPath.c_str());
            int httpCode2 = http2.GET();                                        //Make the request
       
            if (httpCode2 > 0) { //Check for the returning code
                payload2 = http2.getString();
                Serial.println(httpCode2);
                Serial.println(payload2);
              }else {
              Serial.println("Error on HTTP request");
            }
          }

          
          http.end(); //Free the resources
          }
          else {
            Serial.println("WiFi Disconnected");
          }
          lastTime = millis();
        }
        delay(10000);
        }
    else{

      if(loopProceed){
        counter++;
        if(counter>50){
          if(gotPublicIP){
            HTTPClient http3;
            payload.trim();
            String url = "http://eucalyptus.iq-joy.com/joseph1/eucabasic1.php?ESPName="+espMacID+"&IP="+payload;
            Serial.print("url:");
            Serial.println(url);
           
            http3.begin(url); //Specify the URL
            int httpCode3 = http3.GET();                                        //Make the request
       
            if (httpCode3 > 0) { //Check for the returning code
                payload3 = http3.getString();
                Serial.println(httpCode3);
                Serial.println(payload3);
              }else {
              Serial.println("Error on HTTP request");
            }

            //uploadSPIFFImages();
          }
          counter =0;
        }

        if(!streamInProgress){
          //Serial.println("Checking Motion");
          int v = digitalRead(gpioPIR);
          Serial.println("Motion Sensor state: "+ String(v));
          Serial.println("currentMillis: "+ String(currentMillis));
          if(currentMillis > 20000){
          if (v==1){
              digitalWrite(gpioLED, HIGH);
              camera_fb_t * fb = NULL;
              fb = esp_camera_fb_get();  
              if(!fb) {
                Serial.println("Camera capture failed");
                delay(1000);
                ESP.restart();
              }  

            if(telegram){
              alerts2Telegram(telegramToken, telegramId, fb);
              delay(2000);
            }
             
            if(web){
              ftpUpload(fb, espMacID);
            }
            digitalWrite(gpioLED, LOW);
          }
          }
        }
       delay(1000); 
      }
    }
  }else {
            delay(500);
            notConnect++;
            Serial.print(String(notConnect)+" times ");
            if(notConnect ==200){
              Serial.println("WiFi not connected. Attempting to reconnect with:");
              Serial.println("SSID:"+defaultSSID+" pw:"+ defaultPW);
  
              reconnectWIFI(defaultSSID.c_str(), defaultPW.c_str());
              if(stationactive){
                notConnect = 0;
                }else {
                  notConnect = 0;
                  Serial.println("I have tried reconnecting. It did not work. Still on Hotspot mode...");
                  delay(1000);
                  //ESP.restart();
               }
                
            }
          }
}
