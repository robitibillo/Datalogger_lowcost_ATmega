#include <avr/wdt.h>
//wdt_disable();
//wdt_reset();
/*
WDTO_15MS
WDTO_30MS
WDTO_60MS
WDTO_120MS
WDTO_250MS
WDTO_500MS
WDTO_1S
WDTO_2S
WDTO_4S
WDTO_8S
*/
#include <TimerOneThree.h>

#include <SoftwareSerial.h>
SoftwareSerial sim800(0,1);//RX,TX
#define rstpinSIM800L 3

//Libreria bajo consumo Jeelib
#include <JeeLib.h> // sensor library from https://github.com/jcw/jeelib
ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

//Librerias para ADC MCP3424
#include "MCP3424.h"
// Configuration variables for MCP3424
byte address = 0x69; // address of this MCP3424. For setting address see datasheet
byte gain = 0; // gain = x1
byte resolution = 3; // resolution = 18bits, 3SPS
MCP3424 ADC1(address, gain, resolution); // create MCP3424 instance.

//Librerias SD
#include <SD.h>
#include <SPI.h>
File Archivo;
File litrosfile;
#define SDPIN 4
unsigned long pos;//Indica la posicion del apuntador justo al terminar de escribir la linea de datos.
struct datosHTTP
   {   String row="";
       String nom="";
   };
datosHTTP valor;

//Librerias Reloj RTC y puerto I2C
#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
RTCDateTime dt;
boolean isAlarm = false;
String str,anio,mes,dia,hora,minuto,segundo;

//Librerias DHT22
#define DHT22_PIN 18
#define DHT_VER DHT22
#include "DHT.h" //cargamos la librería DHT
DHT dht(DHT22_PIN,DHT_VER); //Se inicia una variable que será usada por Arduino para comunicarse con el sensor

//Librerias DS18B20
#include <OneWire.h> //Se importan las librerías
#include <DallasTemperature.h>
#define OneWirePin 19
OneWire ourWire(OneWirePin); //Se establece el pin declarado como bus para la comunicación OneWire
DallasTemperature ds18b20(&ourWire); //Se instancia la librería DallasTemperature

//Anemometro y Pluviometro
volatile int contapluvi = 0;
String litrosSD;
int litros = contapluvi;
long timepluvi = 0;
volatile int contaane = 0;
long timeane = 0 ;
byte flag=0;
volatile byte timernum=0;

//Pin para reiniciar arduino en caso de recibir errores en el envío de los datos del módulo GPRS
//int resetPin = 15;
byte numerrors=0;

void setup(){
  /*
  pinMode(15,OUTPUT);
  digitalWrite(15,HIGH);
  */
  Serial.begin(115200);
  pinMode(rstpinSIM800L, OUTPUT);
  
  wdt_enable(WDTO_8S);
  Timer3.initialize(2000000);
  Timer3.attachInterrupt(resetWatchDog);
  
  rstSIM800L();
  clock.begin();
  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.enable32kHz(false);
  clock.enableOutput(false);
  
  // Set sketch compiling time
  //clock.setDateTime(__DATE__, __TIME__);
  //clock.setDateTime(2016, 8, 23, 15, 19, 30);
  fijarHora();
    
  clock.setAlarm1(0, 0, 0, 30, DS3231_MATCH_S);
  clock.setAlarm2(0, 0, 0,  DS3231_EVERY_MINUTE);
  
  dht.begin(); //Se inicia el sensor
  ds18b20.begin();
  pinMode(10,INPUT_PULLUP);
  pinMode(11,INPUT_PULLUP);
  pinMode(2,INPUT_PULLUP);
  attachInterrupt( 0, ane, FALLING);//Esta interrupción hardware corresponde al pin 10 ATMega1284p
  attachInterrupt( 1, pluvi, FALLING);//Esta interrupción hardware corresponde al pin 11 ATMega1284p
  attachInterrupt( 2, alarm, FALLING);//Esta interrupción hardware corresponde al pin 2 ATMega1284p
  iniciaSD();
  leerlitros();
  existe();
}
//http://tfgvcr.mipropia.com/
void resetWatchDog()
   {   
     timernum++;
     if(timernum==30){
     wdt_enable(WDTO_15MS);
     delay(100);
     }
     //Serial.println("Timer interrupt");
     wdt_reset();  
   }

