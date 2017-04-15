<?php
$nombre_fichero = 'private/datos.csv';

if (file_exists($nombre_fichero)) {


$arraydatos = explode(",", $_GET['datos']);
$aux=$_GET['datos'];
$aux.=" ";

if($f = fopen('private/datos.csv', 'r')){
  $lineanombres = fgets($f);
  fclose($f);
}

$arraynombres = explode(",", $lineanombres);

if (!strnatcmp($aux, $lineanombres)) {
	goto end;
}
	
$file = fopen("private/datos.csv", "a") or die("Imposible abrir fichero");
if($_GET['datos'] == "")
{
goto end;
}
else
{
fwrite($file, $_GET['datos'].PHP_EOL);
fclose($file);
}

} else {

$file = fopen("private/datos.csv", "a") or die("Imposible abrir fichero");
fwrite($file, $_GET['datos'].PHP_EOL);
fclose($file);


$arraydatos = explode(",", $_GET['datos']);
$aux=$_GET['datos'];
$aux.=" ";

if($f = fopen('private/datos.csv', 'r')){
  $lineanombres = fgets($f);
  fclose($f);
}

$arraynombres = explode(",", $lineanombres);

if (!strnatcmp($aux, $lineanombres)) {
	goto end;
}

$file = fopen("private/datos.csv", "a") or die("Imposible abrir fichero");
if($_GET['datos'] == "")
{
goto end;
}
else
{
fwrite($file, $_GET['datos'].PHP_EOL);
fclose($file);
}

}

$url="http://emoncms.org/input/post.json?apikey=/*COLOAR APIKEY*/&node=1&json={";

for ($i = 1; $i < count($arraynombres)-1; $i++) {
	if($i<count($arraynombres)-2){
		$url.=$arraynombres[$i].":".$arraydatos[$i].",";
	}
  else{
	  $url.=$arraynombres[$i].":".$arraydatos[$i];
  }
}
$url.="}";
file_get_contents($url);
end:
?>