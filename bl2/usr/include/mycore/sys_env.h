
struct Env {
        int io_fd;
};

typedef int (*cmdFunc)(int,char **, struct Env *);

struct cmd {
        char *name;
        cmdFunc fnc;
};

struct cmd_node {
        char *name;
        struct cmd *cmds;
        struct cmd_node *next;
};

extern struct cmd_node *root_cmd_node;

int generic_help_fnc(int argc, char **argv, struct Env *env);
int install_cmd_node(struct cmd_node *, struct cmd_node *parent);
struct cmd *lookup_cmd(char *name, int fd);

int argit(char *str, int len, char *argv[16]);

