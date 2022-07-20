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
// Some global variables
int fd_motor_X;
int fd_motor_Z;

int pid_watchdog;
//pointer to log file
FILE *logfile;


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
void available_commands()
{
    // displays all the instructions
    system("clear");
    printf("\n\n\n  Control Hoist Position \n\n\n");
    printf("        1 - PRESS: W to move hoist UP\n\n");
    printf("        2 - PRESS: S to move hoist DOWN\n\n");
    printf("        3 - PRESS: A to move hoist LEFT\n\n");
    printf("        4 - PRESS: D to move hoist RIGHT\n\n");
    printf("        5 - PRESS: Z to stop Motor Z movement\n\n");
    printf("        6 - PRESS: X to stop Mtor X movement\n\n\n");
    //write on log file
    fprintf(logfile, "Displays all the instructions\n");
    fflush(logfile);
    fflush(stdout);
}

int main(int argc, char *argv[])
{

    //open Log file
    logfile = fopen("Command_Comsole.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Command Console's log File\n");
        return 0;
    }
    fprintf(logfile, "******log file created******\n");
    fflush(logfile);

    // pipe file path
    char *fifo_command_motorX = "/tmp/command_motorX";
    char *fifo_command_motorZ = "/tmp/command_motorZ";
    char *fifo_watchdog_pid = "/tmp/watchdog_pid_c";
    char *fifo_command_pid = "/tmp/pid_c";

    // creating fifo
    mkfifo(fifo_command_motorX, 0666);
    mkfifo(fifo_command_motorZ, 0666);
    mkfifo(fifo_watchdog_pid, 0666);
    mkfifo(fifo_watchdog_pid, 0666);
    fprintf(logfile, "p - fifo files have created and connection has been established\n");

    fflush(logfile);

    char buffer[SIZE];


    //getting watchdog pid
    int fd_watchdog_pid = check(open(fifo_watchdog_pid, O_RDONLY));
    check(read(fd_watchdog_pid, buffer, SIZE));
    pid_watchdog = atoi(buffer);
    fprintf(logfile, "Opened fifo file for watchdog\n");
    fprintf(logfile, "Getting watchdog pid\n");
    fflush(logfile);
    close(fd_watchdog_pid);

    //writing own pid
    int fd_command_pid = check(open(fifo_command_pid, O_WRONLY));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_command_pid, buffer, SIZE));
    close(fd_command_pid);
    fprintf(logfile, "Opened fifo file to write OWN PID\n");
    fprintf(logfile, "Written PID in fifo file\n");
    fflush(logfile);

    char input_ch[80];

    available_commands();

    fd_motor_X = open(fifo_command_motorX, O_WRONLY);
    fprintf(logfile, "Opened fifo file for motorX\n");
    fflush(logfile);
    fd_motor_Z = open(fifo_command_motorZ, O_WRONLY);
    fprintf(logfile, "Opened fifo file for MotorZ\n");
    fflush(logfile);

    while (1)
    {

        scanf("%s", input_ch);

        available_commands();

        if (strlen(input_ch) > 1)
        {
            printf("Unrecognized command, press only one button at a time!\n");
            fflush(stdout);
            fprintf(logfile, "Unrecognized command, press only one button at a time!\n");
            fflush(logfile);
        }

        else
        {
            char out_str[80];
            sprintf(out_str, "%d", input_ch[0]);

            switch (input_ch[0])
            {
            case 'W':
            case 'w':
                //The relevant command send to the motor 
                printf("                   Move the MOTOR_Z UP\n");
                fflush(stdout);
                fprintf(logfile, "p - Move the MOTOR_Z UP\n");
                fflush(logfile);
                check(write(fd_motor_Z, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'S':
            case 's':
                //The relevant command send to the motor 
                printf("                    Move the MOTOR_Z DOWN\n");
                fflush(stdout);
                fprintf(logfile, "p - Move the MOTOR_Z DOWN\n");
                fflush(logfile);
                check(write(fd_motor_Z, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'A':
            case 'a':
                //The relevant command send to the motor 
                printf("                    Move the MOTOR_X LEFT\n");
                fflush(stdout);
                fprintf(logfile, "p - Move the MOTOR_X LEFT\n");
                check(write(fd_motor_X, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'D':
            case 'd':
                //The relevant command send to the motor 
                printf("                    Move the MOTOR_X RIGHT\n");
                fflush(stdout);
                fprintf(logfile, "p - Move the MOTOR_X RIGHT\n");
                check(write(fd_motor_X, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'X':
            case 'x':
                //The relevant command send to the motor 
                printf("                    STOP MOTOR_X\n");
                fflush(stdout);
                fprintf(logfile, "p - STOP MOTOR_X\n");
                check(write(fd_motor_X, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'Z':
            case 'z':
                //The relevant command send to the motor 
                printf("                    STOP MOTOR_Z\n");
                fflush(stdout);
                fprintf(logfile, "p - STOP MOTOR_Z\n");
                check(write(fd_motor_Z, out_str, strlen(out_str) + 1));
                fprintf(logfile, "p - Wrote on fifo file\n");
                fflush(logfile);
                // Signal sent to the watchdog process for monitoring
                kill(pid_watchdog, SIGUSR1);
                break;

            default:
                printf("        Unrecognized command, please try again: \n");
                fflush(stdout);
                fprintf(logfile, "Unrecognized command, please try again: \n");
                fflush(logfile);
                break;
            }
        }
    }
    check(close(fd_motor_X));
    fprintf(logfile, "FILE CLOSED\n");
    fflush(logfile);
    unlink(fifo_command_motorX);
    fprintf(logfile, "p - UNLINKED PIPE FO MOTOR X\n");
    check(close(fd_motor_Z));
    unlink(fifo_command_motorZ);
    fprintf(logfile, "p - UNLINKED PIPE FOR MOTOR Z\n");

    return 0;
}
