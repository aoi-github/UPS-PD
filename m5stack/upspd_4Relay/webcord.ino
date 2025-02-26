const char* htmlContent = R"rawliteral(
<!DOCTYPE HTML>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>エアバルブ制御</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      margin: 0;
      font-family: Arial, sans-serif;
      background-color: #f0f0f0;
    }
    h1 {
      margin-bottom: 20px;
    }
    .button-container {
      display: flex;
      justify-content: center;
      margin-bottom: 20px;d:\google\upspd_jusin_modified.ino
    }
    button {
      padding: 20px 40px;
      font-size: 24px;
      margin: 10px;
      border: none;
      border-radius: 10px;
      background-color: #007bff;
      color: white;
      cursor: pointer;
      transition: background-color 0.3s;
      user-select: none;
      -webkit-user-select: none;
      -ms-user-select: none;
    }
    button:hover {
      background-color: #0056b3;
    }
    button:active {
      background-color: #004085;
    }
    #status {
      margin-top: 10px;
      padding: 10px;
      background-color: #ffffff;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      text-align: left;
      white-space: pre-line;
    }
    #status ul {
      list-style-type: none;
      padding: 0;
      margin: 0;
      line-height: 1.2; /* 行間隔の調整 */
    }
    #status li {
      margin: 2px 0;
      font-size: 18px;
    }
    .control-group {
      margin: 10px 0;
    }
  </style>
  <script>
    function updateStatus() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/status", true);
      xhr.onload = function () {
        if (xhr.status === 200) {
          var status = JSON.parse(xhr.responseText);
          document.getElementById("battery").innerText = "バッテリー: " + status.battery + "%";
          document.getElementById("connectionL").innerText = "接続L: " + status.connectionL;
          document.getElementById("connectionR").innerText = "接続R: " + status.connectionR;
          document.getElementById("delay").innerText = "遅延: " + status.delay + " ms";
          document.getElementById("bpm").innerText = "BPM: " + status.bpm;
        }
      };
      xhr.send();
    }

    function setDelay() {
      var delay = document.getElementById("delaySelect").value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/setDelay?delay=" + delay, true);
      xhr.send();
    }

    function setBPM() {
      var bpm = document.getElementById("bpmSelect").value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/setBPM?bpm=" + bpm, true);
      xhr.send();
    }

    function populateDelayOptions() {
      var select = document.getElementById("delaySelect");
      for (var i = 0; i <= 2000; i += 50) {
        var option = document.createElement("option");
        option.value = i;
        option.text = i + " ms";
        select.appendChild(option);
      }
    }

    function populateBPMOptions() {
      var select = document.getElementById("bpmSelect");
      for (var i = 0; i <= 300; i += 1) {
        var option = document.createElement("option");
        option.value = i;
        option.text = i + " BPM";
        select.appendChild(option);
      }
    }

    document.addEventListener("DOMContentLoaded", function() {
      populateDelayOptions();
      populateBPMOptions();
      setInterval(updateStatus, 1000);
    });
  </script>
</head>
<body>
  <h1>エアバルブ制御</h1>
  <div id="status">
    <ul>
      <li id="battery">バッテリー: -%</li>
      <li id="connectionL">接続L: -</li>
      <li id="connectionR">接続R: -</li>
      <li id="delay">遅延: - ms</li>
      <li id="bpm">BPM: -</li>
    </ul>
  </div>
  <div class="control-group">
    <label for="delaySelect">遅延設定:</label>
    <select id="delaySelect" onchange="setDelay()"></select>
  </div>
  <div class="control-group">
    <label for="bpmSelect">BPM設定:</label>
    <select id="bpmSelect" onchange="setBPM()"></select>
  </div>
</body>
</html>
)rawliteral";
