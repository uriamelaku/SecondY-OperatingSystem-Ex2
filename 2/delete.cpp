//hello world program
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>

using namespace std;

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) {
        cout << "Child pid = "  << getpid() << endl;
        while(1);
    } 
    else if(pid > 0){
        cout << "Parent pid = " << getpid() << endl;
        cout << "Parent process ended" << endl;
    }
}