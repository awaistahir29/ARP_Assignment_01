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

//pointer to log file
FILE *logfile;

float position = 0;             //position of the Z motor
float max_z = 1;                //maximum porition of the motorZ
float movement_distance = 0.10; //Displacement, after command inserted
float movement_time = 1;        //Time while executing one instance

int fd_inspection;
int fd_command;
int fd_motorZ;

char buffer[SIZE];
char last_input_command[SIZE];
char last_input_inspection[SIZE];

int pid_watchdog;

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

void sigusr1_handler(int sig)
{
    sprintf(buffer, "%f", position);
    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
    strcpy(last_input_inspection, "");
    strcpy(last_input_command, "");
}

void sigusr2_handler(int sig)
{
    sprintf(last_input_inspection, "%d", 'r');
}

int main(int argc, char *argv[])
{
    //open Log file
    logfile = fopen("MotorZ.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating MotorZ's log File\n");
        return 0;
    }
    fprintf(logfile, "******log file created******\n");
    fflush(logfile);

    //Writing in log file
    fprintf(logfile, "p - position of the Z motor\n");
    fprintf(logfile, "p - maximum position of Z motor\n");
    fprintf(logfile, "p - the amount of movement made after receiving a command\n");
    fprintf(logfile, "p - the amount of seconds needed to do the movement\n");
    fflush(logfile);

    //randomizing seed for random error generator
    srand(time(NULL));
    fflush(stdout);

    //Writing in log file
    fprintf(logfile, "randomizing seed for random error generator\n");
    fflush(logfile);

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    //pipe file path
    char *fifo_command_motorZ = "/tmp/command_motorZ";
    char *fifo_inspection_motorZ = "/tmp/inspection_motorZ";
    char *fifo_motorZ_value = "/tmp/motorZ_value";
    char *fifo_watchdog_pid = "/tmp/watchdog_pid_z";
    char *fifo_motZ_pid = "/tmp/pid_z";
    char *fifo_motZ_pid_inspection = "/tmp/pid_z_i";

    mkfifo(fifo_inspection_motorZ, 0666);
    mkfifo(fifo_command_motorZ, 0666);
    mkfifo(fifo_motorZ_value, 0666);
    mkfifo(fifo_watchdog_pid, 0666);
    mkfifo(fifo_motZ_pid, 0666);
    mkfifo(fifo_motZ_pid_inspection,0666);

    //Writing in log file
    fprintf(logfile, "p - FIFO connections have been established\n");
    fflush(logfile);

    int fd_watchdog_pid = check(open(fifo_watchdog_pid, O_RDONLY));
    check(read(fd_watchdog_pid, buffer, SIZE));
    pid_watchdog = atoi(buffer);

    check(close(fd_watchdog_pid));

    struct timeval timeout;
    fd_set readfds;

    float random_error;
    float movement;

    fd_command = check(open(fifo_command_motorZ, O_RDONLY));
    fd_inspection = check(open(fifo_inspection_motorZ, O_RDONLY));

    //writing own pid for inspection console
    int fd_motZ_pid_i = check(open(fifo_motZ_pid_inspection, O_WRONLY));
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_motZ_pid_i, buffer, SIZE));
    check(close(fd_motZ_pid_i));

    //Writing in log file
    fprintf(logfile, "p - writing own pid for inspection console\n");
    fflush(logfile);

    //writing own pid
    int fd_motZ_pid = open(fifo_motZ_pid, O_WRONLY);
    sprintf(buffer, "%d", (int)getpid());
    check(write(fd_motZ_pid, buffer, SIZE));
    sprintf(buffer, "%f", position);
    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
    check(close(fd_motZ_pid));

    //Writing in log file
    fprintf(logfile, "p - writing own pid\n");
    fflush(logfile);

    fd_motorZ = check(open(fifo_motorZ_value, O_WRONLY));

    system("clear");
    while (1)
    {

        //setting timout microseconds to 0
        timeout.tv_usec = 100000;

        //Writing in log file
        fprintf(logfile, "p - setting timout microseconds to 0\n");
        fflush(logfile);
        //initialize with an empty set the file descriptors set
        FD_ZERO(&readfds);

        //Writing in log file
        fprintf(logfile, "p - initialize with an empty set the file descriptors set\n");
        fflush(logfile);

        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_command, &readfds);
        FD_SET(fd_inspection, &readfds);

        //Writing in log file
        fprintf(logfile, "p - add the selected file descriptor to the selected fd_set\n");
        fflush(logfile);

        //generating a small random error between -0.02 and 0.02
        random_error = (float)(-20 + rand() % 40) / 1000;

        //Writing in log file
        fprintf(logfile, "p - generating a small random error between -0.02 and 0.02\n");
        fflush(logfile);

        
        //select return -1 in case of error, 0 if timeout reached, or the number of ready descriptors
        switch (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout))
        {
        case 0: //timeout reached, so nothing new

            switch (atoi(last_input_command))
            {
            case 'S':
            case 's':
                // down

                movement = -movement_distance + random_error;
                if (position + movement < 0)
                {
                    position = 0;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    strcpy(last_input_command, "");
                    sleep(movement_time);
                }
                else
                {
                    position += movement;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    sleep(movement_time);
                }
                kill(pid_watchdog, SIGUSR1);
                break;

            case 'W':
            case 'w':
                //up

                movement = movement_distance + random_error;
                if (position + movement > max_z)
                {
                    position = max_z;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    strcpy(last_input_command, "");
                    sleep(movement_time);
                }
                else
                {
                    position += movement;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    sleep(movement_time);
                }
                kill(pid_watchdog, SIGUSR1);
                break;
            case 'Z':
            case 'z':
                check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                kill(pid_watchdog, SIGUSR1);
                strcpy(last_input_command, "");
                sleep(movement_time);
                break;
            default:
                break;
            }

            if(atoi(last_input_inspection)=='r' || atoi(last_input_inspection)=='R' ) {
                movement = -(5 * movement_distance) + random_error;
                if (position + movement <= 0)
                {
                    position = 0;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    strcpy(last_input_inspection, "");
                    kill(pid_watchdog, SIGUSR1);
                    sleep(movement_time);
                }
                else
                {
                    position += movement;
                    sprintf(buffer, "%f", position);
                    check(write(fd_motorZ, buffer, strlen(buffer) + 1));
                    kill(pid_watchdog, SIGUSR1);
                    sleep(movement_time);
                }
            }
            break;
        case -1: //error
            perror("Error inside motorZ: ");
            fflush(stdout);

            //Writing in log file
            fprintf(logfile, "p - Error inside motorZ:\n");
            fflush(logfile);

            
            break;
        default: //if something is ready, we read it
            if (FD_ISSET(fd_command, &readfds))
                check(read(fd_command, last_input_command, SIZE));

                //Writing in log file
                fprintf(logfile, "p - Reading if something ready\n");
                fflush(logfile);
            if (FD_ISSET(fd_inspection, &readfds))
            {

                check(read(fd_inspection, last_input_inspection, SIZE));
                strcpy(last_input_command, "");

                //Writing in log file
                fprintf(logfile, "p - Reading if something ready\n");
                fflush(logfile);
            }
            break;
        }
    }
    check(close(fd_command));
    unlink(fifo_command_motorZ);
    check(close(fd_inspection));
    unlink(fifo_inspection_motorZ);
    check(close(fd_motorZ));
    unlink(fifo_motorZ_value);

    //Writing in log file
    fprintf(logfile, "p - Files have closed and unlinked\n");
    fflush(logfile);

    return 0;
}