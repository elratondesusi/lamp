<!DocType html>
<html>
<body>

<?php

 echo("requesting plan test.");
 $w = fopen("/var/www-data/lampa/requests/request.txt", "w+");
 fwrite($w, "plantest");
 fclose($w);
?>
<br><br>
 <a href="index.php">back</a>
</body>
</html>