void rstSIM800L(){
  digitalWrite(rstpinSIM800L, HIGH);
  delay(10);
  digitalWrite(rstpinSIM800L, LOW);
  delay(100);
  digitalWrite(rstpinSIM800L, HIGH);
  /*
  delay(100);
  Sleepy::loseSomeTime(14900);
  */
  delay(100);
  Sleepy::loseSomeTime(5000);
  wdt_reset();
  Sleepy::loseSomeTime(5000);
  wdt_reset();
  Sleepy::loseSomeTime(2400);
  wdt_reset();
  sim800.begin(115200);//Esto se ha hecho porque al reiniciar el módulo, por defecto inicia con un baudrate fijo a 115200, una marcado, en la siguiente instrucción indicamos el baudrate de trabajo que queremos; 
  sim800.println("AT+IPR=57600"); //indicamos al módulo GSM el baudrate que queremos
  sim800.flush();//Una vez cambiado el baudrate del módulo GSM, liberamos el buffer serie
  delay(2);
  sim800.end();//Con esto finalizamos el bus y lo reiniciamos a que esos pines utilizados para serie tengan un uso normal
  sim800.begin(57600);//Volvemos a inicializar la comunicación serie a la velocidad fijada en el módulo GSM
  sim800.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  disponible();
  sim800.println("AT+SAPBR=3,1,\"APN\",\"lowi.private.omv.es\"");
  disponible();
  sim800.println("AT+SAPBR=1,1");//Levantamos la conexion GPRS(Imprescindible)
  wait();
  sim800.println("AT+HTTPINIT");
  disponible();
  sim800.flush();
  /*
  delay(100);
  Sleepy::loseSomeTime(14900);
  */
  delay(100);
  Sleepy::loseSomeTime(5000);
  wdt_reset();
  Sleepy::loseSomeTime(5000);
  wdt_reset();
  Sleepy::loseSomeTime(2400);
  wdt_reset();
}

void iniciaSD(){
if (!SD.begin(SDPIN)){
  pinMode(SDPIN, OUTPUT);
  }
}

void leerlitros(){
  char c[5];
  byte i=0;
  if (!SD.exists("litros.txt")){
  litrosfile = SD.open("litros.txt", FILE_WRITE);
    if (litrosfile){
     litrosfile.print('0');
     litrosfile.close();
  }else{
    Serial.println("El archivo litros.txt no se abrio correctamente");
  }
}else{
    litrosfile = SD.open("litros.txt");
    if (litrosfile){
    while (litrosfile.available()){
     c[i]=litrosfile.read();
     i++;
     }
     litros=atoi(c);
     litrosfile.close();
  }else{
    Serial.println("El archivo litros.txt no se abrio correctamente");
  }
 }
 contapluvi=litros;
}

void escribirlitros(){
  SD.remove("litros.txt");
  litrosfile = SD.open("litros.txt", FILE_WRITE);
    if (litrosfile){
     litrosfile.print(litros);
     litrosfile.close();
  }else{
    Serial.println("El archivo litros.txt no se abrio correctamente");
  }
}

void existe(){
if (SD.exists("datos.csv")){
sim800.println("AT+HTTPPARA=\"CID\",1");
disponible();
sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.esy.es/existe.php\"");
disponible();
sim800.println("AT+HTTPACTION=0");//Envia los datos al servidor
wait();
sim800.println("AT+HTTPREAD");
  delay(1000);
  String response="";
  while(sim800.available()){
  response = sim800.readString();
  }
  String res=response.substring(28,response.length()-6);
  if (res=="SI" || res=="NO"){
  if(res=="NO"){
  char letra;
  //Se vuelve a abrir el fichero, esta vez para leer los datos escritos.
  Archivo = SD.open("datos.csv");
  //Si el archivo se ha abierto correctamente se muestran los datos.
  if (Archivo){
    Archivo.seek(0);
    while(Archivo.available()){
    if( Archivo.peek() == '\n' ){goto end;}
    letra=Archivo.read();
    valor.nom+=String(letra);
    }
    end:
    Archivo.close();
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  sim800.println("AT+HTTPPARA=\"CID\",1");
  disponible();
  sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.esy.es/datos.php?datos="+valor.nom+"\"");
  disponible();
  sim800.println("AT+HTTPACTION=0");//Envia los datos al servidor
  respuesta();
  sim800.println("AT+CSCLK=2");
  disponible();
  valor.nom="";
  }
  }else{
  wdt_enable(WDTO_15MS);
  delay(100);  
  /*
  digitalWrite(resetPin, LOW);
  delay(200);
  pinMode(resetPin, OUTPUT);
  */
  }
  response="";
  res="";
}
}

