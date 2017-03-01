
#define FIFO_NAME "/tmp/desk-control"
//#define FIFO_NAME "/var/run/desk-control"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <ext/stdio_filebuf.h>
#include <fstream>
#include <iostream>
#include <poll.h>

#include "control_thread.h"

using namespace std;

int main(int argc, char** argv){
	int res;
	int fifo_fd;

	res = mkfifo(FIFO_NAME, 0666);
	if(res != 0){
		if(errno == EEXIST){
			// fifo exists -> ignoring
		} else{
			cerr << "could not create fifo: " << strerror(errno) << endl;
			return -1;
		}
	}

	ControlThread ct;
	if(ct.stopped()){
		cout << "stopped" << endl;
	}else{
		cout << "not stopped" << endl;
	}

	while(!ct.stopped()){
		//fstream fifo(FIFO_NAME);
		ifstream fifo(FIFO_NAME);
		cout << "opened fifo..." << endl;


		while(fifo.good()){
			string line;

			getline(fifo, line);
			if(!line.empty()){
				cout << "> " << line << endl;

				ct.cmd(line);
				/*
				if(line == "_EXIT"){
					ct.stop();
				} else{
				}
				*/
			}

		}
		cout << "fifo closed." << endl;
	}

	ct.join();

	remove(FIFO_NAME);

	return 0;
}
