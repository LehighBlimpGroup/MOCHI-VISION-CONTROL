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

float joy_data[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
volatile bool joy_ready = false;
volatile unsigned long time_now, time_loop; 

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
// float rollrate, pitchrate, yawrate, yawrateave;
// float estimatedZ, velocityZ, groundZ; //from sensor
// float abz; //absolute heihgt from sensor
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
  
  // sensorSuite.initSensors();
  // sensorSuite.updateKp(5,-1,0);//5,-1,0.3
  // groundZ = sensorSuite.returnZ();
  //sensorSuite.recordData();

  float transformationMatrix[3][3] = {
    {    1.0000f,  -32.2488f,   -0.4705f},
    {  -30.6786f,   -0.2169f,   -5.6020f},
    {   -1.1802f,    0.0597f,   35.5136f}
  };
  float offsets[3] = {20.45f, 64.11f, -67.0f};
  // sensorSuite.enterTransform(offsets, transformationMatrix);
  // getSensorValues();

  servo1.write((int) 0);
  servo2.write((int) 0);
  delay(500);
  servo1.write((int) 30);
  servo2.write((int) 30);

  // //Setting up WiFi
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.println("WiFi Failed");
  //   while(true) {
  //     delay(3000);
  //     servo1.write((int) 180);
  //     servo2.write((int) 0);
  //     delay(3000);
  //     servo1.write((int) 0);
  //     servo2.write((int) 180);
  //   }
  // }

  // servo1.write((int) 50);
  // servo2.write((int) 50);
  // delay(500);
  // servo1.write((int) 90);
  // servo2.write((int) 90);

  // set the motor out pins as outputs
  //pinMode(THRUST1, OUTPUT);
  //pinMode(THRUST2, OUTPUT);

  // time_now = millis();
  // time_loop = millis();
  // if(udp.listen(1333)) {
  //   Serial.print("UDP Listening on IP: ");
  //   Serial.println(WiFi.localIP());
  //   // setup callback functions of the udp
  //   udp.onPacket([](AsyncUDPPacket packet) {
  //     joy_ready = false;
  //     time_now = millis();
  //     unsigned char *buffer = packet.data();
  //     //buff = *buffer;
  //     unpack_joystick(joy_data, buffer);
  //     joy_ready = true;
  //     //reply to the client
  //     //packet.printf("Got %u bytes of data", packet.length());
  //   });
  // }

  // servo1.write((int) 110);
  // servo2.write((int) 110);
  // delay(500);
  // servo1.write((int) 150);
  // servo2.write((int) 150);
  // delay(500);
  // servo1.write((int) 90);
  // servo2.write((int) 90);

}

