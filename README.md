# Datalogger_lowcost_ATmega
Registrador de datos de bajo coste autónomo para monitorización de sistemas fotovoltaicos híbridos(es más parecido a una estación meteorológica, con lo cual se utiliza para medir el ambiente en cualquier sitio que se desee, pero en este trabajo se ha utilizado para tal fin).

Este registrador de datos se ha programado en C++ mediante el IDE de Arduino y consta de varios shields como GPRS, lector de tarjetas microSD, reloj RTC DS3231, conversor analógico digital, sensores de temperatura y humedad relativa, pluviómetro y anemómetro y por último el ATmega1284p-pu.

Está diseñado para medir el ambiente en campos solares, por lo que su alimentación estará sustentada por una batería que se carga a través de un módulo solar independiente. Esta batería se calcula en función del consumo total en el ciclo de medidas del registrador, y en función de esto según las necesidades que tengamos. Este presente proyecto se calcula para que la batería sea capaz de alimentarlo durante 7 días de ausencia solar. Otro aspecto importante es el regulador de carga solar, pues será el encargado de tener los márgenes para iniciar el proceso de carga y evitar que reduzca su carga por debajo del 80% de su voltaje nominal y con esto lograr un mayor periodo de vida útil de la misma.

Para realizar la medida del consumo del proyecto se ha utilizado el sensor de corriente digital INA219B.

Este trabajo se apoya de unas aplicaciones en php que son las encargadas de recibir los datos mediante peticiones GET, y son almacenados en un fichero CSV para después ser reenviados a la web de gestión de contenido emonCMS, para su posterior visionado en gráficas. Cabe decir que se podría haber evitado el diseño de estas aplicaciones de recogida de datos, pero se hace para tener una programación más sencilla en registrador y además tener un backup de los datos en el servidor web, ya que en primera instancia se almacenan en la tarjeta microSD. El servidor también aloja aplicaciones que comprueban la existencia del fichero para actuar en consecuencia y otra app que devuelve la hora a nuestro datalogger y con ello fijarla en el RTC, consiguiendo con esto no necesitar de usar una pila para guardar la hora del reloj en caso de quedarse sin alimentación nuestro ATmega, ya que sincroniza la hora cada vez que se inicia de una forma tipo al protocolo NTP(Network time protocol) utilizado por cualquier PC, smartphone, tablet, etc para sincronizar la hora automáticamente desde internet.

El microcontrolador tiene grabado el bootloader optiboot, ya que hace uso del watchdog para evitar un posible cuelgue del mismo, debido a que su emplazamiento teóricamente estaría en un lugar lejos de nuestra vivienda.

Este proyecto ha intentado minimizar los gastos de todo el hardware y reducir el consumo al máximo para optimizar la autonomía del mismo, ya que un menor consumo, reduce los gastos en los componentes de mayor valor, que en este caso serán la batería y el módulo solar.

----------------------------------------------------------------------------------------------------------------------------------------

Voy a tratar de explicar el funcionamiento general del programa del micro. Antes de hacerlo pido disculpas si hay cosas que carecen de entendimiento por no explicarme de forma correcta o por cometer alguna errata.

El programa utiliza librerías para watchdog, timeronethree para interrupciones software, softwareserial para añadir puertos serie adicionales(para GSM/GPRS SIM800L), librerías para el ADC MCP3424, para el lector SD/SPI, librería WIRE/DS3231 para reloj RTC, DHT22 para sensor de temperatura/humedad relativa y OneWire/DallasTemperature para sensor de temperatura DS18B20.

Una vez instanciadas las librerías y variables globales, hay que tener en cuenta definir las variables que usa la función de una interrupción como tipo "volatile".

Una vez entra al SETUP, se inician las comunicaciones Serial, se inician los pines I/O, se activa el watchdog, las interrupciones hardware(el ATmega1284p tiene 3, una para anemómetro, pluviómetro y otra para reloj RTC DS3231, encargado de disparar el proceso de medida) y una interrupción por software utilizada para reiniciar el reloj del watchdog ya que si no lo hacemos, no funcionaría correctamente. También se inicia la comunicación de los sensores ds18b20, dht22, el reloj y su salida SQW(esta ocupa otra interrupción hardware, es la encargada de disparar el proceso de medida, a modo de alarma) y se fijan sus alarmas, las cuales se disparan cuando nosotros se lo indiquemos, se fija la hora en el reloj, se inicia el lector microSD, se leen los litros que se han podido perder de las variables tipo volatile en caso de que el micro pierda su alimentación, ya que por elección propia, la pluviometría toma su valor cada 12 horas y por último, se comprueba si el fichero de los datos que se crea en el servidor existe.

Esta comprobación se realiza debido al tipo de estructuración que tiene el fichero donde se almacenan los datos. Los ficheros CSV, si se exportan a excel, utilizan la primera línea para indicar los nombres de las columnas, y el resto de filas, son los datos. Debido a esto hay una casuistica que debe ser cubierta por la programación del micro(más sencilla) y la app del servidor que recibe los datos(más compleja), como por ejemplo si el fichero no existe en el servidor y sí en la tarjeta SD, debe leer la primera línea de este y enviarla al servidor para crearlo, ya que será la línea de nombres o los sensores que tenemos y a continuación realizar las mediciones. No voy a explicar toda la casuística completa para no extenderme en este documento, en caso de querer la explicación completa, preguntar en el foro.