void uploaddata(){
  sim800.println("AT"); //check AT
  disponible();
  /*sim800.println("AT+CPIN=*******"); //Introducimos el PIN de la SIM si tuviera
  disponible();*/
  /*sim800.println("AT+CREG?");//Comprobamos la conexion a la red
  disponible();
  sim800.println("AT+CSQ");
  disponible();
  sim800.println("AT+CGATT?");
  disponible();
  sim800.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  disponible();
  sim800.println("AT+SAPBR=3,1,\"APN\",\"internetmas\"");
  disponible();*/
  /*sim800.println("AT+SAPBR=1,1");//Levantamos la conexion GPRS(Imprescindible)
  disponible();
  /*sim800.println("AT+SAPBR=2,1");//Comprueba si la estación base a cogido ip en la conexion GPRS
  disponible();*/
  /*sim800.println("AT+HTTPINIT");
  disponible();*/
  sim800.println("AT+HTTPPARA=\"CID\",1");
  disponible();
  sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.esy.es/datos.php?datos="+valor.row+"\"");
  //sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.esy.es/datos.php?datos="+valor.row+"&nombres="+valor.nom+"\"");
  //sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.ddns.net/datos.php?datos="+valor.row+"&nombres="+valor.nom+"\"");
  disponible();
  sim800.println("AT+HTTPACTION=0");//Envia los datos al servidor
  respuesta();
  /*sim800.println("AT+HTTPREAD");
  disponible();*/
  /*sim800.println("AT+HTTPTERM");
  disponible();
  sim800.println("AT+CIPSHUT");//Se cierra la conexion mientras no se utiliza
  disponible();*/
  /*sim800.println("AT+SAPBR=0,1");
  disponible();
  sim800.println("AT+CGATT=0");
  disponible();*/
  sim800.println("AT+CSCLK=2");
  disponible();
  valor.row="";
}

/*Esta funcion identifica cualquier dispositivo conectado al bus 1-Wire
no tiene por que ser solo para el sensor de temperatura DS18B20, pero
en este caso está adaptada para dar sus id*/
void idDS18B20() {
  byte numsensor=0;
  byte i;
  byte addr[8];  
  String DatosHEX[8];
  while(ourWire.search(addr)) {
    for( i = 0; i < 8; i++) {
      DatosHEX[i]=String(addr[i],HEX);      
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
        numsensor=0;
        Serial.print("CRC is not valid!\n");
        return;
    }
  numsensor++;
  String numerosensores="S" + String(numsensor);
  Archivo.print(numerosensores + "_");
      if (DatosHEX[7].length() == 1) {
       DatosHEX[7]="0" + DatosHEX[7];       
      }
      DatosHEX[7]="0x" + DatosHEX[7];
  Archivo.print(DatosHEX[7]+",");
}
  ourWire.reset_search();
return;
}

void names(){
//              ESCRIBIENDO DATOS EN LA MEMORIA SD DE ARDUINO
  //Se abre el documento sobre el que se va a leer y escribir.
  Archivo = SD.open("datos.csv", FILE_WRITE);
  pos=Archivo.position();//Se utiliza para almacenar la posicion donde comienza a escribir para después el apuntador 
                         //comenzar a leer en la misma posición que comenzó la escritura. Con esto, seremos 
                         //capaces de leer solamente la ultima linea que contiene los nuevos datos.
  
  //Se comprueba que el archivo se ha abierto correctamente y se procede a escribir en él.
  if (Archivo){
    
    //Se escribe información en el documento de texto datos.csv.
    Archivo.print("Fecha,(DHT22)Humedad,Temperatura,(DS18B20)_Temperatura_");
    idDS18B20();
    Archivo.print("MCP3424(CH1),(CH2),(CH3),(CH4),");
    Archivo.print("Anemometro(m/s),");
    Archivo.print("Pluviometro(L/m2_dia)");
    Archivo.println(",");
    //Se cierra el archivo para almacenar los datos.
    Archivo.close();
  }
  //            FIN DE LA ESCRITURA DE DATOS EN LA MEMORIA SD DE ARDUINO
}

