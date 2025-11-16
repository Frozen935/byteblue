#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "bt_stack_init.h"
#include "common/bt_shell_private.h"

int main(int argc, char *argv[])
{
	struct bt_shell sh;
	int _argc = 0;
	char* _argv[32];
	char* buffer = NULL;
	char* saveptr;
	int ret = 0;
	size_t len, size = 0;

    bt_stack_init_once();
    bt_shell_init(&sh);

    bt_shell_cmd_bt_register(&sh);

	while (1) {
		bt_shell_fprintf_print("openblue> ");
		fflush(stdout);

		memset(_argv, 0, sizeof(_argv));
		len = getline(&buffer, &size, stdin);
		if (-1 == len)
			goto end;

		buffer[len] = '\0';
		if (buffer[len - 1] == '\n')
			buffer[len - 1] = '\0';

		if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
			system(buffer + 1);
#endif
			continue;
		}

		saveptr = NULL;
		char* tmpstr = buffer;

		while ((tmpstr = strtok_r(tmpstr, " ", &saveptr)) != NULL) {
			_argv[_argc] = tmpstr;
			_argc++;
			tmpstr = NULL;
		}

		if (_argc > 0) {
			if (strcmp(_argv[0], "q") == 0) {
				bt_shell_fprintf_info("Bye!\n");
				ret = 0;
				goto end;
			} else if (strcmp(_argv[0], "help") == 0) {
				bt_shell_cmds_show(&sh);
			} else {
				ret = bt_shell_exec(&sh, (const char **)(_argv), _argc);
			}

			_argc = 0;
		}
	}

return 0;

end:
	free(buffer);
	if (ret)
		bt_shell_cmds_show(&sh);

	return 0;
}