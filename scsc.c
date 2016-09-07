// source code size counter

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char cmp(const char *s, int j, int j1, const char *t)
{
	char c = 0;
	int l = strlen(t);
	if (j + l <= j1 && strncmp(s + j, t, l - 1) == 0) {
		char ct = t[l - 1], cs = s[j + l - 1];
		if (ct == ' ' && cs <= ' ')
			c = 1;
		else if (ct == '&' && strchr(" )&|;", cs) != NULL)
			c = 1;
		else if (cs == ct)
			c = 1;
	}
	return c;
}

char isAlpha(char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (strchr("_@", c) != NULL);
}

char isNum(char c)
{
	return ('0' <= c && c <= '9');
}

char isAlpNum(char c)
{
	return isAlpha(c) | isNum(c);
}

#define MAXSIZ	16 * 1024 * 1024

#define C				buf[j]
#define CMP(t)			cmp(buf, j, j1, t)
#define INC_SC			sc++, j++;
#define WHILE_J(cond)	while (j < j1 && (cond))
#define SKIP(t)			else if (CMP(t + 2) != 0) { sc += *t - '0'; j += t[1] - '0'; }

int main(int argc, const char **argv)
{
	char *buf = malloc(MAXSIZ);
	int i, j, j1 = 0, sc, tsc = 0;
	puts("    <scsc03a>");
	for (i = 1; i < argc; i++) {
		char *errmsg = "", c, cc;
		FILE *fp = fopen(argv[i], "rb");
		sc = j = 0;
		if (fp == NULL) {
			errmsg = " -- fopen error";
		} else {
			j1 = fread(buf, 1, MAXSIZ - 2, fp);
			fclose(fp);
			if (j1 >= MAXSIZ - 2)
				errmsg = " -- file too large";
		}
		if (*errmsg == '\0') {
			while (j < j1) {
				c = C;
				if (c <= ' ' || strchr("{}()", c) != NULL) {
					j++;
				}
				SKIP("05const ")	// constはカウントしない.
				SKIP("14'\\0'")		// '\0' を 0 と同一視してやる.
				SKIP("14'\\b'")		// '\b' を 8 と同一視してやる.
				SKIP("14'\\t'")		// '\t' を 9 と同一視してやる.
				SKIP("24'\\n'")		// '\n' を 10 と同一視してやる.
				SKIP("24'\\r'")		// '\r' を 13 と同一視してやる.
				SKIP("04!= 0&")		// != 0 はカウントしない.
				SKIP("14== 0&")		// == 0 は ! に読み替えてやる.
				SKIP("07!= NULL&")	// != NULL はカウントしない.
				SKIP("17== NULL&")	// == NULL は ! に読み替えてやる.
				SKIP("15!= -1&")	// != -1 は ~ に読み替えてやる.
				SKIP("16!= EOF&")	// != EOF は ~ に読み替えてやる.
				else if (isAlpha(c) != 0) {
					INC_SC
					WHILE_J (isAlpNum(C) != 0)
						j++;
				} else if (CMP("//") + CMP("#include") != 0) {
					WHILE_J (C != '\n')
						j++;
				} else if (CMP("/*") != 0) {
					while (CMP("*/") == 0)
						j++;
					j += 2;
				} else if (strchr("\"\'", c) != NULL) {
					cc = c;
					INC_SC
					WHILE_J (1) {
						c = C;
						INC_SC
						if (c == cc) break;
						if (j < j1 && c == '\\' && strchr("\"\'\\", C) != NULL) {
							INC_SC
						}
					}
				} else if (isNum(c) != 0 || (j + 1 < j1 && c == '.' && isNum(buf[j + 1]) != 0)) {
					if (CMP("0x") != 0) {
						j += 2;
						while (j + 1 < j1 && strchr("0_", C) != NULL && isAlpNum(buf[j + 1]) != 0)
							j++;
					}
					WHILE_J (isAlpNum(C) != 0 || C == '.') {
						INC_SC
					}
				} else if (CMP("#if 0 ") != 0) {
					cc = 1;
					do {
						j++;
						if (CMP("#if 0 ")  != 0)
							cc++;
						if (CMP("#endif ") != 0) {
							cc--;
							j += 6;
						}
					} while (cc > 0);
				} else {
					INC_SC
				}
			}
		}
		printf("%9d : %s%s\n", sc, argv[i], errmsg);
		tsc += sc;
	}
	printf("%9d : [total]\n", tsc);
	return 0;
}
