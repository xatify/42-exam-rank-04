#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

typedef struct node {
	void		*data;
	struct node *next;
}			t_node;

enum errors { BAD_ARGS, CD_FAILED, EXEC_FAILED, FATAL};

const char *ERRORS[] = { "error: cd: bad arguments\n", "error: cd: cannot change directory to ",
						 "error: cannot execute ", "error: fatal\n"};

size_t ft_strlen (const char *str) { return (*str)? 1 + ft_strlen (++str): 0; }

size_t size (t_node *nd) { return (nd == 0)? 0: 1 + size (nd->next) ; }

t_node *last (t_node *head) { return (head->next)? last (head->next): head; }

void 	print (const char *str) { write (2, str, ft_strlen (str)); }

void	fill (char **args, t_node *list) {
	*args = (list)? list->data: 0x0;
	if (!list) return;
	fill (++args, list->next);
}

void 	error (int r, const char *str) { 
	print (ERRORS[r]);
	if (str) {
		print (str);
		print ("\n");
	}
	if (r == FATAL) exit (1); 
}

void duplicate (int fd1, int fd2) { if (dup2 (fd1, fd2) == -1) error (FATAL, 0x0); }

void 	erase (t_node *nd, void (*clean) (void *))	{
	if (!nd) return;
	erase (nd->next, clean);
	if (clean) clean (nd->data);
	free (nd);
}

void add_ (t_node **head, t_node * list, void *data) {
	t_node *node = malloc (sizeof (t_node));
	node->data = (data) ? data: malloc (sizeof (char *) * (size (list) + 1));
	if (!data) fill (node->data, list);
	node->next = 0x0;
	if (!(*head)) *head = node;
	else last (*head)->next = node;
}

void cd (char **args) {
	if (!args[1] || args[2]) error (BAD_ARGS, 0x0);
	else if (chdir (args[1]) == -1)
		error (CD_FAILED, args[1]);
}

void execute (t_node *cmd, char **envp) {
	int tmp_in = -1;
	int fds[2];
	for ( ; cmd; cmd = cmd->next) {
		char **args = cmd->data;
		if (strcmp (*args, "cd") == 0)
			cd (args);
		else {
			if (cmd->next)
				if (pipe (fds) == -1) error (FATAL, 0x0);
			int pid = fork ();
			if (pid == 0) {
				if (tmp_in == -1) {
					if (cmd->next) {
						duplicate (fds[1], 1);
						close (fds[0]);
					}
				}
				else if (!(cmd->next))
					duplicate (tmp_in, 0);
				else {
					duplicate (fds[1], 1);
					duplicate (tmp_in, 0);
					close (fds[0]);
				}
				execve (args[0], args, envp);
				error (EXEC_FAILED, args[0]);
			}
			else if (pid < 0)
				error (FATAL, 0x0);
			if (tmp_in != -1)
				close (tmp_in);
			tmp_in = fds[0];
			if (cmd->next)
				close (fds[1]);
		}
	}
	while (waitpid (-1, 0x0, 0x0) > 0);
}

int main (int argc, char **argv, char **envp) {
	(void)argc;
	argv++;
	t_node *cmd = 0x0;
	while (*argv) {
		t_node *pipe = 0x0;
		while (*argv && strcmp (*argv, "|") && strcmp (*argv, ";"))
				add_ (&pipe, 0x0, *argv++);
		if (pipe) {
			add_ (&cmd, pipe, 0x0);
			erase (pipe, 0x0);
		}
		if (!(*argv) || strcmp (*argv, ";") == 0) {
			execute (cmd, envp);
			erase (cmd, free);
			cmd = 0x0;
		}
		if (!*argv++) break;
	}
} 
