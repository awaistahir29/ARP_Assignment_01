#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/resource.h>

#define SIZE 80

#define TIMEDELTA 60

//pointer to log file
FILE *logfile;

int fd_starter;
char buffer[SIZE];

int pid_motX;
int pid_motZ;
int pid_inspection;
int pid_command;

time_t time_check;

int check(int retval)
{
    if (retval == -1)
    {
        fprintf(logfile, "\nERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

void signal_handler(int sig)
{
    time_check = time(NULL);
}

int main(int argc, char *argv[])
{
    //open Log file
    logfile = fopen("Watchdog.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Watchdog's log File\n");
        return 0;
    }
    fprintf(logfile, "******log file created******\n");
    fflush(logfile);


    signal(SIGUSR1, signal_handler);

    char *fifo_watchdog_pid_command = "/tmp/watchdog_pid_c";
    char *fifo_watchdog_pid_inspection = "/tmp/watchdog_pid_i";
    char *fifo_watchdog_pid_motX = "/tmp/watchdog_pid_x";
    char *fifo_watchdog_pid_motZ = "/tmp/watchdog_pid_z";
    char *fifo_command_pid = "/tmp/pid_c";
    char *fifo_inspection_pid = "/tmp/pid_i";
    char *fifo_motZ_pid = "/tmp/pid_z";
    char *fifo_motX_pid = "/tmp/pid_x";

    mkfifo(fifo_watchdog_pid_command, 0666);
    mkfifo(fifo_watchdog_pid_inspection, 0666);
    mkfifo(fifo_watchdog_pid_motX, 0666);
    mkfifo(fifo_watchdog_pid_motZ, 0666);
    mkfifo(fifo_command_pid, 0666);
    mkfifo(fifo_inspection_pid, 0666);
    mkfifo(fifo_motX_pid, 0666);
    mkfifo(fifo_motZ_pid, 0666);

    //Writing in log file
    fprintf(logfile, "p - FIFO files connection have been estabished\n");
    fflush(logfile);

    //sending pid to other processes
    int fd_watchdog_pid_command = check(open(fifo_watchdog_pid_command, O_WRONLY));
    int fd_watchdog_pid_inspection = check(open(fifo_watchdog_pid_inspection, O_WRONLY));
    int fd_watchdog_pid_motX = check(open(fifo_watchdog_pid_motX, O_WRONLY));
    int fd_watchdog_pid_motZ = check(open(fifo_watchdog_pid_motZ, O_WRONLY));
    

    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_watchdog_pid_command, buffer, strlen(buffer) + 1));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_watchdog_pid_inspection, buffer, strlen(buffer) + 1));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_watchdog_pid_motX, buffer, strlen(buffer) + 1));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_watchdog_pid_motZ, buffer, strlen(buffer) + 1));

    check(close(fd_watchdog_pid_command));
    unlink(fifo_watchdog_pid_command);
    check(close(fd_watchdog_pid_inspection));
    unlink(fifo_watchdog_pid_inspection);
    check(close(fd_watchdog_pid_motX));
    unlink(fifo_watchdog_pid_motX);
    check(close(fd_watchdog_pid_motZ));
    unlink(fifo_watchdog_pid_motZ);


    //Writing in log file
    fprintf(logfile, "p - Sending PID to the other processes\n");
    fflush(logfile);



    //reading other processes's pid
    int fd_pid_command = check(open(fifo_command_pid, O_RDONLY));
    int fd_pid_inspection = check(open(fifo_inspection_pid, O_RDONLY));
    int fd_pid_motX = check(open(fifo_motX_pid, O_RDONLY));
    int fd_pid_motZ = check(open(fifo_motZ_pid, O_RDONLY));
    check(read(fd_pid_command,buffer,SIZE));
    pid_command=atoi(buffer);
    check(read(fd_pid_inspection,buffer,SIZE));
    pid_inspection=atoi(buffer);
    check(read(fd_pid_motX,buffer,SIZE));
    pid_motX=atoi(buffer);
    check(read(fd_pid_motZ,buffer,SIZE));
    pid_motZ=atoi(buffer);

    //Writing in log file
    fprintf(logfile, "p - Reading other processes's PID\n");
    fflush(logfile);



    time_check = time(NULL);

    while (1)
    {
        sleep(1);
        ////Writing in log file
        fprintf(logfile, "p - Sleeping\n");
        fflush(logfile);
        if (difftime(time(NULL), time_check) >= TIMEDELTA)
        {   
            fprintf(logfile, "-------------Time Out------------\n");

            kill(pid_motX, SIGUSR2);

            ////Writing in log file
            fprintf(logfile, "p - MotorX process has killed\n");
            fflush(logfile);

            kill(pid_motZ, SIGUSR2);

            //Writing in log file
            fprintf(logfile, "p - MotorZ process has killed\n");
            fflush(logfile);

            time_check = time(NULL);
        }
    }

    return 0;
}