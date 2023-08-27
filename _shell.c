#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"

char *buffer;
char *trimmed;

/**
 * handle_sigint - to free memory when the program is killed
 *
 * Return: nothing
 */
void handle_sigint(int signum)
{
	if (signum > 0)
	{
		free(buffer);
	}
	exit(EXIT_SUCCESS);
}

/**
 * main - simple shell! The big one!
 *
 * Return: Always 0.
 */
int main(int ac, char **av, char **env)
{
	int count = 0, tty = 1, i = 0, path_searched = 0;
	size_t input = 0;
	size_t bufsize = 4096;
	char **argv = NULL;
	pid_t pid;
	char *paths, *validpath = NULL, *errmsg = NULL, *final;

	buffer = malloc(bufsize + 1);

	signal(SIGINT, handle_sigint);
	tty = isatty(STDIN_FILENO);

	/* handle the echo */
	if (tty == 0 && ac == 1 && av[0] != NULL)
	{
		/* _setenv(env, "PATH", ""); */
		paths = _getenv(env, "PATH");
		/* printf("path - %s\n", paths); */

		while ((input = getline(&buffer, &bufsize, stdin)) < bufsize)
		{
			/* printf("Retrieved line of length %lu :\n", input); */
			/* printf("%s", buffer); */

			/* clean up the buffer input and remove whitespace */
			
			trimmed = strtrim(buffer);
			if (trimmed != NULL)
			{
				strcpy(buffer, trimmed);

				path_searched = 0;
				if (paths != NULL)
				{
					validpath = _exists(paths, buffer);
					path_searched = 1;
				}

				if (validpath == NULL)
				{
					errmsg = malloc(strlen(av[0]) + strlen(buffer) + 17);

					/* example: ./hsh: 1: ls: not found  */
					strcpy(errmsg, av[0]);
					strcat(errmsg, ": ");
					strcat(errmsg, "1");
					strcat(errmsg, ": ");
					strcat(errmsg, buffer);
					strcat(errmsg, ": not found\n");

					write(2, errmsg, strlen(errmsg));

					if (path_searched == 1)
					{
						free(validpath);
					}

					free(errmsg);
					exit(127);
				}

				/* printf("valid path exists!\n"); */

				final = malloc(strlen(validpath) + strlen(buffer) + 2);
				strcpy(final, validpath);
				strcat(final, "/");
				strcat(final, buffer);

				free(buffer);
				buffer = final;

				/* printf("buffer - %s\n", buffer); */

				count = count_cmd_line_params(buffer, " ");
				argv = populate_argv_array(count, buffer, " ");

				pid = fork();
				if (pid < 0)
				{
					exit(EXIT_FAILURE);
				}
				else if (pid == 0)
				{
					/* printf("execute - %s\n", argv[0]); */
					execvp(argv[0], argv);

					/* if execvp returns, there was an error */
					perror("execvp");
					exit(EXIT_FAILURE);
				}

				i = 0;
				while(argv[i] != NULL)
				{
					/* printf("argv[%d] - %s\n", i, argv[i]); */
					free(argv[i]);
					i++;
				}
				free(argv);

				wait(NULL);
			}

			free(trimmed);
		}
	}

	/* handle user input */
	while(tty)
	{
		printf("$ ");

		input = getline(&buffer, &bufsize, stdin);
		if (input > bufsize)
		{
			exit(EXIT_SUCCESS);
		}

		/* printf("buffer - %s\n", buffer); */

		count = count_cmd_line_params(buffer, " ");
		argv = populate_argv_array(count, buffer, " ");

		pid = fork();

		/* printf("pid is - %d\n", pid); */

		if (pid < 0)
		{
			/* handle error */
			exit(EXIT_FAILURE);
		}
		else if (pid == 0)
		{
			if (argv[0] != NULL && strlen(argv[0]) > 0)
			{
				execvp(argv[0], argv);
				/* if execvp returns, there was an error */
				perror("execvp");
				exit(EXIT_FAILURE);
			}
		}

		i = 0;
		while(argv[i] != NULL)
		{
			free(argv[i]);
			i++;
		}
		free(argv);

		wait(NULL);
	}
	
	free(buffer);
	return (0);
}

void _setenv(char **env, char *myvar, char *myvalue)
{
	int i = 0;
	/* extern char** environ; */
	char *temp, *final, *token;

	while (env[i] != NULL)
	{
		temp = malloc(strlen(env[i]) + 1);
		strcpy(temp, env[i]);

		token = strtok(temp, "=");
		if (strcmp(token, myvar) == 0)
		{
			final = malloc(strlen(token) + strlen("=") +  strlen(myvalue) + 1);

			strcpy(final, token);
			strcat(final, "=");
			strcat(final, myvalue);
			env[i] = malloc(strlen(final) + 1);

			env[i] =  final;

			/* j = i; */
		}
		i++;
		free(temp);
	}
}

char *_getenv(char **env, char *name)
{
	int i = 0;
	char *temp, *token;

	while (env[i] != NULL)
	{
		temp = malloc(strlen(env[i]) + 1);
		strcpy(temp, env[i]);

		token = strtok(temp, "=");
		if (strcmp(token, name) == 0)
		{
			token = strtok(NULL, "=");
			return token;
		}

		i++;
		free(temp);
	}

	return NULL;
}

char *_exists(char *paths, char *input)
{
	char *cmd, *temp = NULL, *path, *pathstemp = NULL, *final = "", *validpath = NULL;
	struct stat st;

	temp = malloc(strlen(input) + 1);
	strcpy(temp, input);
	/* in case the program string passed in has flags */
	cmd = strtok(temp, " ");

	/* printf("cmd is %s\n", cmd); */

	pathstemp = malloc(strlen(paths) + 1);
	strcpy(pathstemp, paths);

	path = strtok(pathstemp, ":");
	while (path != NULL)
	{
		/* printf("path is %s\n", path); */

		final = malloc(strlen(path) + strlen(cmd) + 2);

		strcpy(final, path);
		strcat(final, "/");
		strcat(final, cmd);

	        if (stat(final, &st) == 0)
	        {
			validpath = malloc(strlen(path) + 1);
			strcpy(validpath, path);

			free(temp);
			free(pathstemp);
			free(final);
        	        return(validpath);
	        }

		free(final);
		path = strtok(NULL, ":");
	}

	free(temp);
	free(pathstemp);
	return(NULL);
}
