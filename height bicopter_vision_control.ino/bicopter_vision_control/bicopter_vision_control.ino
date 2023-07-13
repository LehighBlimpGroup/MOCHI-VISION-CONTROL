#include <crazyflieComplementary.h>
#include "WiFi.h"
#include <IBusBM.h>
#include "AsyncUDP.h"
#include <ESP32Servo.h>

#define SERVO1 D2
#define SERVO2 D3
#define THRUST1 D0 //0
#define THRUST2 D1 //1
 
const char * ssid = "AIRLab-BigLab";
const char * password = "Airlabrocks2022";

Servo servo1;
Servo servo2; 
Servo thrust1;
Servo thrust2;

// SensFusion sensorSuite;

//*************************************
//iBus protocols
IBusBM IBus; 
HardwareSerial MySerial0(0);
//*************************************

AsyncUDP udp;

// float joy_data[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
// volatile bool joy_ready = false;
// volatile unsigned long time_now, time_loop; 

//added code
//Enter arming sequence for ESC
void escarm(Servo& thrust1, Servo& thrust2) {
  // ESC arming sequence for BLHeli S
  thrust1.writeMicroseconds(1000);
  delay(10);
  thrust2.writeMicroseconds(1000);
  delay(10);

  // Sweep up
  for (int i=1100; i<1500; i++) {
    thrust1.writeMicroseconds(i);
    delay(5);
    thrust2.writeMicroseconds(i);
    delay(5);
  }
  // Sweep down
  for (int i=1500; i<1100; i--) {
    thrust1.writeMicroseconds(i);
    delay(5);
    thrust2.writeMicroseconds(i);
    delay(5);
  }
  // Back to minimum value
  thrust1.writeMicroseconds(1000);
  delay(10);
  thrust2.writeMicroseconds(1000);
  delay(10);
} 

float roll, pitch, yaw;
float kpz = .2; // prop gain in z
float kdz = 0; 
float kpx = .4; // prop gain in x
float kdx = .2;
float kptz = 0.05; // prop gain in angle in z .3
float kdtz = 0.1;
float kptx = .01; // prop gain in angle in x
float kdtx = .01;
float lx = .15; // distance b/t motors and conters
float m1, m2, s1, s2;
float yaw_control, height_control;

void setup() {
  //For debugging
  Serial.begin(115200);
  delay(500);
  //For reading iBus
  //*************************************
  MySerial0.begin(115200, SERIAL_8N1, -1, -1);
  IBus.begin(MySerial0, IBUSBM_NOTIMER);
  //*************************************
  //Setting up pins servos and motors
  pinMode(SERVO1, OUTPUT);
  pinMode(SERVO2, OUTPUT);
  pinMode(THRUST1, OUTPUT);
  pinMode(THRUST2, OUTPUT);
  //while(!Serial);
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);// Standard 50hz servo
  servo2.setPeriodHertz(50);// Standard 50hz servo
  thrust1.setPeriodHertz(51);
  thrust2.setPeriodHertz(51);

  servo1.attach(SERVO1, 450, 2550);
  servo2.attach(SERVO2, 450, 2550);
  thrust1.attach(THRUST1, 1000, 2000);
  thrust2.attach(THRUST2, 1000, 2000);
  // Testing
  escarm(thrust1, thrust2);
  delay(500);

  float transformationMatrix[3][3] = {
    {    1.0000f,  -32.2488f,   -0.4705f},
    {  -30.6786f,   -0.2169f,   -5.6020f},
    {   -1.1802f,    0.0597f,   35.5136f}
  };
  float offsets[3] = {20.45f, 64.11f, -67.0f};

  servo1.write((int) 0);
  servo2.write((int) 0);
  delay(500);
  servo1.write((int) 30);
  servo2.write((int) 30);

}

