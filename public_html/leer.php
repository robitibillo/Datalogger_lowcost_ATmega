<?php //Ejemplo aprenderaprogramar.com
$self = $_SERVER['PHP_SELF']; //Obtenemos el nombre de la página en la que nos encontramos
header("refresh:60; url=$self"); //Refrescamos cada 5 segundos*/
$file = fopen("private/datos.csv", "r");
while(!feof($file)) {
$linea = fgets($file);
echo $linea . "<br/>";
}
fclose($file);
?>