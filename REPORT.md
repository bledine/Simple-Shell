# sshell: Simple Shell

## Summary

Simple Shell implements a simplified shell in C, it can execute programs
present on the system and has 4 built-in commands. Sls to print the files in a
directory and their size. Cd to change the directory, pwd to print the
current working directory, and exit to leave the shell. Our Shell also supports
piping and output redirection, where it either appends to a file or overwrites
a file.

## Implementation

This program starts with parsing the input and then separates the command
apart at the spaces and stores the individual strings into an array.

We parse the array of commands and call the relevant functions.

### Parsing

For input parsing, we call the processInput() function; here, we check for
invalid inputs. We store the array of arguments and the size of the array in a
struct called commands. This will be passed to functions in order to complete
various tasks. 

First, the file redirection value of stdout is saved in case the
output needs to be redirected. Then we parse the input for the redirection
sign. If found, the command is split into the command portion, and the
file name portion, which is sent to the relevant functions. Next, the input is
parsed for the piping sign, and the piped functions are called if found. 

We then parse the input for the built-in commands. If they are not present, we
send the command to the system. Stdout is redirected back to the terminal in
case it was redirected earlier. Lastly, the completion message is printed
out and we return to main and wait for further input.

### Build-in Commands

There are 4 built-in commands, and other commands are passed to the OS through
system calls.

#### cd

The cd() function changes the directory using chdir(), it takes up to 2
arguments. If only 1 argument is present (ie. `cd`), it will change to the home
dir. If there are 2 arguments present we change into the specified dir, or else
we return an error.

#### pwd

The pwd() function prints the current directory using the system call getcwd().

#### sls

The sls() function is in charge of opening the current directory, then it reads
all the files in the directory. It iterates through the files and prints them
out with their byte size.

#### exit

Upon receiving the command exit, the shell returns exit status 0.

#### System Commands

If none of the built-in commands are present in the input, we send the
command to the system using the execCommand() function. In the execCommand()
function, we fork the process. The child process executes the command and the
parent process waits for the child and then saves the return status.

### Redirection

First, the file descriptor of stdout is assigned to a variable. The
outputRedirection() function is in charge of redirecting the output. We split
the input at the redirection characters `>` and `>>`, then we take the right
side as the file name and remove all of the spaces. Then, we use the function
filePermissions() to check if we have access to the file or directory; 

If we have access, the file is created if it doesn't exist, and overwritten or
appended to if it exists. Then, stdout is redirected to the file descriptor of
the file. The left side is sent off to the processInput() function, where the
command is executed. stdout is then returned to the terminal in processInput().

### Piping

In processInput() the input is parsed for the number of pipe signs, which with
the command, is sent off to the piping() function. We iterate over the number
of commands, and for each, except for the last command, we create a pipe. Then
we send each command and an array of 2 file descriptors to execPiping() to be
executed. 

In execPipe() we fork the prcocess. The child redirects the input to stdin and
stdout if necessary. Finally, it executes the command; the parent waits for the
child and returns its exit status. Back in piping(), the read end of the pipe
is saved for the next iteration, and the right end of the pipe is closed if it
is not the last command.

### Note about the memory management

The deallocateComand() is in charge of deallocating the memory for our struct
Comand(). All other dynamically allocated memory is freed after being used.
There are, therefore, no memory leaks per se.

### Testing

We used the tester file to check the provided test cases, and we also conducted
rigorous testing of our shell. We tested edge cases to ensure our shell was
performant and returned the correct errors.
