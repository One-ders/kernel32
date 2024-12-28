
#define SYSCALL_NATIVE	1
#define SYSCALL_LNX	2

#define SVC_CREATE_TASK 1
#define SVC_MSLEEP      SVC_CREATE_TASK+1
#define SVC_SLEEP_ON    SVC_MSLEEP+1
#define SVC_WAKEUP      SVC_SLEEP_ON+1
#define SVC_IO_OPEN     SVC_WAKEUP+1
#define SVC_IO_READ     SVC_IO_OPEN+1
#define SVC_IO_WRITE    SVC_IO_READ+1
#define SVC_IO_CONTROL  SVC_IO_WRITE+1
#define SVC_IO_LSEEK    SVC_IO_CONTROL+1
#define SVC_IO_CLOSE    SVC_IO_LSEEK+1
#define SVC_IO_SELECT   SVC_IO_CLOSE+1
#define SVC_IO_MMAP     SVC_IO_SELECT+1
#define SVC_IO_MUNMAP   SVC_IO_MMAP+1
#define SVC_KILL_SELF   SVC_IO_MUNMAP+1
#define SVC_BLOCK_TASK  SVC_KILL_SELF+1
#define SVC_UNBLOCK_TASK SVC_BLOCK_TASK+1
#define SVC_KILL_PROC	SVC_UNBLOCK_TASK+1
#define SVC_SETPRIO_TASK SVC_KILL_PROC+1
#define SVC_SETDEBUG_LEVEL SVC_SETPRIO_TASK+1
#define SVC_REBOOT      SVC_SETDEBUG_LEVEL+1
#define SVC_GETTIC      SVC_REBOOT+1
#define SVC_SBRK        SVC_GETTIC+1
#define SVC_BRK         SVC_SBRK+1
#define SVC_FORK        SVC_BRK+1
#define SVC_EXIT	SVC_FORK+1
#define SVC_WAIT	SVC_EXIT+1


