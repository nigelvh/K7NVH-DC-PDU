#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <math.h>

// Define some directories
#define pidfile "/tmp/k7nvh_dc_pdu.pid"

// File handles
int pidFileHandle;
int ttyFileHandle;

int timeout = 1;
int running = 1;

// Buffer and state regarding reading from serial device
char buf [1] = {0};
int num_bytes = 0;

void exit_cleanup(int value){
	// Close file handles before exit
	if(pidFileHandle > 0) close(pidFileHandle);
	if(ttyFileHandle > 0) close(ttyFileHandle);
	
	// Actually exit
	exit(value);
}

void signal_handler(int sig){
	signal(sig, signal_handler);

	switch(sig){
		case SIGALRM:
			running = 0;
			break;
		case SIGHUP:
			fprintf(stderr, "<NOTICE> Received SIGHUP signal.");
			break;
		case SIGINT:
		case SIGTERM:
			fprintf(stderr, "<NOTICE> Received SIGTERM. Exiting.");
			exit_cleanup(0);
			break;
		default:
			fprintf(stderr, "<WARNING> Unhandled signal %s", strsignal(sig));
			break;
	}
}

int main(int argc, char *argv[]) {
	// Check to ensure we've got an argument for the device we want to read from.
	if(argc < 2){
		fprintf(stdout, "Please provide the tty device as an argument. EX: %s /dev/tty.usbSerial0\r\n", argv[0]);
		exit_cleanup(-7);
	}
		
	// Change the file mode mask
	umask(0);
	
	// Check for an existing PID
	pidFileHandle = open(pidfile, O_RDWR|O_CREAT, 0600);
	if(pidFileHandle < 0){
		fprintf(stderr, "<ERROR> PID file exists or could not be created.");
		exit_cleanup(-5);
	}
	
	// Lock the PID file
	if(lockf(pidFileHandle, F_TLOCK, 0) < 0){
		fprintf(stderr, "<ERROR> PID file could not be locked.");
		exit_cleanup(-6);
	}
	
	// Write PID to the PID file
	char str[10];
	sprintf(str, "%d\n", getpid());
	write(pidFileHandle, str, strlen(str));
			
	// Set up the signal handlers
	signal(SIGALRM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	// Try opening the tty device, and error if we can't.
	ttyFileHandle = open(argv[1], O_RDWR | O_NONBLOCK | O_NOCTTY);
	if(ttyFileHandle < 0){
		fprintf(stderr, "<ERROR> Could not open the tty device, %s. Error %d: %s", argv[1], errno, strerror(errno));
		exit_cleanup(-8);
	}

	// Read serial interface attributes
	struct termios tty;
	if(tcgetattr(ttyFileHandle, &tty) != 0){
		fprintf(stderr, "<ERROR> Could not get the tty device attributes. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-13);
	}

	// Set what we want the serial interface attributes to be
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 bit characters
	tty.c_iflag &= ~IGNBRK; // Ignore Breaks
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable hardware flow control
	tty.c_cflag |= (CLOCAL | CREAD); // Local control, enable reads
	tty.c_cflag &= ~(PARENB | PARODD); // No parity
	tty.c_cflag &= ~CSTOPB; 
	tty.c_cflag &= ~CRTSCTS;

	// Set the serial interface attributes
	if(tcsetattr(ttyFileHandle, TCSANOW, &tty) != 0){
		fprintf(stderr, "<ERROR> Could not set the tty device attributes. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-14);
	}

	// Assert and then clear DTR
#ifdef __APPLE__
	if(ioctl(ttyFileHandle, TIOCSDTR) != 0){
		fprintf(stderr, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(ttyFileHandle, TIOCCDTR) != 0){
		fprintf(stderr, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#elif __linux
	int setdtr = TIOCM_DTR;
	if(ioctl(ttyFileHandle, TIOCMBIC, &setdtr) != 0){
		fprintf(stderr, "<ERROR> Could not assert DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-16);
	}
	if(ioctl(ttyFileHandle, TIOCMBIS, &setdtr) != 0){
		fprintf(stderr, "<ERROR> Could not clear DTR. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-17);
	}
#endif

	// Send command to the PDU
	//write(ttyFileHandle, 3, sizeof(char));
	int n = dprintf(ttyFileHandle, "%s\n", argv[2]);
	if(n < 0){
		fprintf(stderr, "<ERROR> Error writing to tty device. Error %d: %s", errno, strerror(errno));
		exit_cleanup(-19);
	}
	
	// Set timeout alarm
	alarm(timeout);
	
	while (running) {
		// Reset the variables
		num_bytes = 0;
		
		// Check for available bytes
		ioctl(ttyFileHandle, FIONREAD, &num_bytes);
		if(num_bytes >= 1){
			// Read a byte from the serial port
			int n = read(ttyFileHandle, &buf, 1);
			
			// Check for an error
			if(n < 0){
				fprintf(stderr, "<ERROR> Error reading from tty device. Error %d: %s", errno, strerror(errno));
				exit_cleanup(-9);
			}
			
			// Check if our byte is a '>' or EOF. If so, break, else print out the char.
			if(buf[0] == '#' || buf[0] == EOF){
				exit_cleanup(0);
			}else{
				fprintf(stdout, "%c", buf[0]);
			}
		}else{
			// Sleep for 20ms (non-blocking wait, lowers CPU time dramatically)
			usleep(20*1000);
		}
	}
	
	// Clean up and exit
	exit_cleanup(0);
}
