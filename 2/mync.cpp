#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstdlib>

using namespace std;

int main(int argc, char *args[]) {
    // Check for valid command line arguments
    // (the number of arguments is not equal to 3 or the second argument is not "-e").
    if (argc != 3 || strcmp(args[1], "-e") != 0) {
        cout << "Usage: " << args[0] << " -e \"<program> <arguments>\"" << endl;
        return 1;
    }
    // check if the third argument is not in the correct format "<program> <arguments>"
    if (strchr(args[2], ' ') == NULL) {
        cout << "Invalid format for program name and arguments." << endl;
        return 1;
    }

    // Splitting the second argument into program name and arguments
    char program_name[100]; // Assuming max size of program name is 100
    char program_arguments[100]; // Assuming max size of arguments is 100

    // Copy the program name and arguments
    strcpy(program_name, strtok(args[2], " "));
    strcpy(program_arguments, strtok(NULL, ""));

    //Creates an array of pointers containing program name, program arguments, and a null terminator.
    char *program_args[] = {program_name, program_arguments, NULL};

    // Print the parsed program name and arguments
    cout << "Program to execute: " << program_name << endl;
    cout << "Arguments: ";
    if (program_arguments != NULL) {
        cout << program_arguments;
    } 
    else {
        cout << "None";
    }
    cout << endl;

    // Fork a new process
    pid_t process_id = fork();

    if (process_id < 0) {
        
        // Fork failed
        perror("fork failed");
        return 1;

    } 
    else if (process_id == 0) {

        // Child process
        cout << "Child process executing \"" << program_name << "\" with arguments: \"" << (program_arguments ? program_arguments : "") << "\"" << endl;
        
        //Replaces the current process image with a new one, executing the given program with specified arguments.
        execvp(program_name, program_args); 

        // If execvp returns, there was an error
        perror("execvp");
        return 1;
   
    } 
    else {

        // Parent process
        cout << "Parent process (PID: " << getpid() << ") waiting for child process (PID: " << process_id << ") to finish..." << endl;
       
        int status;
        waitpid(process_id, &status, 0); // Wait for child process to finish
       
        if (WIFEXITED(status)) {
            cout << "Child process completed with exit status: " << WEXITSTATUS(status) << endl;
        } 
        else {
            cout << "Child process did not exit normally." << endl;
        }
    }

    return 0;
}