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

// Some Global Variable 
int fd_motor_Z_value;
int fd_stdin;
int fd_inspection;
int pid_motorX;
int pid_motorZ;
int pid_watchdog;
float position_x = 0;
float position_z = 0;
int fd_motor_X;
int fd_motor_Z;
int fd_motor_X_value;



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

void motor_state()
{

    system("clear");
    printf("\n\n\n                X position: %.3f\n", position_x);
    printf("\n\n\n                Z position: %.3f\n", position_z);
    printf("\n\n\n\n    PRESS R TO RESET THE HOIST, PRESS S FOR EMERGENCY STOP\n\n");

    fflush(stdout);
}

int main(int argc, char *argv[])
{

    //open Log file
    logfile = fopen("Inspection_Console.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Command Console's log File\n");
        return 0;
    }
    fprintf(logfile, "******log file created******\n");
    fflush(logfile);

    struct timeval timeout;
    timeout.tv_usec = 50;
    fd_set readfds;
    fd_set read_stdinfds;
    char input_ch[SIZE];
    char value_from_motor_x[SIZE];
    char value_from_motor_z[SIZE];
    char buffer[SIZE];

    char *fifo_inspection_motorX = "/tmp/inspection_motorX";
    char *fifo_inspection_motorZ = "/tmp/inspection_motorZ";
    char *fifo_motorX_value = "/tmp/motorX_value";
    char *fifo_motorZ_value = "/tmp/motorZ_value";
    char *fifo_watchdog_pid = "/tmp/watchdog_pid_i";
    char *fifo_inspection_pid = "/tmp/pid_i";
    char *fifo_motX_pid_inspection = "/tmp/pid_x_i";
    char *fifo_motZ_pid_inspection = "/tmp/pid_z_i";

    mkfifo(fifo_inspection_motorX, 0666);
    mkfifo(fifo_inspection_motorZ, 0666);
    mkfifo(fifo_motorX_value, 0666);
    mkfifo(fifo_motorZ_value, 0666);
    mkfifo(fifo_watchdog_pid, 0666);
    mkfifo(fifo_inspection_pid, 0666);
    mkfifo(fifo_motX_pid_inspection, 0666);
    mkfifo(fifo_motZ_pid_inspection, 0666);
    fprintf(logfile, "p - fifo files have created\n");
    fflush(logfile);

    //getting watchdog pid
    int fd_watchdog_pid = check(open(fifo_watchdog_pid, O_RDONLY));
    check(read(fd_watchdog_pid, buffer, SIZE));
    pid_watchdog = atoi(buffer);
    
    check(close(fd_watchdog_pid));

    //Writing Log File
    fprintf(logfile, "Opened fifo file for watchdog\n");
    fprintf(logfile, "Getting watchdog pid\n");
    fflush(logfile);

    //writing own pid
    int fd_inspection_pid = check(open(fifo_inspection_pid, O_WRONLY));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_inspection_pid, buffer, SIZE));
    check(close(fd_inspection_pid));

    //Writing Log File
    fprintf(logfile, "Opened fifo file to write OWN PID\n");
    fprintf(logfile, "Written PID in fifo file\n");
    fflush(logfile);

    fd_motor_X = check(open(fifo_inspection_motorX, O_WRONLY));
    fd_motor_Z = check(open(fifo_inspection_motorZ, O_WRONLY));
    fd_stdin = fileno(stdin);
    int fd_motX_pid_i = check(open(fifo_motX_pid_inspection, O_RDONLY));
    int fd_motZ_pid_i = check(open(fifo_motZ_pid_inspection, O_RDONLY));

    //getting motors pid
    check(read(fd_motX_pid_i, buffer, SIZE));
    pid_motorX = atoi(buffer);
    check(read(fd_motZ_pid_i, buffer, SIZE));
    pid_motorZ = atoi(buffer);
    check(close(fd_motX_pid_i));
    check(close(fd_motZ_pid_i));

    fd_motor_X_value = check(open(fifo_motorX_value, O_RDONLY));
    fd_motor_Z_value = check(open(fifo_motorZ_value, O_RDONLY));

    motor_state();

    while (1)
    {

        //setting timout microseconds to 0
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        //initialize with an empty set the file descriptors set
        FD_ZERO(&readfds);
        FD_ZERO(&read_stdinfds);

        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_motor_X_value, &readfds);
        FD_SET(fd_motor_Z_value, &readfds);
        FD_SET(fileno(stdin), &read_stdinfds);

        switch (select(FD_SETSIZE + 1, &read_stdinfds, NULL, NULL, &timeout))
        {
        case 0: //timeout reached, so nothing new

            break;

        case -1: //error
            perror("Error inside inspectionConsole read: ");
            fflush(stdout);
            break;
        default: //if something is ready, we read it
            check(read(fd_stdin, input_ch, SIZE));
            input_ch[strcspn(input_ch, "\n")] = 0; //trims the \n command read by user input
            fflush(stdin);

            if (strlen(input_ch) > 1)
            {
                motor_state();
                printf("Unrecognized command, press only one button at a time!\n");
                fflush(stdout);
            }

            else
            {
                char out_str[80];
                sprintf(out_str, "%d", input_ch[0]);

                switch (input_ch[0])
                {
                case 'R': //R
                case 'r': //r
                    motor_state();

                    //if a command is recognized, the command is sent to the relative motor and a signal sent to watchdog
                    printf("RESET PRESSED\n");
                    fflush(stdout);
                    check(write(fd_motor_X, out_str, strlen(out_str) + 1));
                    check(write(fd_motor_Z, out_str, strlen(out_str) + 1));
                    kill(pid_watchdog, SIGUSR1);
                    break;

                case 'S': //S
                case 's': //s
                    motor_state();

                    //if a command is recognized, the command is sent to the relative motor and a signal sent to watchdog
                    printf("EMERGENCY STOP PRESSED\n");
                    fflush(stdout);
                    kill(pid_motorX, SIGUSR1);
                    kill(pid_motorZ, SIGUSR1);
                    kill(pid_watchdog, SIGUSR1);
                    break;

                default:
                    motor_state();

                    printf("Unrecognized command, please try again: \n");
                    fflush(stdout);
                    break;
                }
            }
            break;
        }

        //check if one of the motor has sent a new position
        switch (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout))
        {
        case 0: //timeout reached, so nothing new

            break;

        case -1: //error
            perror("Error inside inspectionConsole");
            fflush(stdout);
            break;
        default: //if something is ready, we read it
            if (FD_ISSET(fd_motor_X_value, &readfds))
            {
                //update position
                check(read(fd_motor_X_value, value_from_motor_x, SIZE));
                position_x = atof(value_from_motor_x);
            }
            if (FD_ISSET(fd_motor_Z_value, &readfds))
            {
                //update position
                check(read(fd_motor_Z_value, value_from_motor_z, SIZE));
                position_z = atof(value_from_motor_z);
            }

            motor_state();
            break;
        }
    }

    check(close(fd_motor_X));
    unlink(fifo_inspection_motorX);
    check(close(fd_motor_Z));
    unlink(fifo_inspection_motorZ);
    check(close(fd_motor_X_value));
    unlink(fifo_motorX_value);
    check(close(fd_motor_Z_value));
    unlink(fifo_motorZ_value);

    return 0;
}