<!DocType html>
<html>
<body>

<?php
 $day = $_POST['day'];
 $r = $_POST['red'];
 $wa = $_POST['warm'];
 $c1 = $_POST['cold1'];
 $c2 = $_POST['cold2'];
 $maxtime = $_POST['maxtime'];

 echo("requesting setcolor($day,$r,$wa,$c1,$c2,$maxtime).");
 $w = fopen("/var/www-data/lampa/requests/request.txt", "w+");
 fwrite($w, "setcolor($day,$r,$wa,$c1,$c2,$maxtime)");
 fclose($w);
?>
<br><br>
 <a href="index.php">back</a>
</body>
</html>
