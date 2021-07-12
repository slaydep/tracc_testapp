#include <Encoder.h>
#include <Arduino_FreeRTOS.h>

#include <TB6560.h>
#include <HX711_ADC.h>

//manejadores de tareas
TaskHandle_t control_Handler;
TaskHandle_t test_Handler;
TaskHandle_t bascula_Handler;
TaskHandle_t enco_Handler;

// definiendo tareas
void Control( void *pvParameters );
void Test( void *pvParameters );
void Bascula(void *pvParameters);
void Enco(void *pvParameters);

//definiendo funciones
void calibrate();


//celdas de carga

const int HX711_dout = 4; //mcu > HX711 dout pin ALTA
const int HX711_sck = 5; //mcu > HX711 sck pin ALTA
//const int HX711_dout2 = 6; //mcu > HX711 dout pin BAJA
//const int HX711_sck2 = 7; //mcu > HX711 sck pin BAJA
float calibrationValue = -16.58;
//float calibrationValue2 = 1.0;
HX711_ADC LoadCellalta(HX711_dout, HX711_sck); //carga Alta
//HX711_ADC LoadCellbaja(HX711_dout2, HX711_sck2); //carga Baja
bool bascula = false;

//motor paso
const int CLK = 9;
const int CW = 8;
const int pasos = 1600;
const int EnPin = 10;
int velocidad = 100;
TB6560 mystepper(CLK, CW, pasos);
bool motor = false;
//encoder
const int EncA = 18;
const int EncB = 19;
Encoder myEnc(EncA, EncB);
bool enco = false;

unsigned long t = 0;
bool test = false;
long oldPosition  = -999;

void setup() {

  Serial.begin(9600);
  while (!Serial) {};
  //Serial.println("Inicio setup()..");
  pinMode(EnPin, OUTPUT); //pin Eneable del driver
  digitalWrite(EnPin, HIGH); //pin Eneable del driver en alto para moverlo libremente
  mystepper.setSpeed(velocidad);

  //celdas de carga
  LoadCellalta.begin();
  //LoadCellbaja.begin();

  unsigned long stabilizingtime = 2000;
  LoadCellalta.start(stabilizingtime, true);
  //LoadCellbaja.start(stabilizingtime, false);

  if (LoadCellalta.getTareTimeoutFlag() || LoadCellalta.getSignalTimeoutFlag()) {

    Serial.println("Timeout HX711");

    while (1);
  }
  else {
    LoadCellalta.setCalFactor(calibrationValue); // set calibration value (float)


    Serial.println("setup()...ok");
    //Serial.println("Se inició la celda de carga alta");
  }

  while (!LoadCellalta.update());

  xTaskCreate(Control, "Tarea de Control", 128, NULL, 5, &control_Handler);
  //xTaskCreate(Bascula, "Tarea de bascula", 128, NULL, 3, &bascula_Handler);
}

void loop()
{

  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tareas ---------------------*/
/*--------------------------------------------------*/

void Control(void *pvParameters)  // Tarea de control
{
  (void) pvParameters;
  String inputString;
  while (true) {
    int opc = 0;
    inputString = "";
    if (Serial.available() > 0)
    {
      inputString = Serial.readString();
      opc = inputString.toInt();
      Serial.println(opc);
    }
    if (opc == 1) {
      calibrate();
    }
    if (opc == 2)//switch para activar y seactivar las tareas
    {
      Serial.println("establecer velocidad ");
      while (true) {
        if (Serial.available() > 0)
        {
          inputString = Serial.readString();
          velocidad = inputString.toInt();
          break;
        }
      }
    }
    if (opc == 3)  //switch para activar y seactivar las tareas
    {
      if (!bascula) { //si esta apagada la enciende
        xTaskCreate(Bascula, "Tarea de bascula", 128, NULL, 3, &bascula_Handler);
        bascula = true;
      }
      else { //si no, la apaga
        vTaskDelete(bascula_Handler);
        bascula = false;
      }

    }
    if (opc == 4)//switch para activar y seactivar las tareas
    {
      if (!test) {
        xTaskCreate(Test, "Tarea de Test", 128, NULL, 3, &test_Handler);
        test = true;
      }
      else {
        vTaskDelete(test_Handler);
        test = false;
        digitalWrite(EnPin, HIGH);
      }
    }
    if (opc == 5)//switch para activar y seactivar las tareas
    {
      if (!enco) {
        xTaskCreate(Enco, "Tarea de Encoder", 128, NULL, 5, &enco_Handler);
        enco = true;
      }
      else {
        vTaskDelete(enco_Handler);
        enco = false;
        digitalWrite(EnPin, HIGH);
      }
    }
  }
}


void Bascula(void *pvParameters) // Tarea bascula (para pruebas)
{
  (void) pvParameters;
  int op;
  static boolean newDataReady = false;
  const int serialPrintInterval = 100; //increase value to slow down serial print activity
  t = millis();
  while (true) { 
    // check for new data/start next conversion:
    if (LoadCellalta.update()) newDataReady = true;

    // get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        float i = LoadCellalta.getData();
        Serial.print("Carga: ");
        Serial.println(i);
        newDataReady = false;
        t = millis();
      }
    }
  }
}

void Test(void *pvParameters) // Tarea test
{
  (void) pvParameters;
  int op;
  float i;
  long newPosition;
  t = millis();
  static boolean newDataReady = 0;
  const int serialPrintInterval = 1; //incrementar el valor para disminuir la frecuencia del envio serial
  digitalWrite(EnPin, LOW);
  mystepper.setDirection(0);//sentido de giro del motor
  mystepper.setSpeed(velocidad);
  myEnc.write(0);
  Serial.println("Carga,Deformacion");
  while (true) {
    mystepper.step(5); //numero de pasos por cada loop
    if (LoadCellalta.update()) newDataReady = true;
    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        newPosition = myEnc.read();
        i = LoadCellalta.getData();
        //Serial.print("Carga: ");
        Serial.print(i);
        //Serial.print(" def: ");
        Serial.print(",");
        Serial.println(newPosition);
        newDataReady = false;
        t = millis();
      }
    }
  }
}
void Enco(void *pvParameters) // Tarea encoder
{
  (void) pvParameters;
  myEnc.write(0);
  while (true) {
    long newPosition = myEnc.read();
    if (newPosition != oldPosition) {
      Serial.print("oldPosition: ");
      Serial.print(oldPosition);
      Serial.print(" newPosition: ");
      Serial.println(newPosition);
      oldPosition = newPosition;
    }
  }
}

/*--------------------------------------------------*/
/*--------------------- Funciones-------------------*/
/*--------------------------------------------------*/
void calibrate() {

  Serial.println("***");
  Serial.println("Iniciando calibracion:");
  Serial.println("Coloque la celda de carga en una superficie nivelada y estable.");
  Serial.println("Retire cualquier carga aplicada a la celda de carga.");
  Serial.println("Enviar 't' para establecer el Tara.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCellalta.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCellalta.tareNoDelay();
      }
    }
    if (LoadCellalta.getTareStatus() == true) {
      Serial.println("Tara completo");
      _resume = true;
    }
  }

  Serial.println("Ahora, coloque su masa conocida en la celda de carga.");
  Serial.println("Luego envíe el peso de esta masa (ejemplo, 100.0)");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCellalta.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("La masa es: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCellalta.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCellalta.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("El nuevo valor de calibración se ha establecido en: ");
  Serial.print(newCalibrationValue);
  Serial.println("Fin Calibracion alta");
  Serial.println("***");
}
