
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>


static int gettype(mode_t mode) {
	if (S_ISREG(mode)) {
		return '-';
	} else if (S_ISDIR(mode)) {
		return 'd';
	} else if (S_ISLNK(mode)) {
		return 'l';
	} else if (S_ISFIFO(mode)) {
		return 'f';
	} else if (S_ISSOCK(mode)) {
		return 's';
	} else if (S_ISBLK(mode)) {
		return 'b';
	} else if (S_ISCHR(mode)) {
		return 'c';
	}
	return '?';
}

static char *getperms(mode_t mode, char *buf) {
	int i;

	for(i=0;i<9;i++) {
		if ((mode>>i)&1) {
			switch(i%3) {
				case 0:
					buf[8-i]='x';
					break;
				case 1:
					buf[8-i]='w';
					break;
				case 2:
					buf[8-i]='r';
					break;
			}
		} else {
			buf[8-i]='-';
		}
	}
	buf[9]=0;
	return buf;
}


int main(int argc, char **argv) {
	DIR *dirp;
	char *path="/";
	char *t;
	struct dirent *dirent;
	char  pbuf[10];

	if (argc==2) {
		path=argv[1];
	}

	dirp=opendir(path);
	if(!dirp) return 0;

	while((dirent=readdir(dirp))) {
		struct stat sb;
		int rc;

#if 0
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
#endif
		rc=stat(dirent->d_name, &sb);
		if (rc<0) {
			printf("failed to to stat\n");
		} else {
			printf("%c%s %8d %8d %16lld %s\n",
				gettype(sb.st_mode), getperms(sb.st_mode,pbuf),
				sb.st_uid, sb.st_gid, sb.st_size,
				dirent->d_name);
#if 0
		printf("I-node number:            %ld\n", (long) sb.st_ino);

		printf("Mode:                     %lo (octal)\n",
			(unsigned long) sb.st_mode);

		printf("Link count:               %ld\n", (long) sb.st_nlink);
		printf("Ownership:                UID=%ld   GID=%ld\n",
			(long) sb.st_uid, (long) sb.st_gid);

		printf("Preferred I/O block size: %ld bytes\n",
			(long) sb.st_blksize);
		printf("File size:                %lld bytes\n",
			(long long) sb.st_size);
		printf("Blocks allocated:         %lld\n",
			(long long) sb.st_blocks);

		printf("Last status change:       %x\n", sb.st_ctime);
		printf("Last file access:         %x\n", sb.st_atime);
		printf("Last file modification:   %x\n", sb.st_mtime);
#endif
		fflush(stdout);
		}
	}
	return 0;
}