// Yaw control loop()
void loop() {
  float cfx, cfy, cfz, ctx, cty, ctz, abz;   //control force x    control torque x
  cfx = 0;//joy_data[0];
  cfy = 0;//joy_data[1];
  cfz = 0;//joy_data[2];
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
  Serial.print(half_width);
  Serial.print("\nCh2: cy=");
  Serial.print(cy);
  Serial.print( half_height);
  float value_x, value_z;
  if (cx != 0) {
    value_x = (cx - half_width);
    Serial.print("\nx position on frame=");
    Serial.print(value_x);
    cfz = .1;
  } else {
    value_x = 0;
  }
  if (cy != 0) {
    value_z = (cy - half_height);
    Serial.print("\ny position on frame=");
    Serial.print(value_z);
  } else {
    value_z = 0;
  }
  yaw_control = -value_x/half_width;
  Serial.print("\nyaw_control=");
  Serial.print(yaw_control);
  height_control = value_z/half_height;
  Serial.print("\nheight_control=");
  Serial.print(height_control);
  //*************************************

  // addFeedback(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, abz);
  addFeedback(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, &yaw_control, &height_control);
  Serial.print("\nctz=");
  Serial.println(ctz);
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


// Joystick loop()
// void loop() {
//   //gyro, acc, mag, euler, z
//   float cfx, cfy, cfz, ctx, cty, ctz;

//   //*************************************
//   // Getting messages from OpenMV
//   IBus.loop();
//   int cx = IBus.readChannel(0);
//   int half_width = IBus.readChannel(1);
//   Serial.print("\nCh1: cx=");
//   Serial.print(cx);
//   Serial.print("\nCh2: frame=");
//   Serial.print(half_width);
//   float xPositionOfBalloon = ((float)cx)/((float)half_width);
//   Serial.print("\nxPositionOfBalloon=");
//   Serial.print(xPositionOfBalloon);
//   delay(20);
//   //*************************************

//   if (joy_ready && joy_data[7] != 0) {
//     servo1.write((int) (90));
//     servo2.write((int) (90));
//     thrust1.writeMicroseconds(1000);
//     //delay(5);
//     thrust2.writeMicroseconds(1000);
//     //delay(5);//unpack
//   } else if (joy_ready && millis() - time_now <= 1000) { //&& millis() - time_loop > 50) {
//     // sensorSuite.sensfusionLoop(false, 4);
//     // getSensorValues();
//     time_loop = millis();

//     //time_now = millis();// comment out when using a joystick controller
//     getControllerInputs(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, &abz);
// //     Serial.print(cfx);
// //     Serial.print(",");
// //     Serial.print(cfy);
// //     Serial.print(",");
// //     Serial.print(cfz);
// //     Serial.print(",");
// //     Serial.print(ctx);
// //     Serial.print(",");
// //     Serial.print(cty);
// //     Serial.print(",");
// //     Serial.print(ctz);
// //     Serial.print(",");
// //     Serial.println(abz);
//     addFeedback(&cfx, &cfy, &cfz, &ctx, &cty, &ctz, abz);
//     controlOutputs(cfx, cfy, cfz, ctx, cty, ctz);

//     servo1.write((int) (s1*180));
//     servo2.write((int) ((1-s2)*180));
    
//     thrust1.writeMicroseconds(1100 + 400*m1);
//     //delay(5);
//     thrust2.writeMicroseconds(1100 + 400*m2);
//     //delay(5);
//     //analogWrite(THRUST1, (int) (m1*255));
//     //analogWrite(THRUST2, (int) (m2*255));
// //
// //     Serial.print((int) (s1*180));
// //     Serial.print(",");
// //     Serial.print((int) (s2*180));
// //     Serial.print(",");
// //     Serial.print(m1);
// //     Serial.print(",");
// //     Serial.println(m2);Serial.print((int) (s1*180));
//   } else {
//     servo1.write((int) (90));
//     servo2.write((int) (90));
//     thrust1.writeMicroseconds(1000);
//     //delay(5);
//     thrust2.writeMicroseconds(1000);
//     //delay(5);
//   }
// //  send_udp_feedback();
// }

// void getSensorValues() { //all in radians or meters or meters per second
//   roll = sensorSuite.getRoll() -5*PI/180;
//   pitch = 1*sensorSuite.getPitch()+8*PI/180;
//   yaw = sensorSuite.getYaw();
//   rollrate = sensorSuite.getRollRate();
//   pitchrate = sensorSuite.getPitchRate();
//   yawrate = sensorSuite.getYawRate();
//   yawrateave = yawrateave * .95 + yawrate * .05;
//   estimatedZ = sensorSuite.returnZ();
//   velocityZ = sensorSuite.returnVZ(); 
// }

// float valtz = 0;
// void getControllerInputs(float *fx, float *fy, float *fz, float *tx, float *ty, float *tz, float *abz) {
//   if (true) {
//     *fx = 0;//joy_data[0];
//     *fy = 0;//joy_data[1];
//     *fz = 0;//joy_data[2];
//     *tx = 0;//joy_data[3];
//     *ty = 0;//joy_data[4];
//     *tz = 0;//joy_data[5];
//     *abz = 0;//joy_data[6];
//   } else {
//     *fx = joy_data[0];
//     *fy = joy_data[1];
//     *fz = joy_data[2];
//     *tx = joy_data[3];
//     *ty = joy_data[4];
//     *tz = joy_data[5];
//     *abz = joy_data[6];
//   }
// }

void addFeedback(float *fx, float *fy, float *fz, float *tx, float *ty, float *tz, float *yaw_control, float *height_control) {
  // height control
  *fz = *fz; //+ *height_control * kpz 
  // yaw control
  *tz = *yaw_control * kptz; //- yawrateave*kdtz;
  
  //*tx = *tx - roll* kptx - rollrate *kdtx;

  // float cosp = (float) cos(pitch);
  // float sinp = (float) sin(pitch);
  // float cosr = (float) cos(roll);
  // float ifx = *fx;
  // // sinr = (float) sin(self->roll);
  // *fx = ifx*cosp + *fz*sinp; // p stands for pitch
  // //float tfy = fy*cosp/2 + tempz*sinp/2;
  // *fz = (ifx*sinp + *fz* cosp)/cosr;
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


void unpack_joystick(float *dat, const unsigned char *buffer) {
  int num_floats = 8;
  int num_bytes = 4;
  int i, j;

  for(i = 0; i < num_floats; i++) {
    char temp[4] = {0, 0, 0, 0};
    for(j = 0; j < num_bytes; j++) {
      temp[j] = buffer[4*i + j];
    }
    dat[i] = *((float*) temp);
    // if(i == 1 || i == 3){
    //   dat[i] = -*((float*) temp);
    // } else {
    //   dat[i] = *((float*) temp);
    // }
  }
}

// send udp feedback on roll, pitch, and yaw
// void send_udp_feedback() { //const unsigned char *buffer
//   int num_floats = 4;
//   int num_bytes = 4;
//   float dat[4] = {roll, pitch, yaw, yawrate};
//   int i, j;
//   /*
//   for (i = 0; i < num_floats; i++){
//     char temp[4] = {0, 0, 0, 0};
//     for (j = 0; j < num_bytes; j++){
//       temp[j] = buffer[4*i + j];
//     }
//     dat[i] = *((float*) temp);
//   }
//   */
//   String blimp_feedback = String("");
//   blimp_feedback = String((float)roll) + String(", ") + String((float)pitch) + String(", ") + String((float)yaw + String(", ") + String((float)yawrate));
  
//   udp.broadcastTo(blimp_feedback.c_str(), 1333);
// }