void leerultimalinea(){
  //  LEYENDO DATOS EN LA MEMORIA SD DE ARDUINO
  char letra;
  //Se vuelve a abrir el fichero, esta vez para leer los datos escritos.
  Archivo = SD.open("datos.csv");
  
  //Si el archivo se ha abierto correctamente se muestran los datos.
  if (Archivo){
    Archivo.seek(pos);//Posición donde comienza a leer el apuntador
    //Se implementa un bucle que recorrerá el archivo hasta que no encuentre más información (Archivo.available()==FALSE).
    while (Archivo.available()){
     //Se escribe la información que ha sido leída del archivo.
     letra=Archivo.read();
     valor.row+=String(letra);
     //row+=String(letra);
    }
    /*
    Archivo.seek(0);
    while(Archivo.available()){
    if( Archivo.peek() == '\n' ){goto end;}
    letra=Archivo.read();
    valor.nom+=String(letra);
    }
    end:
    */
    Archivo.close();
  }
  
  //En caso de que haya habido problemas abriendo datos.csv, se muestra por pantalla.
  else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
}

void leerDHT22(){
//Leyendo datos sensor DHT22  
  float h = dht.readHumidity(); //Se lee la humedad
  float t = dht.readTemperature(); //Se lee la temperatura  

  //              ESCRIBIENDO DATOS EN LA MEMORIA SD DE ARDUINO
  
  //Se abre el documento sobre el que se va a leer y escribir.
  /*
  Archivo = SD.open("datos.csv",FILE_WRITE);
  //pos=Archivo.position();//Se utiliza para almacenar la posicion donde comienza a escribir para después colocar
                         //el apuntador en la misma posición que comenzó la escritura. Con esto, seremos capaces 
                         //de leer solamente la ultima linea que contiene los nuevos datos.
                         //Se comprueba que el archivo se ha abierto correctamente y se procede a escribir en el.
  if (Archivo){
  //Se escribe información en el documento de texto del sensor DHT22
  dt = clock.getDateTime();
  Archivo.print(clock.dateFormat("d-m-Y_H:i:s,", dt));
  */
  Archivo.print(h);
  Archivo.print(",");
  Archivo.print(t);
  Archivo.print(",");
  /*
  //Se cierra el archivo para almacenar los datos.
  Archivo.close();
  }
  
  //En caso de que haya habido problemas abriendo datos.csv, se muestra por pantalla.
  else{

    Serial.println("El archivo de datos no se abrio correctamente");
  }
  */ 
  //            FIN DE LA ESCRITURA DE DATOS EN LA MEMORIA SD DE ARDUINO
}

void leerDS18B20() {
  /*
  //Se abre el documento sobre el que se va a leer y escribir.
  Archivo = SD.open("datos.csv", FILE_WRITE);
  
  //Se comprueba que el archivo se ha abierto correctamente y se procede a escribir en él.
  if (Archivo){
  */  
  byte numsensor=0;
  byte addr[8];//Array que recibe la direccion de cada uno de los sensores del bus OneWire  
  while(ourWire.search(addr)) {
  ds18b20.setResolution(addr, 12);//Le indicamos la resolucion que queremos obtener
  ds18b20.requestTemperatures();//Preparamos al sensor para medir
  float temp=ds18b20.getTempCByIndex(numsensor);
  Archivo.print(temp);
  numsensor++;
  Archivo.print(",");
}
  ourWire.reset_search();
  /*
  Archivo.close();
    
  //Se muestra por el monitor que los datos se han almacenado correctamente.
  }
  
  //En caso de que haya habido problemas abriendo datos.csv, se muestra por pantalla.
  else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  //            FIN DE LA ESCRITURA DE DATOS EN LA MEMORIA SD DE ARDUINO
return;
  */ 
}

void readMCP3424(){
  /*
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo){
  */  
  for(byte i=0;i<4;i++){
  // get channel data with function
  double CH = ADC1.getChannelmV(i);
  Archivo.print(CH);
  Archivo.print(",");
  }
  /*
  Archivo.close();
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  */
 }

