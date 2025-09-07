#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>



const char *ssid = "Aryan";
const char *password = "12345679";

const int light = 2;
const int motor1 = 12;
const int motor2 = 23;
const int motor3 = 33;
const int motor1_dir = 13;
const int motor2_dir = 15;
const int motor3_dir = 32;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Omni Bot Controller</title>
    <style>
        body {
            margin: 0;
            height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            background:white;
            font-family: Arial, sans-serif;
        }

        .controller-box {
            background: #000;
            border-radius: 30px;
            width: 600px;
            max-width: 90%;
            padding: 30px;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 30px;
        }

        .top-section {
            display: flex;
            width: 100%;
            justify-content: space-between;
            align-items: center;
        }

        .joystick {
            width: 120px;
            height: 120px;
            background: white;
            border-radius: 50%;
            position: relative;
            touch-action: none;
        }

        .stick {
            width: 50px;
            height: 50px;
            background: #555;
            border-radius: 50%;
            position: absolute;
            top: 35px;
            left: 35px;
            transition: top 0.05s, left 0.05s;
        }

        .status {
            background: #111;
            padding: 15px 20px;
            border-radius: 12px;
            color: #fff;
            font-size: 14px;
            text-align: center;
            min-width: 180px;
        }

        .dpad {
            display: grid;
            grid-template-columns: repeat(3, 60px);
            grid-template-rows: repeat(3, 60px);
            gap: 5px;
            justify-content: center;
            align-items: center;
        }

        .dpad button {
            width: 60px;
            height: 60px;
           background-color: white;
            color:#111;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            cursor: pointer;
        }

        .buttons-row {
            display: flex;
            gap: 15px;
            margin-top: 20px;
            flex-wrap: wrap;
            justify-content: center;
        }

        .buttons-row button {
            padding: 10px 20px;
            border: none;
            border-radius: 10px;
            background:#fff ;
            color: #444;
            cursor: pointer;
            font-size: 14px;
        }

        .buttons-row button:active {
            background: #666;
        }

        .disabled {
            opacity: 0.5;
            pointer-events: none;
        }
    </style>
</head>

<body>
    <div class="controller-box">
        <div class="top-section">
            <div class="joystick disabled" id="joystick1">
                <div class="stick"></div>
            </div>

            <div class="status" id="status">Status: Locked (Press Start)</div>

            <div class="dpad">
                <div></div>
                <button id="upBtn" class="disabled">↑</button>
                <div></div>
                <button id="ccwBtn" class="disabled">⟲</button>
                <div></div>
                <button id="cwBtn" class="disabled">⟳</button>
                <div></div>
                <button id="downBtn" class="disabled">↓</button>
                <div></div>
            </div>
        </div>

        <div class="buttons-row">
            <button id="startBtn">Start</button>
            <button id="stopBtn">Stop</button>
            <button id="wifiBtn">WiFi</button>
        </div>
    </div>

    <script>
        const ws = new WebSocket(`ws://${window.location.hostname}:81/`);

        const joystick = document.getElementById("joystick1");
        const stick = joystick.querySelector(".stick");
        const status = document.getElementById("status");
        let dragging = false;
        let controlsEnabled = false;

        let currentK = 0.6;
        let currentLevel = 2;

        function updateStatus(mainText) {
            if (controlsEnabled) {
                status.innerHTML =
                    `Status: ${mainText}<br>` +
                    `Speed Level: ${currentLevel}<br>`;
            } else {
                status.innerHTML = `Status: ${mainText}`;
            }
        }

        function resetStick() {
            stick.style.left = "35px";
            stick.style.top = "35px";
        }

        function moveStick(x, y) {
            if (!controlsEnabled) return;
            const rect = joystick.getBoundingClientRect();
            const centerX = rect.width / 2 - stick.offsetWidth / 2;
            const centerY = rect.height / 2 - stick.offsetHeight / 2;

            let dx = x - rect.left - centerX - stick.offsetWidth / 2;
            let dy = y - rect.top - centerY - stick.offsetHeight / 2;

            const maxDist = 35;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist > maxDist) {
                dx = (dx * maxDist) / dist;
                dy = (dy * maxDist) / dist;
            }

            stick.style.left = centerX + dx + "px";
            stick.style.top = centerY + dy + "px";
            updateStatus(`Joystick Active<br>dx=${dx.toFixed(0)}, dy=${-dy.toFixed(0)}`);

            let a = Math.trunc((dx / maxDist) * 127);
            let b = Math.trunc((-dy / maxDist) * 127);
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(`${a},${b}`);
            }
        }

        // Joystick events
        joystick.addEventListener("mousedown", () => {
            if (controlsEnabled) dragging = true;
        });
        joystick.addEventListener("mouseup", () => {
            if (controlsEnabled) {
                dragging = false;
                resetStick();
                updateStatus("Idle");
                ws.send("0,0");
            }
        });
        joystick.addEventListener("mouseleave", () => {
            if (controlsEnabled) {
                dragging = false;
                resetStick();
                updateStatus("Idle");
                ws.send("0,0");
            }
        });
        joystick.addEventListener("mousemove", (e) => {
            if (dragging && controlsEnabled) moveStick(e.clientX, e.clientY);
        });
        joystick.addEventListener("touchstart", () => {
            if (controlsEnabled) dragging = true;
        });
        joystick.addEventListener("touchend", () => {
            if (controlsEnabled) {
                dragging = false;
                resetStick();
                updateStatus("Idle");
                ws.send("0,0");
            }
        });
        joystick.addEventListener("touchmove", (e) => {
            if (dragging && controlsEnabled)
                moveStick(e.touches[0].clientX, e.touches[0].clientY);
        });
        window.addEventListener("resize", resetStick);
        resetStick();

        // Button logic
        function createButton(id, activeMessage) {
            const btn = document.getElementById(id);
            let interval = null;

            const startAction = () => {
                if (!controlsEnabled) return;
                updateStatus(activeMessage);
                ws.send(activeMessage);
            };

            const stopAction = () => {
                ws.send("0,0");
                clearInterval(interval);
                interval = null;
                if (controlsEnabled) updateStatus("Idle");
            };

            // btn.addEventListener("mousedown", startAction);
            // btn.addEventListener("mouseup", stopAction);
            // btn.addEventListener("mouseleave", stopAction);
            btn.addEventListener("touchstart", startAction);
            btn.addEventListener("touchend", stopAction);
        }

        createButton("cwBtn", "Rotating Clockwise");
        createButton("ccwBtn", "Rotating Anticlockwise");
        createButton("upBtn", "Speed UP");
        createButton("downBtn", "Speed Down");

        // Enable/Disable controls
        function setControls(state) {
            controlsEnabled = state;
            const buttons = document.querySelectorAll(".dpad button, .joystick");
            if (state) {
                buttons.forEach((b) => b.classList.remove("disabled"));
                updateStatus("Controls Enabled");
            } else {
                buttons.forEach((b) => b.classList.add("disabled"));
                dragging = false;
                resetStick();
                status.innerHTML = "Status: Locked (Press Start)"; // hide speed
            }
        }

        document.getElementById("startBtn").onclick = () => setControls(true);
        document.getElementById("stopBtn").onclick = () => setControls(false);
        document.getElementById("wifiBtn").onclick = () =>
            updateStatus("WiFi Connecting...");

        // Handle messages from ESP32
        ws.onmessage = (event) => {
            try {
                const val = JSON.parse(event.data);
                if (val.k !== undefined && val.level !== undefined) {
                    currentK = parseFloat(val.k);
                    currentLevel = val.level;
                    if (controlsEnabled) updateStatus("Updated from ESP32");
                }
            } catch (e) {
                console.log("Non-JSON message:", event.data);
            }
        };
    </script>
