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
        <button type="button" class="btn" id="btn1" onclick="deepSleepButtonPress()">Turn On</button>                                         
        <br><br>
        <div class="text">Data </div>
        <button type="button" class="btn" id="btn2" onclick="getDataButtonPress()">Download</button>                                               
    </body>

    <script type = "text/javascript">
        const xmlHttp = new XMLHttpRequest();

        function connection() {
            if(xmlHttp.readyState == 0 || xmlHttp.readyState == 4) {
                xmlHttp.open("PUT", "/", true);
                xmlHttp.send(null);
            } 
        }

        function powerButtonPress() {
            var request = new XMLHttpRequest();
            var btn = document.getElementById('btn0');

            request.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    if(btn.textContent == "Turn Off") {
                        btn.textContent = "Turn On";
                    }
                    else {
                        btn.textContent = "Turn Off";
                    }
                }
            }

            request.open("PUT", "power", false);
            request.send(); 
        }

        function updateStartTime(startTime) {
            var request = new XMLHttpRequest();
            var startTime = document.getElementById('time1')

            request.open("PUT", "startTime?VALUE=" + startTime.value, true);
            request.send(); 
        }

        function updateEndTime(endTime) {
            var request = new XMLHttpRequest();
            var endTime = document.getElementById('time2')

            request.open("PUT", "endTime?VALUE=" + endTime.value, true);
            request.send(); 
        }

        function deepSleepButtonPress() {
            var request = new XMLHttpRequest();
            var btn0 = document.getElementById('btn0');
            var btn1 = document.getElementById('btn1');
            var btn2 = document.getElementById('btn2');

            request.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    if(btn1.textContent == "Turn On") {
                        btn1.textContent = "Activated";
                        btn0.textContent = "Blocked";
                        btn2.textContent = "Blocked";
                    }
                    else {
                        btn1.textContent = "Turn On";
                    }
                }
            }

            request.open("PUT", "sleep", false);
            request.send(); 
        }

        function getDataButtonPress() {

        }

    </script>
</html>

)=====";