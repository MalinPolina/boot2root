#include <stdio.h>
#include <unistd.h>
int func4(int nbr) {
	if (nbr <= 1)
		return 1;
	int x = func4(nbr - 1);
	int y = func4(nbr - 2);
	return x + y;
}

void main(void) {
	int nbr = 8;
	//l = sscanf(str, "%d", nbr);
	//if (l != 1) {
		//write(1, "fail\n"), 5;
	if (nbr > 0) {
		if (func4(nbr) != 55)
			write(1, "fail\n", 5);
	}
	//}
	return;
}
