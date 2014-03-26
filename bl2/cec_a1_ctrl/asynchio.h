
typedef int (*EventHandler)(int fd, int event, void *uref);

int register_event(int fd,
                   int event,
                   EventHandler,
                   void *uref);