void readane() {
  detachInterrupt(digitalPinToInterrupt(10));//Desactiva la interrupcion del pin de arduino(no el numero de interrupcion),
                         //que este caso equivale al pin 10 del ATMega1284p e interrupcion 0
                         /*Realizamos la conversión de nuestro número de clicks(cada click es como pulsar un pulsador).
                         Primero contamos el número de clicks y lo dividimos entre dos, ya que cada dos clicks es una
                         vuelta completa del anemómetro que también es igual a 1Hz. Lo siguiente para realizar el
                         cálculo en mi caso voy a realizar el conteo en un un espacio temporal de 30 segundos y pasado
                         este tiempo resetearemos el contador a 0. Pues bien cuando sabemos el número de vueltas lo
                         divideremos entre el tiempo de conteo y con esto tenemos el número de vueltas(o Hertzios) segundo.
                         Ya por último nos quedaría dividir entre 2.5Hz que equivale a 1m/s según nuestra hoja de
                         características y con esto ya obtendríamos la velocidad.*/
  /*
  float freq = contaane / 2;// Frecuencia o numero de vueltas
  float velm = freq / (30 * 2.5);//Maximo 40m/s(144Km/h).
  float velkm = velm*3.6;
  */
  float velkm = ((contaane)/(2*30*2.5))*3.6;
  /*
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo) {
    */
    Archivo.print(velkm);
    Archivo.print(",");
    /*
    Archivo.close();
  }
  else {

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  */
  contaane = 0;
  attachInterrupt( 0, ane, FALLING);//Activa de nueva la interrupcion 0(Pin 10 ATMega1284p)
}

void readpluvi(){
  /*
  dt = clock.getDateTime();
  if(((dt.hour==12)||(dt.hour==0)) && (dt.minute==0) && (dt.second==0))
  {
  flag++;
  */ 
  detachInterrupt(digitalPinToInterrupt(11));//Desactiva la interrupcion del pin de arduino(no el numero de interrupcion),
                                             //que este caso equivale al pin 11 del ATMega1284p e interrupcion 1
  if(flag!=0){
  /*  
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo){
  */  
  Archivo.print(contapluvi);
  Archivo.println(",");
  contapluvi=0;
  litros=contapluvi;
  escribirlitros();
  flag=0;
  /*
  Archivo.close();
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  */  
  }else{
  /*  
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo){
  */  
  Archivo.print("0");
  Archivo.println(",");
  /*
  Archivo.close();
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }
  */  
  }
  attachInterrupt( 1, pluvi, FALLING);//Activa de nueva la interrupcion 1(Pin 11 ATMega1284p)
  //}  
}

void writeDateTime(){
  /*
  Archivo = SD.open("datos.csv",FILE_WRITE);
  pos=Archivo.position();//Se utiliza para almacenar la posicion donde comienza a escribir para después colocar
                         //el apuntador en la misma posición que comenzó la escritura. Con esto, seremos capaces 
                         //de leer solamente la ultima linea que contiene los nuevos datos.
                         //Se comprueba que el archivo se ha abierto correctamente y se procede a escribir en el.
  if (Archivo){
  */
  dt = clock.getDateTime();
  Archivo.print(clock.dateFormat("d-m-Y_H:i:s,", dt));
  //Se cierra el archivo para almacenar los datos.
  /*
  Archivo.close();
  }
  
  //En caso de que haya habido problemas abriendo datos.csv, se muestra por pantalla.
  else{

    Serial.println("El archivo de datos no se abrio correctamente");
  }
  */
}

void tomarlecturas(){
  iniciaSD();
  litros=contapluvi;
  escribirlitros();
  if(!SD.exists("datos.csv")){
  names();
  }else{
  Archivo = SD.open("datos.csv",FILE_WRITE);
  pos=Archivo.position();//Se utiliza para almacenar la posicion donde comienza a escribir para después colocar
                         //el apuntador en la misma posición que comenzó la escritura. Con esto, seremos capaces 
                         //de leer solamente la ultima linea que contiene los nuevos datos.
                         //Se comprueba que el archivo se ha abierto correctamente y se procede a escribir en el.
  if (Archivo){
  writeDateTime();  
  dt = clock.getDateTime();
  if(((dt.hour==12)||(dt.hour==0)) && (dt.minute==0) && (dt.second==0)){
  flag++;
  }
  leerDHT22();
  leerDS18B20();
  readMCP3424();
  readane();
  readpluvi();
  Archivo.close();
  }
  
  //En caso de que haya habido problemas abriendo datos.csv, se muestra por pantalla.
  else{

    Serial.println("El archivo de datos no se abrio correctamente");
  }
  /*
  if(flag!=0){
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo){  
  Archivo.print(contapluvi);
  Archivo.println(",");
  Archivo.close();
  contapluvi=0;
  flag=0;
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }  
  }else{
  Archivo = SD.open("datos.csv", FILE_WRITE);
  if (Archivo){  
  Archivo.print("0");
  Archivo.println(",");
  Archivo.close();
  }else{

    Serial.println("El archivo datos.csv no se abrio correctamente");
  }  
  }
  */
  }
  leerultimalinea();
  //Serial.println(valor.row);
}

