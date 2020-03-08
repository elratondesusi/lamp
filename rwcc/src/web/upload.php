<!DocType html>
<html>
<body>
<?php

        if(!isset($_POST['upload'])) return;
        $target_file = '/var/www-data/lampa/requests/newplan.txt';
        $current_plan_file = '/var/www-data/lampa/requests/plan.txt';
        $original_filename = basename($_FILES['filename']['name']);
        $tmp_filename = $_FILES['filename']['tmp_name'];
        //echo('<br>original_filename=' . $original_filename);
        //echo('<br>target_file=' . $target_file);
        //echo('<br>tmp_filename=' . $tmp_filename);

        // Check if file already exists
        if (file_exists($target_file))
                unlink($target_file);

        if (move_uploaded_file($tmp_filename, $target_file))
        {
                echo('the file ' . $original_filename . ' has been uploaded as a new plan.');
                copy($target_file, $current_plan_file);
        }
        else echo('Sorry, there was an error uploading your file.');

?>
<br><br>
<a href="index.php">back</a>
</body>
</html>
