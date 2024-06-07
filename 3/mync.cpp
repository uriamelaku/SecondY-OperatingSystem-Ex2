#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <netdb.h>
#include <errno.h>

using namespace std;

// Function to start a TCP server and listen for input
int startTcpServer(int port) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    cout << "Creating socket..." << endl;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    cout << "Setting socket options..." << endl;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //cout << "Binding socket to port " << port << "..." << endl;
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //cout << "Listening on port " << port << "..." << endl;
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    cout << "Waiting for connection..." << endl;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    cout << "Connection accepted." << endl;
    return new_socket;
}

// Function to start a TCP client and connect for output
int startTcpClient(const string& host, int port) {
    struct sockaddr_in serv_addr;
    int sock = 0;
    struct hostent *he;

    //cout << "Creating socket..." << endl;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    //cout << "Resolving hostname " << host << "..." << endl;
    if ((he = gethostbyname(host.c_str())) == NULL) {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *)he->h_addr);

    cout << "Connecting to " << host << " on port " << port << "..." << endl;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    cout << "Connected to " << host << " on port " << port << "." << endl;
    return sock;
}

// Function to execute the command
void executeCommand(const string& command, int input_fd, int output_fd) {
    int pipeIn[2];
    int pipeOut[2];
    pid_t pid;

    cout << "Creating pipes..." << endl;
    if (pipe(pipeIn) == -1) {
        perror("pipeIn");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipeOut) == -1) {
        perror("pipeOut");
        exit(EXIT_FAILURE);
    }

    //cout << "Forking process..." << endl;
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        //cout << "In child process. Redirecting stdin and stdout..." << endl;
        close(pipeIn[1]); // Close write end of pipeIn
        close(pipeOut[0]); // Close read end of pipeOut

        dup2(pipeIn[0], STDIN_FILENO); // Redirect stdin to pipeIn read end
        dup2(pipeOut[1], STDOUT_FILENO); // Redirect stdout to pipeOut write end

        close(pipeIn[0]);
        close(pipeOut[1]);

        cout << "Executing command: " << command << endl;
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        perror("execl");
        exit(EXIT_FAILURE);
    } 
    else { // Parent process
        close(pipeIn[0]); // Close read end of pipeIn
        close(pipeOut[1]); // Close write end of pipeOut

        fd_set readfds;
        char buffer[1024];

        cout << "In parent process. Entering select loop..." << endl;
        while (true) {
            FD_ZERO(&readfds);
            FD_SET(input_fd, &readfds);
            FD_SET(pipeOut[0], &readfds);
            int max_fd = max(input_fd, pipeOut[0]) + 1;

            int activity = select(max_fd, &readfds, NULL, NULL, NULL);
            if ((activity < 0) && (errno != EINTR)) {
                perror("select error");
                break;
            }

            if (FD_ISSET(input_fd, &readfds)) {
                int valread = read(input_fd, buffer, sizeof(buffer));
                if (valread == 0) {
                    break; // EOF
                }
                //cout << "Read " << valread << " bytes from input_fd. Writing to pipeIn..." << endl;
                write(pipeIn[1], buffer, valread);
            }

            if (FD_ISSET(pipeOut[0], &readfds)) {
                int valread = read(pipeOut[0], buffer, sizeof(buffer));
                if (valread == 0) {
                    break; // EOF
                }
                //cout << "Read " << valread << " bytes from pipeOut. Writing to output_fd..." << endl;
                write(output_fd, buffer, valread);
            }
        }

        close(pipeIn[1]);
        close(pipeOut[0]);
    }
}

// Main function
int main(int argc, char* argv[]) {
    if (argc < 3 || string(argv[1]) != "-e") {
        cerr << "Usage: mync -e <command> [options]\n";
        return 1;
    }

    string command = argv[2];
    string programName, strategy, inputMethod, outputMethod;

    cout << "Parsing arguments..." << endl;
    for (int i = 3; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            inputMethod = argv[++i];   // inputMethod  = TCPS4050
            cout << "Input method: " << inputMethod << endl;
        } 
        else if (arg == "-o" && i + 1 < argc) {
            outputMethod = argv[++i];
            cout << "Output method: " << outputMethod << endl;
        } 
        else if (arg == "-b" && i + 1 < argc) { 
            inputMethod = outputMethod = argv[++i]; // inputMethod = outputMethod = TCPS4050
            cout << "Both input and output in the client: " << inputMethod << endl;
        }
    }

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    if (!inputMethod.empty() && inputMethod.rfind("TCPS", 0) == 0) {
        int port = stoi(inputMethod.substr(4));
        cout << "Starting TCP server on port " << port << endl;
        input_fd = startTcpServer(port);
        cout << "TCP server started and client connected." << endl;
    } 
    else if (!inputMethod.empty() && inputMethod.rfind("TCPC", 0) == 0) {
        string host = inputMethod.substr(4, inputMethod.find(',') - 4);
        int port = stoi(inputMethod.substr(inputMethod.find(',') + 1));
        cout << "Connecting to TCP server at " << host << " on port " << port << endl;
        input_fd = startTcpClient(host, port);
        cout << "Connected to TCP server." << endl;
    }

    if (!outputMethod.empty() && outputMethod.rfind("TCPC", 0) == 0) {
        string host = outputMethod.substr(4, outputMethod.find(',') - 4);
        int port = stoi(outputMethod.substr(outputMethod.find(',') + 1));
        cout << "Connecting to TCP server at " << host << " on port " << port << endl;
        output_fd = startTcpClient(host, port);
        cout << "Connected to TCP server." << endl;
    }

    char *temp = strdup(command.c_str());
    programName = strtok(temp, " ");
    strategy = strtok(NULL, " ");
    free(temp);

    cout << "Program: " << programName << ", Strategy: " << strategy << endl;

    executeCommand(command, input_fd, output_fd);

    return 0;
}
