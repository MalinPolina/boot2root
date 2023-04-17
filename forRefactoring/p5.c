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
