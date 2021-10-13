#ifndef SERVER_H
#define SERVER_H

#include <pgmspace.h>

static char Server_HTML [] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <style>
    body {
      margin-right: auto;
      margin-left: auto;
      width: 600px;
      background-color: lightgray;
    }

    #FrontView {
      background-image: url('/FrontView.png');
      padding-bottom: 0px;
      border-bottom: 6px solid black;
    }

    div.wrapper {
      width: 100%;
      overflow: hidden;
      background-size: contain;
      background-repeat: no-repeat;
      background-position: center;
    }

    div.FrontWheels {
      width: 110px;
      height: 210px;
      margin-top: 220px;
      border-radius: 15px;
      background-color: black;
      float: left;
      opacity: 0.8;
    }

    #FrontRight {
      margin-left: 10px;
      transform: rotate(3deg);
    }

    #FrontLeft {
      margin-left: 360px;
      transform: rotate(-3deg);
    }

    .line1,
    .line2 {
      width: 0px;
      height: 400px;
      margin-top: -190px;
    }

    .line1 {
      border-left: 8px solid red;
      margin-left: 51px;
    }

    .line2 {
      border-left: 8px solid mediumblue;
      margin-left: -8px;
      transform-origin: bottom left;
    }

    #RightLine {
      transform: rotate(-3deg);
    }

    #LeftLine {
      transform: rotate(3deg);
    }

    #TopView {
      background-image: url('/TopView.png');
      padding-bottom: 250px;
    }

    div.TopWheels {
      width: 110px;
      height: 210px;
      margin-top: 155px;
      border-radius: 15px;
      background-color: black;
      float: left;
      opacity: 0.8;
    }

    #TopRight {
      margin-left: 40px;
      transform: rotate(3deg);
    }

    #TopLeft {
      margin-left: 300px;
      transform: rotate(-3deg);
    }

    #TopRightLine {
      transform: rotate(-3deg);
    }

    #TopLeftLine {
      transform: rotate(3deg);
    }

    h1,
    h2,
    h3 {
      text-align: center;
      font-variant: all-small-caps;
      font-family: sans-serif;
    }

    h1 {
      font-size: 4em;
      font-weight: bolder;
      border-bottom-style: solid;
      margin-bottom: 0px;
      font-variant: petite-caps;
    }

    h2.Camber {
      float: left;
      margin-bottom: -20px;
      width: 33.33%;
    }

    h2 {
      font-size: 3em;
    }

    h3 {
      font-size: 2em;
      margin-top: 10px;
    }

    #Toe {
      margin-top: -50px;
      margin-bottom: -40px;
    }

    #Toe,
    #CamberRight,
    #CamberLeft {
      font-size: 4em;
    }
  </style>
  <script>
  
    function handleData(val) {

      let minutesL = (val[4]).toLocaleString(undefined, {minimumIntegerDigits: 2});
      let minutesR = (val[9]).toLocaleString(undefined, {minimumIntegerDigits: 2});
      
      //let degreesL = val[2].toString(); 
      //degreesL = degreesL.substring(0,degreesL.length - 1);
      //degreesL = degreesL + val[3];
      
      //let degreesR = val[7].toString(); 
      //degreesR = degreesR.substring(0,degreesR.length - 1);
      //degreesR = degreesR + val[8];
      
      document.getElementById('FrontLeft').style.transform = 'rotate(' + val[0] + 'deg)'; 
      document.getElementById('LeftLine').style.transform = 'rotate(' + val[1] + 'deg)';
      
      document.getElementById('CamberLeft').innerHTML = val[2] + "°" + val[3] + "'";
      
      document.getElementById('FrontRight').style.transform = 'rotate(' + val[4] + 'deg)';
      document.getElementById('RightLine').style.transform = 'rotate(' + val[5] + 'deg)';
      
      document.getElementById('CamberRight').innerHTML = val[6] + "°" + val[7] + "'";
      
      document.getElementById('TopLeft').style.transform = 'rotate(' + val[8] + 'deg)';
      document.getElementById('TopLeftLine').style.transform = 'rotate(' + val[9] + 'deg)';
      document.getElementById('TopRight').style.transform = 'rotate(' + val[10] + 'deg)';
      document.getElementById('TopRightLine').style.transform = 'rotate(' + val[11] + 'deg)';
      document.getElementById('Toe').innerHTML = val[12] + "°" + val[13] + "'";
    }

    function getdata() {
      let oReq = new XMLHttpRequest();
      oReq.open("GET", "/api/data");
      oReq.addEventListener("load", function() {
        let rawData = JSON.parse(oReq.response);
        handleData(rawData);
      });
      oReq.send();
    }

    function streamdata() {
      streamdatatimer = setTimeout(streamdata, 200);
      getdata();
    }
    
  </script>
</head>
<body onload="streamdata()">
  <h1>Trackside Aligner</h1>
  <h3>By Wilkinson Engineering</h3>
  <div>
    <h2 id="CamberRight" class="Camber">-3°00'</h2>
    <h2 class="Camber">Camber</h2>
    <h2 id="CamberLeft" class="Camber">-3°00'</h2>
  </div>
  <div id="FrontView" class="wrapper">
    <div id="FrontRight" class="FrontWheels">
      <div class="line1">
        <div id="RightLine" class="line2"></div>
      </div>
    </div>
    <div id="FrontLeft" class="FrontWheels">
      <div class="line1">
        <div id="LeftLine" class="line2"></div>
      </div>
    </div>
  </div>
  <h2>Toe</h2>
  <h2 id="Toe">-3°00'</h2>
  <div id="TopView" class="wrapper">
    <div id="TopRight" class="TopWheels">
      <div class="line1">
        <div id="TopRightLine" class="line2"></div>
      </div>
    </div>
    <div id="TopLeft" class="TopWheels">
      <div class="line1">
        <div id="TopLeftLine" class="line2"></div>
      </div>
    </div>
  </div>
</body>
</html>

)=====";

#endif
