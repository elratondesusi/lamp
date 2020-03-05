<!DocType html>
<html>
<body>

<?php
 echo("requesting planoff.");
 $w = fopen("/var/www-data/lampa/requests/request.txt", "w+");
 fwrite($w, "planoff");
 fclose($w);
?>
<br><br>
 <a href="index.php">back</a>
</body>
</html>
