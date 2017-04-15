# Datalogger_lowcost_ATmega
Registrador de datos de bajo coste autónomo para monitorización de sistemas fotovoltaicos híbridos(es más parecido a una estación meteorológica, con lo cual se utiliza para medir el ambiente en cualquier sitio que se desee, pero en este trabajo se ha utilizado para tal fin).

Este registrador de datos se ha programado en C++ mediante el IDE de Arduino y consta de varios shields como GPRS, lector de tarjetas microSD, reloj RTC DS3231, conversor analógico digital, sensores de temperatura y humedad relativa, pluviómetro y anemómetro y por último el ATmega1284p-pu.

Está diseñado para medir el ambiente en campos solares, por lo que su alimentación estará sustentada por una batería que se carga a través de un módulo solar independiente. Esta batería se calcula en función del consumo total en el ciclo de medidas del registrador, y en función de esto según las necesidades que tengamos. Este presente proyecto se calcula para que la batería sea capaz de alimentarlo durante 7 días de ausencia solar. Otro aspecto importante es el regulador de carga solar, pues será el encargado de tener los márgenes para iniciar el proceso de carga y evitar que reduzca su carga por debajo del 80% de su voltaje nominal y con esto lograr un mayor periodo de vida útil de la misma.

Para realizar la medida del consumo del proyecto se ha utilizado el sensor de corriente digital INA219B.

Este trabajo se apoya de unas aplicaciones en php que son las encargadas de recibir los datos mediante peticiones GET, y son almacenados en un fichero CSV para después ser reenviados a la web de gestión de contenido emonCMS, para su posterior visionado en gráficas. Cabe decir que se podría haber evitado el diseño de estas aplicaciones de recogida de datos, pero se hace para tener una programación más sencilla en registrador y además tener un backup de los datos en el servidor web, ya que en primera instancia se almacenan en la tarjeta microSD. El servidor también aloja aplicaciones que comprueban la existencia del fichero para actuar en consecuencia y otra app que devuelve la hora a nuestro datalogger y con ello fijarla en el RTC, consiguiendo con esto no necesitar de usar una pila para guardar la hora del reloj en caso de quedarse sin alimentación nuestro ATmega, ya que sincroniza la hora cada vez que se inicia de una forma tipo al protocolo NTP(Network time protocol) utilizado por cualquier PC, smartphone, tablet, etc para sincronizar la hora automáticamente desde internet.

El microcontrolador tiene grabado el bootloader optiboot, ya que hace uso del watchdog para evitar un posible cuelgue del mismo, debido a que su emplazamiento teóricamente estaría en un lugar lejos de nuestra vivienda.

Este proyecto ha intentado minimizar los gastos de todo el hardware y reducir el consumo al máximo para optimizar la autonomía del mismo, ya que un menor consumo, reduce los gastos en los componentes de mayor valor, que en este caso serán la batería y el módulo solar.

----------------------------------------------------------------------------------------------------------------------------------------

EXPLICACIÓN APLICACIÓN ATMEGA1284P

Voy a tratar de explicar el funcionamiento general del programa del micro. Antes de hacerlo pido disculpas si hay cosas que carecen de entendimiento por no explicarme de forma correcta o por cometer alguna errata.

Este programa toma medidas cada 30s utilizando todos los sensores mencionados, mientras espera a la siguiente medida, el micro se pone en modo bajo consumo para ahorrar el máximo de corriente, junto con el SIM800L, dispositivo que más consume de todo el proyecto. El proceso de medidas es disparado por la interrupción hardware del reloj RTC DS3231, una vez se hace se desactivan las alarmas, sino el micro quedaría clavado dentro de la interrupción. Este dispone como se ha mencionado de un sistema para fijar la hora además de un sistema de detección de errores en el envío de los datos. Si estos llegan a dar dos errores consecutivos en el envío(elección propia) el sistema reinicia el módulo GPRS. En primera instancia, se optó por reiniciar el micro entero, pero cabe la posibilidad de un periodo largo de falta de cobertura, con lo que durante ese periodo, los datos se estarían almacenando en la SD aunque al servidor no llegen. 

El programa utiliza librerías para watchdog, timeronethree para interrupciones software, softwareserial para añadir puertos serie adicionales(para GSM/GPRS SIM800L), JeeLib para utilizar el modo de bajo consumo donde más partes de nuestros micro están desactivadas, dejando solo interrupciones hardware para sacarlo de ese estado el cual es "sleep mode power down", librerías para el ADC MCP3424, para el lector SD/SPI, librería WIRE/DS3231 para reloj RTC, DHT22 para sensor de temperatura/humedad relativa y OneWire/DallasTemperature para sensor de temperatura DS18B20.