</body>

</html>
)rawliteral";

float k = 0.5f;
int w1 = 0, w2 = 0, w3 = 0;
int w1_dir = LOW, w2_dir = LOW, w3_dir = LOW;
void func(int x, int y, int &w1, int &w2, int &w3, int &w1_dir, int &w2_dir, int &w3_dir) {
  float fw1 = 1.155f * y;
  float fw2 = -x - 0.577f * y;
  float fw3 = x - 0.577f * y;

  if (fw1 < 0) {
    fw1 = -fw1;
    w1_dir = LOW;
  } else w1_dir = HIGH;
  if (fw2 < 0) {
    fw2 = -fw2;
    w2_dir = LOW;
  } else w2_dir = HIGH;
  if (fw3 < 0) {
    fw3 = -fw3;
    w3_dir = LOW;
  } else w3_dir = HIGH;


  fw1 = fw1 * k;
  fw2 = fw2 * k;
  fw3 = fw3 * k;

  w1 = (int)fw1;
  w2 = (int)fw2;
  w3 = (int)fw3;
}

int getSpeedLevel(float k) {
  return k*10-2;
}
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("Websocket connected");
  } else if (type == WStype_DISCONNECTED) {
    Serial.println("Websocket  disconnected");
  } else if (type == WStype_TEXT) {
    String msg = String((char *)payload);
    Serial.println(msg);
    if (msg == "Speed UP" && k < 0.7f) {
      k += 0.1f;
      String msg = "{\"k\":" + String(k) + ",\"level\":" + String(getSpeedLevel(k)) + "}";
      webSocket.sendTXT(num, msg);
    } else if (msg == "Speed Down" && k > 0.3f) {
      k -= 0.1f;
      String msg = "{\"k\":" + String(k) + ",\"level\":" + String(getSpeedLevel(k)) + "}";
      webSocket.sendTXT(num, msg);
    }
    if (msg == "Rotating Clockwise") {
      w1 = w2 = w3 = 150 * k;
      w1_dir = w2_dir = w3_dir = LOW;
    } else if (msg == "Rotating Anticlockwise") {
      w1 = w2 = w3 = 150 * k;
      w1_dir = w2_dir = w3_dir = HIGH;
    } else {
      int x = 0, y = 0;
      int commaIndex = msg.indexOf(',');
      if (commaIndex > 0) {
        x = msg.substring(commaIndex + 1).toInt();
        y = -msg.substring(0, commaIndex).toInt();
      }
      func(x, y, w1, w2, w3, w1_dir, w2_dir, w3_dir);
    }
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  pinMode(light, OUTPUT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    digitalWrite(light, HIGH);
    delay(1000);
    digitalWrite(light, LOW);
  }
  digitalWrite(light, HIGH);
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Serve HTML page
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", index_html);
  });

  server.begin();

  // Start WebSocket
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);


  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor1_dir, OUTPUT);
  pinMode(motor2_dir, OUTPUT);
  pinMode(motor3_dir, OUTPUT);
}

void loop() {
  server.handleClient();
  webSocket.loop();

  analogWrite(motor1, w1);
  analogWrite(motor2, w2);
  analogWrite(motor3, w3);
  digitalWrite(motor1_dir, w1_dir);
  digitalWrite(motor2_dir, w2_dir);
  digitalWrite(motor3_dir, w3_dir);
}