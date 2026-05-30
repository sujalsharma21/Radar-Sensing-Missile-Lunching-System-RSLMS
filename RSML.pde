import processing.serial.*;

Serial myPort;

float iAngle = 0;
float displayDistance = 0;

float objX, objY, objAlpha, objSize;
boolean isObjectPresent = false;

void setup() {
  size(1000, 600);
  smooth();

  println(Serial.list()); // Check ports
  myPort = new Serial(this, Serial.list()[0], 115200);
  myPort.bufferUntil('\n');
}

void draw() {
  background(20);

  translate(width/2, height-80);

  drawGrid();
  drawSweep();
  drawDot();
  drawText();
}

void serialEvent(Serial myPort) {
  String data = myPort.readStringUntil('\n');
  if (data != null) {
    data = trim(data);
    String[] values = split(data, ',');

    if (values.length == 2) {
      iAngle = float(values[0]);
      float d = float(values[1]);

      if (d > 0 && d < 40) {
        displayDistance = d;
        isObjectPresent = true;

        float pix = map(d, 0, 40, 0, 400);
        objX = pix * cos(radians(iAngle + 180));
        objY = pix * sin(radians(iAngle + 180));

        objAlpha = map(d, 0, 40, 255, 60);
        objSize = map(d, 0, 40, 35, 10);
      } else {
        isObjectPresent = false;
      }
    }
  }
}

void drawGrid() {
  stroke(255, 60);
  noFill();

  for (int r=200; r<=800; r+=200) {
    arc(0,0,r,r,PI,TWO_PI);
  }

  float[] angles = {0,45,90,135,180};
  for (float a:angles) {
    line(0,0,400*cos(radians(a+180)),400*sin(radians(a+180)));
  }
}

void drawSweep() {
  for (int i=0;i<15;i++) {
    float alpha = map(i,0,15,180,0);
    stroke(0,255,0,alpha);
    strokeWeight(5);
    float ang = iAngle - i;
    line(0,0,400*cos(radians(ang+180)),400*sin(radians(ang+180)));
  }
}

void drawDot() {
  if (isObjectPresent || objAlpha > 0) {
    noStroke();
    fill(255,0,0,objAlpha);
    ellipse(objX,objY,objSize,objSize);
    objAlpha -= 2;
  }
}

void drawText() {
  fill(0,255,0);
  textSize(20);
  textAlign(LEFT);
  text("RADAR SYSTEM", -430, 50);

  textAlign(RIGHT);
  text("DIST: " + nf(displayDistance,1,1)+" cm", 430, 50);
}