Una vez instanciadas las librerías y variables globales, hay que tener en cuenta definir las variables que usa la función de una interrupción como tipo "volatile".

Una vez entra al SETUP, se inician las comunicaciones Serial, se inician los pines I/O, se activa el watchdog, las interrupciones hardware(el ATmega1284p tiene 3, una para anemómetro, pluviómetro y otra para reloj RTC DS3231, encargado de disparar el proceso de medida) y una interrupción por software utilizada para reiniciar el reloj del watchdog ya que si no lo hacemos, no funcionaría correctamente. También se inicia la comunicación de los sensores ds18b20, dht22, el reloj y su salida SQW(esta ocupa otra interrupción hardware, es la encargada de disparar el proceso de medida, a modo de alarma) y se fijan sus alarmas, las cuales se disparan cuando nosotros se lo indiquemos, se fija la hora en el reloj, se inicia el lector microSD, se leen los litros que se han podido perder de las variables tipo volatile en caso de que el micro pierda su alimentación, ya que por elección propia, la pluviometría toma su valor cada 12 horas y por último, se comprueba si el fichero de los datos que se crea en el servidor existe.

Esta comprobación se realiza debido al tipo de estructuración que tiene el fichero donde se almacenan los datos. Los ficheros CSV, si se exportan a excel, utilizan la primera línea para indicar los nombres de las columnas, y el resto de filas, son los datos. Debido a esto hay una casuistica que debe ser cubierta por la programación del micro(más sencilla) y la app del servidor que recibe los datos(más compleja), como por ejemplo si el fichero no existe en el servidor y sí en la tarjeta SD, debe leer la primera línea de este y enviarla al servidor para crearlo, ya que será la línea de nombres o los sensores que tenemos y a continuación realizar las mediciones. No voy a explicar toda la casuística completa para no extenderme en este documento, en caso de querer la explicación completa, preguntar en el foro.

Para llevar una mejor depuración, la opción tomada a sido modular todo en funciones fuera del loop(por propia elección). A continuación voy a poner cuales son y qué hacen:

resetWatchDog(): como su nombre indica, es la encargada de reiniciar el contador del watchdog. Esta función es la utilizada por la interrupción por software con la librería TimerOneThree.

rstSIM800L(): reinicia el módulo, fija la velocidad de comunicación, establece la comunicación con la estación base(hay que incluir el comando AT en caso de utilizar PIN, yo obtuve por no utilizarlo), y levanta la conexión HTTP.

iniciaSD(): simplemente inicia el módulo SD para poder trabajar con la tarjeta.

leerlitros(): esta función se encarga de leer los litros de la tarjeta SD(en caso de que se inicie por primera vez, o se quede sin alimentación).

escribirlitros(): como su nombre indica graba los datos cada 12 horas, tiempo cuando se toman los datos de la pluviometría. Una vez se mandan al servidor el contador se coloca a 0.

existe(): comprueba si el fichero existe en el servidor, para en caso de no existir se crea el fichero y se envía la línea de nombres. En caso contrario solo se manda la línea de datos del proceso de medida. Esta función también comprueba si la respuesta es "SI" o "NO", ya que hay hosting que meten código html adiconal, con lo cual la respuesta sería distinta, ya que lo que se hace es leer la respuesta dada al leer la web mediante un SubString y si no recibe una de estas dos, se reinicia el micro.

uploaddata(): se encarga de enviar los datos mediante una petición GET al servidor, donde son almacenados y enviados posteriormente a emonCMS para su posterior visionado en gráficas. Esta función al terminar coloca al SIM800L en modo autosleep(se pone en modo ahorro pasados unos milisegundos al terminar de recibir datos por el puerto Serie).

idDS18B20(): identifica los sensores del bus 1-Wire y recoge su primer bloque hexadecimal.

names(): Escribe la línea de nombres la cual identifica a todos nuestros shields.

leerultimalinea(): le la última línea escrita en el fichero de datos, que como es lógico son los nuevos datos que acaban de ser medidos y necesitan enviarse al servidor.

leerDHT22(): lee la temperatura y humedad relativa del sensor.

