<!DocType html>
<html>
<body>
Lamp
<br><br>
<ul>
<li>status: <?php $f = fopen("/var/www-data/lampa/requests/status.txt", "r"); echo(fread($f, 1000)); fclose($f);?> </li>
<li><a href="requests/log.txt">log</a></li>
<li><a href="setcolor.html">set color</a></li>
<li><a href="setnightcolor.html">set night color</a></li>
<li><a href="requests/plan.txt">view plan</a></li>
<li><a href="uploadplan.html">upload plan</a></li>
<li><a href="planon.php">turn plan on</a></li>
<li><a href="planoff.php">turn plan off</a></li>
<li><a href="plantest.php">quick test plan</a></li>
</ul>
</body>
</html>
