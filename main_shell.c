#include "shell.h"

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("xsh# ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Fgets failed\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        expand_var(input);
        handle_pipes_and_redirection(input);
    }

    return 0;
}