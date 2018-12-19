#include <dirent.h>
#include <stdio.h>


int main(int argc, char **argv) {
	DIR *dirp;
	char *path=".";
	char *t;
	struct dirent *dirent;

	if (argc==2) {
		path=argv[1];
	}
	dirp=opendir(path);
	if(!dirp) return 0;

	while((dirent=readdir(dirp))) {
		switch(dirent->d_type) {
			case	DT_FIFO:
				t="p";
				break;
			case	DT_CHR:
				t="c";
				break;
			case	DT_DIR:
				t="d";
				break;
			case	DT_BLK:
				t="b";
				break;
			case	DT_REG:
				t="-";
				break;
			case	DT_LNK:
				t="l";
				break;
			case	DT_SOCK:
				t="s";
				break;
			default:
				t="?";
				break;
		}
		printf("%s---------	%d		%s\n",t,dirent->d_fileno,dirent->d_name);
	}
}
