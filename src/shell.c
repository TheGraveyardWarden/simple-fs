#include "shell.h"
#include "core.h"
#include "fs.h"
#include "file.h"
#include "ops.h"
#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define INPUT_LEN 1024
#define MAX_ARGV 10

extern struct superblock *sbp;

int env_init(struct env *env, u64 cwd_ino) {
	char *cwd;

	if (ino_path(cwd_ino, &cwd) < 0)
		return -1;

	strncpy(env->cwd, cwd, ENV_CWD_MAX_LEN);
	env->cwd_ino = cwd_ino;

	free(cwd);

	return 0;
}

void cmd_init(struct cmd* cmd, const char* cmd_name, cmd_handler_t handler) {
	strncpy(cmd->cmd, cmd_name, CMD_LEN);
	cmd->handler = handler;
	list_init(&cmd->cmds);
}

void shell_init(struct shell *shell, struct env *env) {
	shell->env = env;
	list_init(&shell->cmds);
}

int shell_register_cmd(struct shell *shell, struct cmd *cmd) {
	struct cmd *trav;

	list_foreach(&shell->cmds, trav, struct cmd, cmds) {
		if (!strncmp(trav->cmd, cmd->cmd, CMD_LEN)) {
			errno = -EEXIST;
			return -1;
		}
	}

	if ((trav = malloc(sizeof(struct cmd))) == NULL)
		return -1;

	memcpy(trav, cmd, sizeof(struct cmd));

	list_add(&shell->cmds, &trav->cmds);
	return 0;
}

// returns < 0 on err
// on success returns argc
int split_whitespace(char *input, char *argv[MAX_ARGV]) {
	char *tmp, *tmp2;
	int i = 0;

	argv[i++] = input;

	tmp = strchr(input, ' ');
	if (!tmp)
		return i;

	do {
		*tmp++ = 0;

		if (*tmp == ' ')
			continue;

		if (i == MAX_ARGV)
			break;

		if (*tmp == '"') {
			tmp++;
			tmp2 = strchr(tmp, '"');

			if (!tmp2)
				return -2;

			*tmp2++ = 0;
			argv[i++] = tmp;
			tmp = tmp2;
			continue;
		}

		argv[i++] = tmp;
	} while((tmp = strchr(tmp, ' ')) != NULL);

	return i;
}

int shell_enter(struct shell* shell) {
	char input[INPUT_LEN], *argv[MAX_ARGV], *tmp;
	int argc, ret;
	struct cmd *trav;


	while (1) {
loop:
    printf("[user@%s]-[%s] $ ", sbp->name, shell->env->cwd);
    fflush(stdout);

		if (fgets(input, INPUT_LEN, stdin) == NULL) {
			continue;
		}

		tmp = strchr(input, '\n');
		*tmp = 0;

		if (!strncmp(input, "exit\0", 5)) {
			printf("peace ;P\n");
			break;
		}

		argc = split_whitespace(input, argv);
		if (argc < 0) {
			printf("invalid input\n");
			continue;
		}

		list_foreach(&shell->cmds, trav, struct cmd, cmds) {
			if (!strncmp(argv[0], trav->cmd, CMD_LEN)) {
				if ((ret = trav->handler(shell, argc, argv)) < 0) {
					printf("command failed with exit status: %d\n", ret);
				}
				goto loop;
			}
		}

		printf("%s not found\n", argv[0]);
	}

	return 0;
}