// Remapped system calls from musl
#define LNX_EXIT		1
#define LNX_FORK		2
#define LNX_READ		3
#define LNX_WRITE		4
#define LNX_OPEN		5
#define LNX_CLOSE		6
#define LNX_WAITPID		7
#define LNX_CREAT		8
#define LNX_LINK		9
#define LNX_UNLINK		10
#define LNX_EXECVE		11
#define LNX_CHDIR		12
#define LNX_TIME		13
#define LNX_MKNOD		14
#define LNX_CHMOD		15
#define LNX_LCHOWN		16
#define LNX_BREAK		17
#define LNX_LSEEK		18
#define LNX_GETPID		19
#define LNX_MOUNT		20
#define LNX_UMOUNT		21
#define LNX_SETUID		22
#define LNX_GETUID		23
#define LNX_STIME		24
#define LNX_PTRACE		25
#define LNX_ALARM		26
#define LNX_PAUSE		27
#define LNX_UTIME		28
#define LNX_STTY		29
#define LNX_GTTY		30
#define LNX_ACCESS		31
#define LNX_NICE		32
#define LNX_FTIME		33
#define LNX_SYNC		34
#define LNX_KILL		35
#define LNX_RENAME		36
#define LNX_MKDIR		37
#define LNX_RMDIR		38
#define LNX_DUP			39
#define LNX_PIPE		40
#define LNX_TIMES		41
#define LNX_PROF		42
#define LNX_BRK			43
#define LNX_SETGID		44
#define LNX_GETGID		45
#define LNX_SIGNAL		46
#define LNX_GETEUID		47
#define LNX_GETEGID		48
#define LNX_ACCT		49
#define LNX_UMOUNT2		50
#define LNX_LOCK		51
#define LNX_IOCTL		52
#define LNX_FCNTL		53
#define LNX_MPX			54
#define LNX_SETPGID		55
#define LNX_ULIMIT		56
#define LNX_UMASK		57
#define LNX_CHROOT		58
#define LNX_USTAT		59
#define LNX_DUP2		60
#define LNX_GETPPID		61
#define LNX_GETPGRP		62
#define LNX_SETSID		63
#define LNX_SIGACTION		64
#define LNX_SGETMASK		65
#define LNX_SSETMASK		66
#define LNX_SETREUID		67
#define LNX_SETREGID		68
#define LNX_SIGSUSPEND		69
#define LNX_SIGPENDING		70
#define LNX_SETHOSTNAME		71
#define LNX_SETRLIMIT		72
#define LNX_GETRLIMIT		73
#define LNX_GETRUSAGE		74
#define LNX_GETTIMEOFDAY	75
#define LNX_SETTIMEOFDAY	76
#define LNX_GETGROUPS		77
#define LNX_SETGROUPS		78
#define LNX_SYMLINK		79
#define LNX_READLINK		80
#define LNX_USELIB		81
#define LNX_SWAPON		82
#define LNX_REBOOT		83
#define LNX_READDIR		84
#define LNX_MMAP		85
#define LNX_MUNMAP		86
#define LNX_TRUNCATE		87
#define LNX_FTRUNCATE		88
#define LNX_FCHMOD		89
#define LNX_FCHOWN		90
#define LNX_GETPRIORITY		91
#define LNX_SETPRIORITY		92
#define LNX_PROFIL		93
#define LNX_STATFS		94
#define LNX_FSTATFS		95
#define LNX_IOPERM		96
#define LNX_SOCKETCALL		97
#define LNX_SYSLOG		98
#define LNX_SETITIMER		99
#define LNX_GETITIMER		100
#define LNX_STAT		101
#define LNX_LSTAT		102
#define LNX_FSTAT		103
#define LNX_IOPL		104
#define LNX_VHANGUP		105
#define LNX_IDLE		106
#define LNX_VM86		107
#define LNX_WAIT4		108
#define LNX_SWAPOFF		109
#define LNX_SYSINFO		110
#define LNX_IPC			111
#define LNX_FSYNC		112
#define LNX_SIGRETURN		113
#define LNX_CLONE		114
#define LNX_SETDOMAINNAME	115
#define LNX_UNAME		116
#define LNX_MODIFY_LDT		117
#define LNX_ADJTIMEX		118
#define LNX_MPROTECT		119
#define LNX_SIGPROCMASK		120
#define LNX_CREATE_MODULE	121
#define LNX_INIT_MODULE		122
#define LNX_DELETE_MODULE	123
#define LNX_GET_KERNEL_SYMS	124
#define LNX_QUOTACTL		125
#define LNX_GETPGID		126
#define LNX_FCHDIR		127
#define LNX_BDFLUSH		128
#define LNX_SYSFS		129
#define LNX_PERSONALITY		130
#define LNX_AFS_SYSCALL		131
#define LNX_SETFSUID		132
#define LNX_SETFSGID		133
#define LNX_LLSEEK		134
#define LNX_GETDENTS		135
#define LNX_NEWSELECT		136
#define LNX_FLOCK		137
#define LNX_MSYNC		138
#define LNX_READV		139
#define LNX_WRITEV		140
#define LNX_CACHEFLUSH		141
#define LNX_CACHECTL		142
#define LNX_SYSMIPS		143
#define LNX_GETSID		144
#define LNX_FDATASYNC		145
#define LNX_SYSCTL		146
#define LNX_MLOCK		147
#define LNX_MUNLOCK		148
#define LNX_MLOCKALL		149
#define LNX_MUNLOCKALL		150
#define LNX_SCHED_SETPARAM	151
#define LNX_SCHED_GETPARAM	152
#define LNX_SCHED_SETSCHEDULER	153
#define LNX_SCHED_GETSCHEDULER	154
#define LNX_YIELD		155
#define LNX_SCHED_GET_PRIORITY_MAX 156
#define LNX_SCHED_GET_PRIORITY_MIN 157
#define LNX_SCHED_RR_GET_INTERVAL  158
#define LNX_NANOSLEEP		159
#define LNX_MREMAP		160
#define LNX_ACCEPT		161
#define LNX_BIND		162
#define LNX_CONNECT		163
#define LNX_GETPEERNAME		164
#define LNX_GETSOCKNAME		165
#define LNX_GETSOCKOPT		166
#define LNX_LISTEN		167
#define LNX_RECV		168
#define LNX_RECVFROM		169
#define LNX_RECVMSG		170
#define LNX_SEND		171
#define LNX_SENDMSG		172
#define LNX_SENDTO		173
#define LNX_SETSOCKOPT		174
#define LNX_SHUTDOWN		175
#define LNX_SOCKET		176
#define LNX_SOCKETPAIR		177
#define LNX_SETRESUID		178
#define LNX_GETRESUID		179
#define LNX_QUERY_MODULE	180
#define LNX_POLL		181
#define LNX_NFSSERVCTL		182
#define LNX_SETRESGID		183
#define LNX_GETRESGID		184
#define LNX_PRCTL		185
#define LNX_RT_SIGRETURN	186
#define LNX_RT_SIGACTION	187
#define LNX_RT_SIGPROCMASK	188
#define LNX_RT_SIGPENDING	189
#define LNX_RT_SIGTIMEDWAIT	190
#define LNX_RT_SIGQUEUEINFO	191
#define LNX_RT_SIGSUSPEND	192
#define LNX_PREAD64		193
#define LNX_PWRITE64		194
#define LNX_CHOWN		195
#define LNX_CAPGET		196
#define LNX_CAPSET		197
#define LNX_SIGALTSTACK		198
#define LNX_SENDFILE		199
#define LNX_GETPMSG		200
#define LNX_PUTPMSG		201
#define LNX_MMAP2		202
#define LNX_TRUNCATE64		203
#define LNX_FTRUNCATE64		204
#define LNX_STAT64		205
#define LNX_LSTAT64		206
#define LNX_FSTAT64		207
#define LNX_PIVOT_ROOT		208
#define LNX_MINCORE		209
#define LNX_MADVISE		210
#define LNX_GETDENTS64		211
#define LNX_FCNTL64		212