void alarm()
{
  isAlarm = true;
}

void fijarHora(){
sim800.println("AT+HTTPPARA=\"CID\",1");
disponible();
sim800.println("AT+HTTPPARA=\"URL\",\"http://tfgvcr.esy.es/time.php\"");
disponible();
sim800.println("AT+HTTPACTION=0");//Envia los datos al servidor
wait();
sim800.println("AT+HTTPREAD");
espera_hora();
/*
Esta condición, comprueba si la fecha ha sido leída correctamente del servidor
ya que en caso de no ser leído correctamente, el string estará vacío y si esto 
sucede procederemos al reinicio para reintentar de nuevo la sincronización.
*/
String checkanio=anio.substring(0,1);
if(checkanio!="2"){
wdt_enable(WDTO_15MS);
delay(100);
/*  
digitalWrite(resetPin, LOW);
delay(200);
pinMode(resetPin, OUTPUT);
*/
}
clock.setDateTime(anio.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt(), segundo.toInt());
}

void espera_hora(){
  delay(1000);
  while(sim800.available()){
    str = sim800.readString();
  }
  //Serial.println(str);
  anio=str.substring(29,33);
  mes=str.substring(34,36);
  dia=str.substring(37,39);
  hora=str.substring(40,42);
  minuto=str.substring(43,45);
  segundo=str.substring(46,48);
}

void wait(){
  delay(3000);
  while(sim800.available()){
    Serial.write(sim800.read());
  }
}
/*
La función respuesta, lee la respuesta del comando AT+HTTPPARA="CID",1, ya que en caso
de fallo, el módulo GPRS no envía los datos al servidor. En caso de que la respuesta 
sea erronea X veces(en este caso se ha optado por 2 errores consecutivos), el programa
genera un reinicio por hardware del dispositivo.
*/
void respuesta(){
  /*
  unsigned long previous = millis();
  do{
    } while((millis() - previous) < 3000);
  */
  delay(3000);
  String response="";
  while(sim800.available()){
  response = sim800.readString();
  }
  Serial.println(response);
  //Serial.println(response.length());
  String errorres=response.substring(39,response.length()-4);
  /*
  Serial.println("");
  Serial.println(errorres.length());
  Serial.println(errorres);
  */
  if(errorres!="200"){
  numerrors++;
  if(numerrors==2){
  rstSIM800L();
  /*
  wdt_enable(WDTO_15MS);
  delay(100);
  */
  /*  
  digitalWrite(resetPin, LOW);
  delay(200);
  pinMode(resetPin, OUTPUT);
  */
  }
  }else{
  numerrors=0;  
  }
  response="";
  errorres="";
}

void disponible(){
  delay(100);
  while(sim800.available()){
    Serial.write(sim800.read());
  }
}

void loop() {
  dt = clock.getDateTime();
  if(dt.second!=0 || dt.second!=30){
  wdt_reset();
  //Serial.println("Reset WatchDog LooP");
  //delay(10);
  Sleepy::loseSomeTime(5000);
  //Sleepy::powerDown();
  }
  if (isAlarm)
  {
    clock.clearAlarm1();
    clock.clearAlarm2();
    dt = clock.getDateTime();
    if(dt.second==0 || dt.second==30){
    //digitalWrite(15,LOW);
    timernum=0;
    tomarlecturas();
    uploaddata();
    }
  }
}

void pluvi()
{
  if (millis() > timepluvi + 250)
  {
    contapluvi++;
    timepluvi = millis();
  }
}

void ane()
{
  if (millis() > timeane + 5)
  {
    contaane++;
    timeane = millis();
  }
}
