#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>

using namespace std;

int createServerSocket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    return server_fd;
}

void handleClientInput(int client_fd, char *program_args[], const char *program_name) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        close(pipefd[1]); // Close the write end of the pipe in the child

        // Redirect stdin of the child process to the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // Replaces the current process image with a new one, executing the given program with specified arguments.
        execvp(program_name, program_args);

        // If execvp returns, there was an error
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipefd[0]); // Close the read end of the pipe in the parent

        char buffer[1024];
        int bytes_read;

        while (true) {
            // Reading data from the client
            bytes_read = read(client_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                if (bytes_read < 0) {
                    perror("read");
                }
                cout << "Client disconnected" << endl;
                break;
            }

            // Writing the received data to the pipe (which is the stdin of the child process)
            if (write(pipefd[1], buffer, bytes_read) < 0) {
                perror("write to pipe");
                break;
            }
        }

        close(pipefd[1]); // Close the write end of the pipe
        wait(NULL); // Wait for child process to finish
    }
}

void handleClientInputOutput(int client_fd, char *program_args[], const char *program_name) {
    int pipefd_in[2];
    int pipefd_out[2];
    if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        close(pipefd_in[1]);  // Close the write end of the input pipe
        close(pipefd_out[0]); // Close the read end of the output pipe

        // Redirect stdin and stdout
        dup2(pipefd_in[0], STDIN_FILENO);
        dup2(pipefd_out[1], STDOUT_FILENO);

        // Close the original pipe file descriptors
        close(pipefd_in[0]);
        close(pipefd_out[1]);

        // Execute the command
        execvp(program_name, program_args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipefd_in[0]);  // Close the read end of the input pipe
        close(pipefd_out[1]); // Close the write end of the output pipe

        char buffer[1024];
        int bytes_read;

        fd_set read_fds;
        int max_fd = max(client_fd, pipefd_out[0]) + 1;

        while (true) {
            FD_ZERO(&read_fds);
            FD_SET(client_fd, &read_fds);
            FD_SET(pipefd_out[0], &read_fds);

            int activity = select(max_fd, &read_fds, NULL, NULL, NULL);
            if (activity < 0 && errno != EINTR) {
                perror("select");
                break;
            }

            if (FD_ISSET(client_fd, &read_fds)) {
                bytes_read = read(client_fd, buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    if (bytes_read < 0) {
                        perror("read");
                    }
                    cout << "Client disconnected" << endl;
                    break;
                }
                write(pipefd_in[1], buffer, bytes_read);
            }

            if (FD_ISSET(pipefd_out[0], &read_fds)) {
                bytes_read = read(pipefd_out[0], buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    if (bytes_read < 0) {
                        perror("read");
                    }
                    break;
                }
                write(client_fd, buffer, bytes_read);
            }
        }

        close(pipefd_in[1]);  // Close the write end of the input pipe
        close(pipefd_out[0]); // Close the read end of the output pipe

        // Wait for child process to finish
        wait(NULL);
    }
}

void handleClientInputOutputSeparate(int client_fd_in, int client_fd_out, char *program_args[], const char *program_name) {
    int pipefd_in[2];
    int pipefd_out[2];
    if (pipe(pipefd_in) == -1 || pipe(pipefd_out) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        close(pipefd_in[1]);  // Close the write end of the input pipe
        close(pipefd_out[0]); // Close the read end of the output pipe

        // Redirect stdin and stdout
        dup2(pipefd_in[0], STDIN_FILENO);
        dup2(pipefd_out[1], STDOUT_FILENO);

        // Close the original pipe file descriptors
        close(pipefd_in[0]);
        close(pipefd_out[1]);

        // Execute the command
        execvp(program_name, program_args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        close(pipefd_in[0]);  // Close the read end of the input pipe
        close(pipefd_out[1]); // Close the write end of the output pipe

        char buffer[1024];
        int bytes_read;

        fd_set read_fds;
        int max_fd = max(client_fd_in, pipefd_out[0]) + 1;

        while (true) {
            FD_ZERO(&read_fds);
            FD_SET(client_fd_in, &read_fds);
            FD_SET(pipefd_out[0], &read_fds);

            int activity = select(max_fd, &read_fds, NULL, NULL, NULL);
            if (activity < 0 && errno != EINTR) {
                perror("select");
                break;
            }

            if (FD_ISSET(client_fd_in, &read_fds)) {
                bytes_read = read(client_fd_in, buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    if (bytes_read < 0) {
                        perror("read");
                    }
                    cout << "Client (input) disconnected" << endl;
                    break;
                }
                write(pipefd_in[1], buffer, bytes_read);
            }

            if (FD_ISSET(pipefd_out[0], &read_fds)) {
                bytes_read = read(pipefd_out[0], buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    if (bytes_read < 0) {
                        perror("read");
                    }
                    break;
                }
                write(client_fd_out, buffer, bytes_read);
            }
        }

        close(pipefd_in[1]);  // Close the write end of the input pipe
        close(pipefd_out[0]); // Close the read end of the output pipe

        // Wait for child process to finish
        wait(NULL);
    }
}



int main(int argc, char* argv[]) {
    // Check for valid command line arguments
    // if (argc != 5 || strcmp(argv[1], "-e") != 0 || (strcmp(argv[3], "-i") != 0 && strcmp(argv[3], "-b") != 0)) {
    //     cout << "Usage: " << argv[0] << " -e \"<program> <arguments>\" -i/-b <port>" << endl;
    //     return 1;
    // }

    
    int port = stoi(string(argv[4]).substr(4));  // Extract port number
    int server_fd = createServerSocket(port);

    struct sockaddr_in client_address;
    socklen_t client_addrlen = sizeof(client_address);
    int client_fd;
    cout << "Waiting for connection..." << endl;

    // Splitting the second argument into program name and arguments
    char temp[100]; // Assuming max size of program name is 100
    char program_arguments[100]; // Assuming max size of arguments is 100

    // Copy the program name and arguments
    strcpy(temp, strtok(argv[2], " "));
    strcpy(program_arguments, strtok(NULL, ""));

    char program_name[100] = "./";
    strcat(program_name, temp);

    //Creates an array of pointers containing program name, program arguments, and a null terminator.
    char *program_args[] = {program_name, program_arguments, NULL};

    // Accepting a new connection
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    cout << "New connection accepted" << endl;

    // these three if for the flags - -i, -b or -o
    if (strcmp(argv[3], "-i") == 0 && argc == 5) {

        // the function that handles only input
        handleClientInput(client_fd, program_args, program_name);
    } 
    if (strcmp(argv[3], "-b") == 0) {

        // the function that handles both input and output
        handleClientInputOutput(client_fd, program_args, program_name);
    }
    else if (strcmp(argv[5], "-o") == 0) {

        // Create a new connection to the output server. only for -o flag
        int client_fd_out = -1;
        struct sockaddr_in serv_addr;
        if ((client_fd_out = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            return 1;
        }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(4455);
        string outputHost = "127.0.0.1";
        if (inet_pton(AF_INET, outputHost.c_str(), &serv_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            return 1;
        }
        if (connect(client_fd_out, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection Failed");
            return 1;
        }
        cout << "Output connection established" << endl;

        // the function that handles both input and output separately
        handleClientInputOutputSeparate(client_fd, client_fd_out, program_args, program_name);
    
    
    }

    // Closing the connection with the client and the server
    close(client_fd);
    close(server_fd);

    return 0;
}
