#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define READ_BLOCK_SIZE 128

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Too few arguments. Usage: sys_info <shell_command>\n");
        return EXIT_FAILURE;
    }
    // Create a pipe
    int file_descriptors[2], return_value;
    return_value = pipe(file_descriptors);

    if (return_value == -1)
    {
        // Exit with error if unable to create pipe
        printf("Error creating pipe.\n");
        return EXIT_FAILURE;
    }

    int read_end = file_descriptors[0];
    int write_end = file_descriptors[1];

    // Fork a child
    return_value = fork();
    if (return_value == 1)
    {
        // Exit with error if unable to create child
        printf("Error creating child process.\n");
        return EXIT_FAILURE;
    }

    if (return_value != 0)
    {
        // Parent Process
        printf("Parent PID = %d\n", getpid());
        close(read_end); // close read end from parent

        // Write the first command line argument to pipe
        char *message = argv[1];
        write(write_end, message, strlen(message));

        close(write_end); // close write end from parent

        int status;
        wait(&status);
        if (!WIFEXITED(status))
        {
            printf("Child process exited with an error.\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;

    }
    else
    {
        // Child Process
        printf("Child PID = %d\n", getpid());
        close(write_end); // Close write end from child

        // Read executable path from pipe and execute via a call to execl
        int executable_path_length = 0, executable_path_size = READ_BLOCK_SIZE;
        char *executable_path = malloc(sizeof(char) * executable_path_size);
        if (!executable_path) 
        {
            printf("Error allocating memory.\n");
            return EXIT_FAILURE;
        }
        // Read data from pipe char by char and accumulate the chars in the string executable_path
        char current_char;
        while (read(read_end, &current_char, 1) > 0)
        {
            executable_path[executable_path_length++] = current_char;
            if (executable_path_length > executable_path_size - 1)
            {
                // allocate more memory
                executable_path_size += READ_BLOCK_SIZE;
                char *memory_block = realloc(executable_path, executable_path_size);
                if (!memory_block)
                {
                    printf("Error allocating memory.\n");
                    return EXIT_FAILURE;
                }
                executable_path = memory_block;
            }
        }
        executable_path[executable_path_length+1] = '\0'; // Null terminate the string
        close(read_end); // Close read end of pipe

        // Check if executable is echo and pass hardcoded string "Hello World!"
        if (strcmp("/bin/echo", executable_path) == 0)
        {
            return_value = execl("/bin/echo", "/bin/echo", "Hello World!", NULL);
        }
        else
        {
            return_value = execl(executable_path, executable_path, NULL);
        }
        
        
        if (return_value == -1)
        {
            // There was an error in the exec system call
            // so, the dynamically allocated memory needs to be freed
            free(executable_path); // Cleanup memory
            printf("Error executing shell command provided.\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
        
    } 
    

}
