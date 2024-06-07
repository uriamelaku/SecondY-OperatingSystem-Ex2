#include <iostream>
#include <unistd.h>  // לצורך fork
#include <sys/wait.h>  // לצורך wait

using namespace std;

int main() {
    pid_t pid = fork();

    // father process waits for the child process to finish and prints the child's PID
    if (pid != 0) {
        int status;
        waitpid(pid, &status, 0);
        cout << "Parent process: My child has finished." << endl;
    }


    // loop from one to 10 and print the number and wait between each print one second
    for (int i = 1; i <= 10; i++) {
        cout << i << endl;
        //sleep(1);
    }
      // יצירת תהליך ילד
    if (pid == -1) {
        // שגיאה ביצירת תהליך הילד
        perror("fork");
        return 1;
    } 
    else if (pid == 0) {
        // קוד של תהליך הילד
        cout << "Child process: My PID is " << getpid() << endl;
        //cout << "Child process: I am going to sleep for 2 seconds." << endl;
        //sleep(2);  // הילד ישן ל-2 שניות
        //cout << "Child process: I have finished sleeping." << endl;
        return 0;  // סיום תהליך הילד
    } 
    else {
        // קוד של תהליך ההורה
        cout << "Parent process: My child's PID is " << pid << endl;
        // ההורה מחכה שתהליך הילד יסתיים
        int status;
        waitpid(pid, &status, 0);
        cout << "Parent process: My child has finished." << endl;

        // בדיקה אם הילד הסתיים בהצלחה
        if (WIFEXITED(status)) {
            cout << "Parent process: Child exited with status " << WEXITSTATUS(status) << endl;
        } else {
            cout << "Parent process: Child did not exit successfully." << endl;
        }
    }

    return 0;
}
