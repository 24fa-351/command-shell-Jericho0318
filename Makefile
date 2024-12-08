shell: shell.c main_shell.c
	gcc -o shell shell.c main_shell.c

clean:
	rm shell