leerDS18B20(): lee la temperatura del sensor.

readMCP3424(): lee el conversor ADC MCP3424.

readane(): realiza la conversión de pulsos a Hz y posteriormente a la velocidad dada en kms, además de almacenarlos en la SD. Una vez que se leen los dispositivos que dependen de una interrupción hardware, para evitar que se quede alguna sin medir, se desactiva la interrupción inherente a este sensor.

readpluvi(): almacena los datos en la SD y desactiva mientras se hace dicha interrupción.

writeDateTime(): escribe la fecha y hora completa en el fichero de la SD.

tomarlecturas(): inicia el módulo SD para trabajar con él, escribe los litros en la SD para evitar perderlos en caso de quedarse sin alimentación el micro, comprueba si el fichero existe en la SD, para crearlo y escribir los nombres o en caso contrario escribir las medidas tomadas, escribe todas las medidas y la marca temporal y comprueba también si es hora de guardar los datos de pluviometría. En caso de afirmativo lo hace y pone el contador a 0.

alarm(): función que cambia una variable booleana cada vez que salta la alarma, esta activa la interrupción y despierta al micro para empezar el proceso de medida.

fijarHora(): utiliza el SIM800L para lee la página que lee la hora de internet, y con esto fijarla en el RTC. Esta función también dispone de un sistema que comprueba si la hora está bien leída. Para ello se comprueba si en la variable "anio"(año), el año el string comienda por un "2", sino lo fuera reinicia el micro. Se hace así por que este RTC está construido para medir la hora como máximo hasta el año 2100.

espera_hora(): toma 1 segundo como tiempo de guarda para esperar a que el servidor devuelva la hora y lee el string que devuelve la respueta(página leída que contiene la hora más datos de conexión satistactoria al servidor) y se divide este en substring que contiene cada dato de una fecha completa.

disponible(): simplemente una función que toma tiempos de guarda entre cada comando AT que se le manda al servidor esperando su respuesta. Nota: se puede jugar con ella en caso de tener errores en el envío. Los tiempos de guarda son críticos en este trabajo, ya que un mayor tiempo utilizado para enviar los datos, estriba en que el proyecto completo esté más tiempo despierto con lo cual aumenta el consumo general, dando lugar a utilizar un módulo solar de mayor bataje y una batería de más amperios, gastando más dinero.

wait(): utilizada para tiempo de guarda al enviar los datos al servidor y esperar su respuesta.

respuesta(): se utiliza para el sistema de detección de errores en el envío.

loop(): en el loop, se toma la hora del RTC, se comprueba si es el segundo 0 o 30, en caso de serlo, el propio reloj ya dispararía el proceso de medida, y en caso contrario, se pone en modo sleep pero solo durante 5s para que la interrupción por software sea capaz de reiniciar el watchdog. Esto es así, debido a que cuando se utiliza este modo de sueño, los timer como la función millis(), microsecond(), etc, se paralizan debido a que se apagan al entrar en modo power_down. Si la alarma está activa, acto seguido se desactivan las alarmas y se vuelve a comprobar la hora y si son los segundos 0 y 30(que en este caso si van a ser seguro, ya que se usa el RTC para este fin) y se dispone a medir, almacenar y enviar las mediciones tomadas.

ane(): lee los pulsos que provoca la interrupción del anemómetro.

pluvi(): lee los pulsos que provoca la interrupción del pluviómetro.

----------------------------------------------------------------------------------------------------------------------------------------

EXPLICACIÓN APLICACIONES PHP EN SERVIDOR

Consta de tres:

existe.php: simple, solo devuelve si o no.

time.php: solo devuelve la hora según una región fijada.

datos.php: comprueba si en el servidor existe el fichero y dive el código en dos partes. La primera(no existe) simplemente crea el fichero y escribe la línea de datos, la segunda(existe) además lee la primera línea del fichero, para comprobar si es igual a los datos que le llegan(puede darse esta situación en el caso de que no existiera en la SD por un cambio de tarjeta y se halla enviado la línea de nombres) y no hacer nada. Además, esta crea un array con los datos divididos, ya que se envían todos juntos en la petición GET, divididos por "," y por último se construye una url en formato tipo json junto con la apikey que te permite escribir datos dentro de emonCMS.

Con esto finalizo la explicación. Espero haberlo hecho de la forma más clara posible, en caso contrario preguntar en el foro para aclarar dudas.

Muchas gracias a todos y saludos.
