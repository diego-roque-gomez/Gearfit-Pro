#include <math.h>
#include <Servo.h>
#include <Wire.h>
#include <VL53L1X.h>

#define XSHUT_1 2
#define XSHUT_2 3
#define XSHUT_3 4
#define XSHUT_5 7
#define XSHUT_6 8
#define SERVO_PIN 6
#define BOTON_C5 12
#define BOTON_S3 13
#define B_SUBIR A1
#define B_MEDIR A2
#define B_SPORT A3
#define B_CONFORT 11
#define B_CUSTOM 5

const int IN1 = 9;
const int IN2 = 10;

VL53L1X sensor1;
VL53L1X sensor2;
VL53L1X sensor3;
VL53L1X sensor5;
VL53L1X sensor6;
Servo servo;

int estadoMenu = 0;      // 0=casco, 1=tipo, 2=accion
int cascoSel = 0;        // 1=c5, 2=s3
int tipoSel = 0;         // 1=sport,2=confort,3=custom

unsigned long tLast = 0;
const unsigned long DEBOUNCE = 60;
int lastC5=0,lastS3=0,lastSport=0,lastConfort=0,lastCustom=0,lastSubir=0,lastMedir=0;

const float radio = 29;
const float cascoM = 10.42;
const float cascoXL = 11.05;
float pad_mejilla = 0;
float pad_mej = 0;
float mej1;
float mej2;
int distancia;

bool risingEdge(int lectura, int &last)
{
  bool edge = (lectura == HIGH && last == LOW);
  last = lectura;
  return edge;
}

void setup(){
  Serial.begin(115200);
  Wire.begin();

  pinMode(BOTON_C5,INPUT);
  pinMode(BOTON_S3,INPUT);
  pinMode(B_SPORT,INPUT);
  pinMode(B_CONFORT,INPUT);
  pinMode(B_CUSTOM,INPUT);
  pinMode(B_SUBIR,INPUT);
  pinMode(B_MEDIR,INPUT);
  pinMode(A0,INPUT);

  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);

  pinMode(XSHUT_1,OUTPUT);
  pinMode(XSHUT_2,OUTPUT);
  pinMode(XSHUT_3,OUTPUT);
  pinMode(XSHUT_5,OUTPUT);
  pinMode(XSHUT_6,OUTPUT);

  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);

  servo.attach(SERVO_PIN);
  servo.write(0);

  digitalWrite(XSHUT_1,LOW);
  digitalWrite(XSHUT_2,LOW);
  digitalWrite(XSHUT_3,LOW);
  digitalWrite(XSHUT_5,LOW);
  digitalWrite(XSHUT_6,LOW);
  delay(10);

  digitalWrite(XSHUT_1,HIGH); delay(10); sensor1.init(); sensor1.setAddress(0x30);
  digitalWrite(XSHUT_2,HIGH); delay(10); sensor2.init(); sensor2.setAddress(0x31);
  digitalWrite(XSHUT_3,HIGH); delay(10); sensor3.init(); sensor3.setAddress(0x32);
  digitalWrite(XSHUT_5,HIGH); delay(10); sensor5.init(); sensor5.setAddress(0x34);
  digitalWrite(XSHUT_6,HIGH); delay(10); sensor6.init(); sensor6.setAddress(0x35);

  sensor1.startContinuous(20);
  sensor2.startContinuous(20);
  sensor3.startContinuous(20);
  sensor5.startContinuous(20);
  sensor6.startContinuous(20);

  Serial.println("Seleccione casco: C5 o S3");
}

void loop()
{
  if (millis() - tLast < DEBOUNCE) 
  {
    return;
  }
  tLast = millis();

  int c5 = digitalRead(BOTON_C5);
  int cs3 = digitalRead(BOTON_S3);
  int sp = digitalRead(B_SPORT);
  int co = digitalRead(B_CONFORT);
  int cu = digitalRead(B_CUSTOM);
  int sb = digitalRead(B_SUBIR);
  int md = digitalRead(B_MEDIR);

  if(estadoMenu == 0)
  {
    if(risingEdge(c5, lastC5))
    {
      cascoSel=1;
      estadoMenu=1;
      mej1 = 25;
      mej2 = 10;
      Serial.println("Casco C5");
      Serial.println("Seleccione tipo: Sport, Confort o Custom");
    }
    if(risingEdge(cs3, lastS3))
    {
      cascoSel=2;
      estadoMenu=1;
      mej1 = 35;
      mej2 = 20;
      Serial.println("Casco S3");
      Serial.println("Seleccione tipo: Sport, Confort o Custom");
    }
  }

  else if(estadoMenu==1)
  {
    if(risingEdge(sp, lastSport))
    {
      tipoSel=1;
      estadoMenu=2;
      Serial.println("Tipo Sport seleccionado");
      Serial.println("Seleccione: SUBIR o MEDIR");
    }
    if(risingEdge(cu, lastCustom))
    {
      tipoSel=3;
      estadoMenu=2;
      Serial.println("Tipo Custom seleccionado");
      Serial.println("Seleccione: SUBIR o MEDIR");
    }
  }

  else if(estadoMenu==2)
  {
    if(risingEdge(sb, lastSubir))
    {
      Serial.println("SUBIR...");
      subirMotor();
    }

    if(risingEdge(md, lastMedir))
    {
      Serial.println("MIDIENDO...");

      for (int i = 0; i <= 50 ; i++)
      {
        digitalWrite(IN1,HIGH);
        digitalWrite(IN2,LOW);
        distancia = 12.432 * pow(0.004887585533 * analogRead(A0), -1.058);
        if (distancia <= 5)
        {
          medirProceso();
          break;
        }
        delay(200);
      }

      if (tipoSel == 1) 
      {
        pad_mej = mej1; 
      } 
      else if (tipoSel == 3) 
      {
        pad_mej = pad_mejilla; 
      }

      Serial.println("Almohadilla de la mejilla: " + String(pad_mej) + " mm");
      delay(500);
  
      estadoMenu = 0;
      cascoSel = 0;
      tipoSel = 0;

      lastC5 = lastS3 = lastSport = lastConfort = lastCustom = lastSubir = lastMedir = 0;

      Serial.println("----- FIN CICLO -----");
      Serial.println("Seleccione casco: C5 o S3");
    }
  }
}

