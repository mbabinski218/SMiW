#include <Arduino.h>

const char mainPage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en" class="js-focus-visible">
    <title>Web Page</title>

    <style>
        .text {
            font-family: "Verdana", "Arial", sans-serif;
            font-size: 4rem;
            text-align: left;
            font-weight: light;
            border-radius: 5rem;
            padding-left: 1rem;
            display:inline;
        }

        .navbar {
            width: 100%;
            height: 6rem;
            padding: 2rem 2rem;
            background-color: #FFF;
            color: #000000;
            border-bottom: 2rem solid #293578;
        }

        .fixed-top {
            position: fixed;
            top: 0;
            right: 0;
            left: 0;
            z-index: 1030;
        }

        .navtitle {
            float: left;
            height: 5rem;
            font-family: "Verdana", "Arial", sans-serif;
            font-size: 6rem;
            font-weight: bold;
            line-height: 5rem;
            padding-left: 1rem;
        }
    
        .btn {
            background-color: #444444;
            border: none;
            color: white;
            padding: 2rem 5rem;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            font-size: 3rem;
            margin: 2rem 1rem;
            cursor: pointer;
        }

        .container {
            max-width: 1000rem;
            margin: 0 auto;
        }

        br {
            content: "";
            margin: 1rem;
            display: block;
            font-size: 24%;
        }

        .pad {
            padding-top: 13rem;
        }
    </style>

    <body style="background-color: #ebebeb" onload="connection()">                                                                              
        <header>
            <div class="navbar fixed-top">
                <div class="container">
                    <div class="navtitle">Controls</div>
                </div>
            </div>
        </header>

        <div class="pad"></div>
        <div class="text">Power </div>
        <button type="button" class="btn" id="btn0" onclick="powerButtonPress()">Turn On</button>                                        
        <br><br>
        <div class="text">Work hours:</div>
        <br>        
        <input type="time" class="btn" id="time1" name="time1" oninput="updateStartTime()"</input>          
        <input type="time" class="btn" id="time2" name="time1" oninput="updateEndTime()"</input>                      
        <br><br>
        <div class="text">Sleep</div>
        <button type="button" class="btn" id="btn1" onclick="sleepButtonPress()">Turn On</button>                                         
        <br><br>
        <div class="text">Data </div>
        <button type="button" class="btn" id="btn2" onclick="getDataButtonPress()">Get</button>
        <br><br>  
        <div class="text" id="data"></div>                                           
    </body>

    <script type = "text/javascript">
        const xmlHttp = new XMLHttpRequest();

        function connection() {
            if(xmlHttp.readyState == 0 || xmlHttp.readyState == 4) {
                xmlHttp.open("POST", "/", false);
                xmlHttp.send();

                var response = new XMLHttpRequest();
                response.open("GET", "/info", false);
                response.send();
                if(response.readyState == 4 && response.status == 200) {
                    update(response);
                }                  
            }
        }

        function powerButtonPress() {
            var request = new XMLHttpRequest();
            request.open("POST", "power", false);
            request.send();

            if(request.readyState == 4 && request.status == 200) {
                update(request);
            }    
        }

        function updateStartTime(startTime) {
            var request = new XMLHttpRequest();
            var startTime = document.getElementById('time1').value;
            request.open("POST", "startTime?VALUE=" + startTime, false);
            request.send(); 

            if(request.readyState == 4 && request.status == 200) {
                update(request);
            } 
        }

        function updateEndTime(endTime) {
            var request = new XMLHttpRequest();
            var endTime = document.getElementById('time2').value;
            request.open("POST", "endTime?VALUE=" + endTime, false);
            request.send();

            if(request.readyState == 4 && request.status == 200) {
                update(request);
            } 
        }

        function sleepButtonPress() {
            var request = new XMLHttpRequest();
            request.open("POST", "sleep", false);
            request.send();

           
                update(request);
            
        }

        function getDataButtonPress() {
            var response = new XMLHttpRequest();
            response.open("GET", "/data", false);
            response.send();

            if(response.readyState == 4 && response.status == 200) {
                var text;
                var data = response.responseXML.getElementsByTagName("Data")[0].childNodes[0].nodeValue;
                if(data !== "empty") {
                    data = data.replace(/(?:\r\n|\r|\n)/g, '<br>');
                    text = "Motion detected: <br>" + data;
                }
                else {
                    text = "No motion detected";
                }

                document.getElementById('data').innerHTML = text;
            }
        }

        function update(response) {
            var xmlDoc = response.responseXML.getElementsByTagName("Info")[0];

            var modeBtn = document.getElementById('btn0');
            var sleepBtn = document.getElementById('btn1');
            var dataBtn = document.getElementById('btn2');

            var mode = xmlDoc.getElementsByTagName("Mode")[0].childNodes[0].nodeValue;
            var sleep = xmlDoc.getElementsByTagName("Sleep")[0].childNodes[0].nodeValue;
            var startTime = xmlDoc.getElementsByTagName("StartTime")[0].childNodes[0].nodeValue;
            var endTime = xmlDoc.getElementsByTagName("EndTime")[0].childNodes[0].nodeValue;

            if(mode === "1") {
                modeBtn.textContent = "Turn Off";
            }
            else {
                modeBtn.textContent = "Turn On";
            }

            if(startTime !== "not set") {
                document.getElementById('time1').value = startTime;            
            }

            if(endTime !== "not set") {
                document.getElementById('time2').value = endTime; 
            }

            if(sleep === "1") {
                modeBtn.textContent = "Blocked";
                sleepBtn.textContent = "Activated";
                dataBtn.textContent = "Blocked";
            }
            else {
                sleepBtn.textContent = "Turn On";
            }
        }

    </script>
</html>

)=====";