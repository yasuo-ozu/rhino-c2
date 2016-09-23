int c = 1, i, j;
for (i = 3; i <= 1000; i += 2) {
	for (j = 3; j < i; j += 2) {
		if (i % j == 0) {
			break;
		}
	}
	if (j == i) c++;
}
"Count is";
c;