void subirMotor()
{
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  delay(800);
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);

  Serial.println("Motor arriba.");
}

void medirProceso()
{
    float craneo = 0;
    float arco1 = 0;
    float arco2 = 0;
    float s1=0,s2=0;
    float m1=0,m2=0,m3=0,m4=0,m5=0,m7=0,m8=0;
    float pad_frontal = 0;
    float pad_lateral_der = 0;
    float pad_lateral_izq = 0;
    float pad_trasero = 0;
    float talla = 0;

    digitalWrite(IN1,LOW);
    digitalWrite(IN2,LOW);

    Serial.println("Brazo en posicion.");

    for(int pos=0;pos<=179;pos++)
    {
      servo.write(pos);

      float d1=sensor1.read();
      float d2=sensor2.read();
      float d3=sensor3.read();
      float d5=sensor5.read();
      float d6=sensor6.read();
      arco1 = (radio - d1/10)*0.01745329252;
      arco2 = (radio - d2/10)*0.01745329252;
      s1 = s1 + arco1;
      s2 = s2 + arco2;
      talla = s1 + s2;
      delay(15);
      if(pos == 45)
      {
        m1 = radio - d1/10;
        m3 = radio - d2/10;
        m5 = radio - d3/10;
        m7 = radio - d5/10;
        m8 = radio - d6/10;
        pad_mejilla = (m5 + m7 + m8 + m5)/4;
      }
      if(pos==135)
      {
        m2 = radio - d1/10;
        m4 = radio - d2/10;
      }
    }

    craneo = ((m1 + m3)/(m2 + m4))*100;
    Serial.print("Talla: ");
    Serial.println(talla);
    if (craneo < 70) 
    {
      Serial.println("Tipo de craneo: Extremadamente ovalado");
    }
    else if (craneo >= 70 && craneo < 75)
    {
      Serial.println("Tipo de craneo: Ovalado");
    } 
    else if (craneo >= 75 && craneo < 81) 
    {
      Serial.println("Tipo de craneo: Normal");
    }
    else if (craneo >= 81 && craneo < 86) 
    {
      Serial.println("Tipo de craneo: Redondo");
    }
    else if (craneo >= 86)
    {
      Serial.println("Tipo de craneo: Extremadamente redondo");
    }

    if (talla >= 52 && talla < 53.5)
    {
      Serial.println("Talla: XS");
    }
    else if (talla >= 53.5 && talla < 55.5)
    {
      Serial.println("Talla: S");
    } 
    else if (talla >= 55.5 && talla < 57.5) 
    {
      pad_lateral_der = cascoM - m1;
      pad_lateral_izq = cascoM - m3;
      pad_trasero = cascoM - m2;
      pad_frontal = cascoM - m4;
      Serial.println("Talla: M");
      Serial.println("Almohadilla frontal: " + String(pad_frontal) + " mm");
      Serial.println("Almohadilla lateral derecha: " + String(pad_lateral_der) + " mm");
      Serial.println("Almohadilla lateral izquierda: " + String(pad_lateral_izq) + " mm");
      Serial.println("Almohadilla trasera: " + String(pad_trasero) + " mm");
    }
    else if (talla >= 57.5 && talla < 59.5) 
    {
      pad_lateral_der = cascoM - m1;
      pad_lateral_izq = cascoM - m3;
      pad_trasero = cascoM - m2;
      pad_frontal = cascoM - m4;
      Serial.println("Talla: L");
      Serial.println("Almohadilla frontal: " + String(pad_frontal) + " mm");
      Serial.println("Almohadilla lateral derecha: " + String(pad_lateral_der) + " mm");
      Serial.println("Almohadilla lateral izquierda: " + String(pad_lateral_izq) + " mm");
      Serial.println("Almohadilla trasera: " + String(pad_trasero) + " mm");
    }
    else if (talla >= 59.5 && talla < 61.5) 
    {
      pad_lateral_der = cascoXL - m1;
      pad_lateral_izq = cascoXL - m3;
      pad_trasero = cascoXL - m2;
      pad_frontal = cascoXL - m4;
      Serial.println("Talla: XL");
      Serial.println("Almohadilla frontal: " + String(pad_frontal) + " mm");
      Serial.println("Almohadilla lateral derecha: " + String(pad_lateral_der) + " mm");
      Serial.println("Almohadilla lateral izquierda: " + String(pad_lateral_izq) + " mm");
      Serial.println("Almohadilla trasera: " + String(pad_trasero) + " mm");
    }
    else if (talla >= 61.5 && talla < 63.5) 
    {
      Serial.println("Talla: XXL");
    }
    else if (talla >= 63.5 && talla < 65.5) 
    {
      Serial.println("Talla: 3XL");
    }
    else
    {
      Serial.println("Fuera de rango");
    }

    for (int pos_regreso = 179; pos_regreso >= 0; pos_regreso--)
    {
      servo.write(pos_regreso);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      delay(15);
      if (pos_regreso == 0)
      {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        break;
      }
    }
}