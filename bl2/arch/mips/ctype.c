
int tolower(int c) {
	if ((c>='A') && (c<='Z')) {
		return (c-'A')+'a';
	}
	return c;
}

int isspace(int c) {
	switch (c) {
		case ' ':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			return 1;
			break;
		default:
			break;
	}
	return 0;
}

