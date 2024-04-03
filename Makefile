all: sshell

sgrep: sshell.c
	gcc -02 -Wall -Wextra -Werror -o sshell sshell.c

clean:
	rm -f sshell
