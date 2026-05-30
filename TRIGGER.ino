#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// -------- WIFI --------
const char* ssid = "ESP32-RADAR";
const char* password = "12345678";
WebServer server(80);

// -------- PINS --------
#define SERVO1_PIN 13
#define SERVO2_PIN 12
#define SERVO3_PIN 14

#define TRIG_PIN 5
#define ECHO_PIN 18

#define GREEN_LED 2
#define RED_LED 4
#define BUZZER_PIN 15

#define RADAR_MIN 5
#define RADAR_MAX 40

Servo radarServo;
Servo targetServo;
Servo triggerServo;

int radarAngle = 90;
int direction = 1;

float distance;

bool targetLocked = false;
bool fired = false;
bool triggerActive = false;

int lockedAngle = 0;

unsigned long detectTime = 0;
unsigned long triggerTime = 0;

// -------- WEB PAGE --------
String page = R"====(
<!DOCTYPE html>
<html>
<head>
<title>Radar</title>
<style>
body { background:black; margin:0; overflow:hidden; }
canvas { display:block; }
</style>
</head>
<body>
<canvas id="radar"></canvas>
<script>
let canvas = document.getElementById("radar");
let ctx = canvas.getContext("2d");

canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

let angle = 90;
let dist = 0;

function drawRadar() {

  ctx.fillStyle = "black";
  ctx.fillRect(0,0,canvas.width,canvas.height);

  let cx = canvas.width/2;
  let cy = canvas.height;

  ctx.strokeStyle = "green";

  for(let i=1;i<=4;i++){
    ctx.beginPath();
    ctx.arc(cx, cy, i*100, Math.PI, 2*Math.PI);
    ctx.stroke();
  }

  ctx.strokeStyle = "lime";
  ctx.beginPath();
  ctx.moveTo(cx, cy);
  ctx.lineTo(cx + 300*Math.cos(angle*Math.PI/180),
             cy - 300*Math.sin(angle*Math.PI/180));
  ctx.stroke();

  if(dist < 40){
    let r = dist * 5;
    let x = cx + r*Math.cos(angle*Math.PI/180);
    let y = cy - r*Math.sin(angle*Math.PI/180);

    ctx.fillStyle = "red";
    ctx.beginPath();
    ctx.arc(x,y,5,0,2*Math.PI);
    ctx.fill();
  }
}

function updateData(){
  fetch("/data")
  .then(res => res.json())
  .then(data=>{
    angle = data.angle;
    dist = data.distance;
  });
}

setInterval(updateData, 80);
setInterval(drawRadar, 30);
</script>
</body>
</html>
)====";


// -------- SENSOR --------
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if(duration == 0) return 400;

  return duration * 0.034 / 2;
}


// -------- SETUP --------
void setup() {

  Serial.begin(115200);

  radarServo.attach(SERVO1_PIN);
  targetServo.attach(SERVO2_PIN);
  triggerServo.attach(SERVO3_PIN);

  radarServo.write(90);
  delay(500);

  triggerServo.write(0);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED, HIGH);

  WiFi.softAP(ssid, password);

  server.on("/", [](){
    server.send(200, "text/html", page);
  });

  server.on("/data", [](){
    String json = "{\"angle\":" + String(radarAngle) +
                  ",\"distance\":" + String(distance) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}


// -------- LOOP --------
void loop() {

  server.handleClient();

  // RADAR MOVEMENT (STOP WHEN TARGET)
  if (!targetLocked) {
    radarServo.write(radarAngle);

    radarAngle += direction * 2;

    if (radarAngle >= 180) {
      radarAngle = 180;
      direction = -1;
    }

    if (radarAngle <= 0) {
      radarAngle = 0;
      direction = 1;
    }
  }

  // DISTANCE
  distance = getDistance();

  // SERIAL (FOR PROCESSING)
  Serial.print(radarAngle);
  Serial.print(",");
  Serial.println(distance);

  // NORMAL BUZZER (NO BEEP-BEEP)
  if (distance > RADAR_MIN && distance < RADAR_MAX) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // TARGET LOCK
  if (!targetLocked && distance > RADAR_MIN && distance < RADAR_MAX) {

    targetLocked = true;
    fired = false;

    lockedAngle = radarAngle;
    detectTime = millis();

    radarServo.write(lockedAngle);
    targetServo.write(lockedAngle);

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }

  // TRIGGER
  if (targetLocked && !fired && millis() - detectTime > 2000) {

    triggerServo.write(90);

    triggerTime = millis();
    triggerActive = true;
    fired = true;
  }

  // RESET
  if (triggerActive && millis() - triggerTime > 300) {

    triggerServo.write(0);

    triggerActive = false;
    targetLocked = false;

    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }

  delay(5);
}