// Yaw control loop()
void loop() {
  float cfx, cfy, cfz, ctx, cty, ctz, abz;   //control force x    control torque x
  cfx = 0;//joy_data[0];
  cfy = 0;//joy_data[1];
  cfz = 1;//joy_data[2];
  ctx = 0;//joy_data[3];
  cty = 0;//joy_data[4];
  ctz = 0;//joy_data[5];
  abz = 0;//joy_data[6];
  //*************************************
  // Getting messages from OpenMV
  IBus.loop();
  int cx = IBus.readChannel(0);
  int cy = IBus.readChannel(1);
  int half_width = 80;
  int half_height = 60;
  Serial.print("\nCh1: cx=");
  Serial.print(cx);
  Serial.print("\nwidth=");
  Serial.print(half_width);
  Serial.print("\nCh2: cy=");
  Serial.print(cy);
  Serial.print("\nheight=");
  Serial.print(half_height);
  float value_x, value_z;
  // if (cx != 0) {
  //   value_x = (cx - half_width);
  //   cfz= 1;
  //   Serial.print("\nx position on frame=");
  //   Serial.print(value_x);
  // } else {
  //   value_x = 0;
  // }
  if (cy != 0) {
    value_z = (-cy + half_height);
    cfz= 1;
    Serial.print("\ny position on frame=");
    Serial.print(value_z);
  } else {
    value_z = 0;
  }
  // yaw_control = -value_x/half_width;
  // Serial.print("\nyaw_control=");
  // Serial.print(yaw_control);
  height_control = 3+(value_z/half_height);
  Serial.print("\nheight_control=");
  Serial.print(height_control);
  //*************************************

  // addFeedback(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, abz);
  addFeedback(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, &height_control);// &yaw_control,
  // Serial.print("\nctz=");
  // Serial.println(ctz);
  Serial.print("\ncfz=");
  Serial.println(cfz);
  controlOutputs(cfx, cfy, cfz, ctx, cty, ctz);

  servo1.write((int) (s1*180));
  servo2.write((int) ((1-s2)*180));
  thrust1.write((int) (m1*180));
  thrust2.write((int) (m2*180));
  
  // thrust1.writeMicroseconds(1100 + 400*m1);
  // thrust2.writeMicroseconds(1100 + 400*m2);

  Serial.print((int) (s1*180));
  Serial.print(",");
  Serial.print((int) (s2*180));
  Serial.print(",");
  Serial.print(m1);
  Serial.print(",");
  Serial.println(m2);
  delay(20);
  
}

void addFeedback(float *fx, float *fy, float *fz, float *tx, float *ty, float *tz,  float *height_control) {//float *yaw_control,
  // height control
  //*fz = *fz; //+ *height_control * kpz 
  // yaw control
  // *tz = *yaw_control * kptz; //- yawrateave*kdtz;
  *fz = *height_control * kpz;

}

float clamp(float in, float min, float max) {
  if (in < min) {
    return min;
  } else if (in > max) {
    return max;
  } else {
    return in;
  }
} //in is input

void controlOutputs(float ifx, float ify, float ifz, float itx, float ity, float itz) {
    //float desiredPitch = wty - self->pitch*(float)g_self.kR_xy - self->pitchrate *(float)g_self.kw_xy;
    // ifx=cfx;
    float l = lx; //.3
    float fx = clamp(ifx, -1 , 1);//setpoint->bicopter.fx;
    float fz = clamp(ifz, 0 , 2);//setpoint->bicopter.fz;
    float taux = clamp(itx, -l + (float)0.01 , l - (float) 0.01);
    float tauz = clamp(itz, -.3 , .3);// limit should be .25 setpoint->bicopter.tauz; //- stateAttitudeRateYaw

    float term1 = l*l*fx*fx + l*l*fz*fz + taux*taux + tauz*tauz;
    float term2 = 2*fz*l*taux - 2*fx*l*tauz;
    float term3 = sqrt(term1+term2);
    float term4 = sqrt(term1-term2);

    float f1 = term3/(2*l); // in unknown units
    float f2 = term4/(2*l);

    float t1 = atan2((fz*l - taux)/term3, (fx*l + tauz)/term3 );// in radians
    float t2 = atan2((fz*l + taux)/term4, (fx*l - tauz)/term4 );
  
    while (t1 < 0) {
      t1 = t1 + 2 * PI;
    }
    while (t1 > 2*PI) {
      t1 = t1 - 2 * PI;
    }
    while (t2 < 0) {
      t2 = t2 + 2 * PI;
    }
    while (t2 > 2*PI) {
      t2 = t2 - 2 * PI;
    }
    s1 = clamp(t1, 0, PI)/(PI);// cant handle values between PI and 2PI
    s2 = clamp(t2, 0, PI)/(PI);
    m1 = clamp(f1, 0, 1);
    m2 = clamp(f2, 0, 1);
    if (m1 < 0.02f ) {
      s1 = 0.5f; 
    }
    if (m2 < 0.02f ) {
      s2 = 0.5f; 
    }
}