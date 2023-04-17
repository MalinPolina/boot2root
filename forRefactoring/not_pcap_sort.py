# ! /usr/bin/env python3
import os
import re
import sys

dict = {}


for filename in os.listdir("/home/polina/ft_fun"):
    with open(os.path.join("/home/polina/ft_fun", filename)) as f:
        txt = f.read()
        f.close()
        nbr_line = re.search(r'//file([0-9]*)', txt)
        dict[int(nbr_line.group(1))] = txt
                          
for k, v in sorted(dict.items()):
    print(v)
