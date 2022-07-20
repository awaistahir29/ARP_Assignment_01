#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>

#define SIZE 80

//pointer to log file
FILE *logfile;

int spawn(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
        return child_pid;
    else
    {
        execvp(program, arg_list);

        perror("exec failed");

        //Writing in log file
        fprintf(logfile, "p - exec failed\n");
        fflush(logfile);

        return 1;
    }
}

int pid_command;
int pid_inspection;
int pid_motorX;
int pid_motorZ;
int pid_watchdog;
char buffer[SIZE];

int fd_watchdog;
int fd_inspection;

int main(int argc, char *argv[])
{

    //open Log file
    logfile = fopen("Starter.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Starter's log File\n");
        return 0;
    }
    fprintf(logfile, "******log file created******\n");
    fflush(logfile);

    char console[] = "/usr/bin/konsole";
    char *arg_list_1[] = {"/usr/bin/konsole", "-e", "./commandConsole", "", (char *)NULL};
    char *arg_list_2[] = {"/usr/bin/konsole", "-e", "./inspectionConsole", "", (char *)NULL};
    char *arg_list_3[] = { "./motorX", "", (char *)NULL};
    char *arg_list_4[] = { "./motorZ", "", (char *)NULL};
    char *arg_list_5[] = { "./watchdog", "", (char *)NULL};

    pid_motorX = spawn("./motorX", arg_list_3);

    //Writing in log file
    fprintf(logfile, "p - MotorX process has been spawned\n");
    fflush(logfile);

    pid_motorZ = spawn("./motorZ", arg_list_4);

    //Writing in log file
    fprintf(logfile, "p - MotorZ process has been spawned\n");
    fflush(logfile);

    pid_command = spawn(console, arg_list_1);

    //Writing in log file
    fprintf(logfile, "p - Command Consle process has been spawned\n");
    fflush(logfile);

    pid_inspection = spawn(console, arg_list_2);

    //Writing in log file
    fprintf(logfile, "p - Inspection Console process has been spawned\n");
    fflush(logfile);

    pid_watchdog = spawn("./watchdog", arg_list_5);

    //Writing in log file
    fprintf(logfile, "p - Watchdog process has been spawned\n");
    fflush(logfile);



    return 0;
}
