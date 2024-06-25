#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <csignal>
#include <thread>
#include <vector>
#include <sys/wait.h>

using namespace std;

int server_fd;
bool running = true;

void timeout_handler(int signum) {
    std::cout << "Timeout reached, closing server." << std::endl;
    running = false;
    close(server_fd);
    exit(0);
}

int createUDPSocket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
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
    cout << "UDP sern port " << port << endl;
    
    return server_fd;
}

void run_command(const char* command, const char* arg) {
    ///////////////////
    /*
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
    execvp(program_name, program_args);
    ///////////////////////////
    */

    std::vector<const char*> args;
    args.push_back(command);
    args.push_back(arg);
    args.push_back(nullptr);
    cout << "Command: " << command << " ====== " << arg << endl;
    
    if (fork() == 0) {
        execvp("./ttt", const_cast<char* const*>(args.data()));
        perror("execvp failed");
        exit(1);
    }
    
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

void udp_server(int port, const char* command, const char* command_arg) {
    struct sockaddr_in udp_address, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024] = {0};

    // Create UDP socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("UDP socket failed");
        exit(EXIT_FAILURE);
    }

    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;
    udp_address.sin_port = htons(port);

    // Bind the UDP socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&udp_address, sizeof(udp_address)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "UDP server listening on port " << port << std::endl;

    run_command(command, command_arg);

    close(server_fd);
}

void udp_server_to_tcp_client(int udp_port, const char* tcp_ip, int tcp_port, const char* command, const char* command_arg) {
    struct sockaddr_in udp_address, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024] = {0};

    // Create UDP socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("UDP socket failed");
        exit(EXIT_FAILURE);
    }

    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;
    udp_address.sin_port = htons(udp_port);

    // Bind the UDP socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&udp_address, sizeof(udp_address)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    // Create TCP socket
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in tcp_address;
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_port = htons(tcp_port);

    if (inet_pton(AF_INET, tcp_ip, &tcp_address.sin_addr) <= 0) {
        perror("Invalid TCP address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(tcp_sock, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) < 0) {
        perror("TCP connection failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "UDP server listening on port " << udp_port << " and sending data to TCP client at " << tcp_ip << ":" << tcp_port << std::endl;

    // Listen for incoming messages
    while (running) {
        int n = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            send(tcp_sock, buffer, strlen(buffer), 0);
            run_command(command, command_arg);
        }
    }

    close(tcp_sock);
    close(server_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: mynetcat -e <command> -i <input> [-o <output>] [-t <timeout>]" << std::endl;
        return 1;
    }

    int timeout = 0;
    int udp_port = 0;
    const char *tcp_ip = nullptr;
    int tcp_port = 0;
    //const char *command = nullptr;
    //const char *command_arg = nullptr;
    bool is_udp_server = false;
    bool has_tcp_client = false;

    // Splitting the second argument into program name and arguments
    char temp[100]; // Assuming max size of program name is 100
    char program_arguments[100]; // Assuming max size of arguments is 100

    char program_name[100];
    char *program_args[100];
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            strcpy(temp, strtok(argv[i+1], " "));
            strcpy(program_arguments, strtok(NULL, "")); // "123456789"

            char program_name[100] = "./";
            strcat(program_name, temp); // "./ttt"
            
            //Creates an array of pointers containing program name, program arguments, and a null terminator.
            char *program_args[] = {program_name, program_arguments, NULL};

        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            if (strncmp(argv[i + 1], "UDPS", 4) == 0) {
                udp_port = atoi(argv[i + 1] + 4); // 4050
                is_udp_server = true;
            }
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            if (strncmp(argv[i + 1], "TCPClocalhost,", 14) == 0) {
                tcp_ip = "127.0.0.1";
                tcp_port = atoi(argv[i + 1] + 14); // 4455
                has_tcp_client = true;
            }
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[i + 1]); // 10
        }
    }

    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout);
    }

    
    int server_fd = createUDPSocket(udp_port);

    struct sockaddr_in client_address;
    socklen_t client_addrlen = sizeof(client_address);
    int client_fd = createUDPSocket(udp_port);

    if (is_udp_server) {
        if (has_tcp_client) {
            //std::thread server_thread(udp_server_to_tcp_client, udp_port, tcp_ip, tcp_port, command, command_arg);
            //server_thread.join();
        } else {
            //handleClientInput(client_fd, program_args, program_name);
        }
    } else {
        std::cerr << "Invalid arguments." << std::endl;
        return 1;
    }

    return 0;
}
