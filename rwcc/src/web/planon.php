<!DocType html>
<html>
<body>

<?php

 echo("requesting planon.");
 $w = fopen("/var/www-data/lampa/requests/request.txt", "w+");
 fwrite($w, "planon");
 fclose($w);
?>
<br><br>
 <a href="index.php">back</a>
</body>
</html>
