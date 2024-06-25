#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[]) {
    // Check for valid command line arguments
    // (the number of arguments is not equal to 3 or the second argument is not "-e").
    if (argc != 3 || strcmp(argv[1], "-e") != 0) {
        cout << "Usage: " << argv[0] << " -e \"<program> <arguments>\"" << endl;
        return 1;
    }
    // check if the third argument is not in the correct format "<program> <arguments>"
    if (strchr(argv[2], ' ') == NULL) {
        cout << "Invalid format for program name and arguments." << endl;
        return 1;
    }

    // Splitting the second argument into program name and arguments
    char temp[100]; // Assuming max size of program name is 100
    char program_arguments[100]; // Assuming max size of arguments is 100

    

    // Copy the program name and arguments
    cout << "argv[2]: " << argv[2] << endl;
    strcpy(temp, strtok(argv[2], " "));
    strcpy(program_arguments, strtok(NULL, ""));
 
    char program_name[100] = "./";
    strcat(program_name,temp);


    //Creates an array of pointers containing program name, program arguments, and a null terminator.
    char *program_args[] = {program_name, program_arguments, NULL};

    // Print the parsed program name and arguments
    cout << "Program to execute: " << program_name << endl;
    cout << "Arguments: " << program_arguments << endl;

    // Fork a new process
    pid_t process_id = fork();

    if (process_id < 0) 
    {
        
        // Fork failed
        perror("fork failed");
        return 1;

    }

    if (process_id == 0) 
    {
        // Child process
        cout << "Child process.." << endl;
        
        //Replaces the current process image with a new one, executing the given program with specified arguments.
        execvp(program_name, program_args);

        // If execvp returns, there was an error
        perror("execvp");
        return 1;
   
    } 
    else 
    {
        // Parent process
        cout << "Parent process.." << endl;

        wait(NULL); // Wait for child process to finish
    }

    return 0;
}