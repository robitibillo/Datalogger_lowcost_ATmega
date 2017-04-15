<?php
$self = $_SERVER['PHP_SELF']; //Obtenemos el nombre de la página en la que nos encontramos
header("refresh:1; url=$self"); //Refrescamos cada 5 segundos*/
date_default_timezone_set("Europe/Madrid");
echo date("Y,m,d,H,i,s", time());
?>