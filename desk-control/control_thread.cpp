#include "control_thread.h"

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "../usb2lin06.hpp"

#define MIN_HEIGHT 0
#define MAX_HEIGHT 52

using namespace std;
using namespace usb2lin06;

const map<string, ControlThread::Command>
ControlThread::s_commandStr = {
	{"status", Command::status},
	{"STATUS", Command::status},
	{"up", Command::up},
	{"UP", Command::up},
	{"down", Command::down},
	{"DOWN", Command::down},
	{"stop", Command::stop},
	{"STOP", Command::stop},
	{"exit", Command::exit},
	{"EXIT", Command::exit},
};

ControlThread::ControlThread():
	m_thread(&ControlThread::run, this),
	m_udev(nullptr),
	m_stop(false)
{
	if(libusb_init(0)!=0)
	{
		cout << "exiting" << endl;
		fprintf(stderr, "Error failed to init libusb");
		throw ControlException("could not initialize libusb");
	}
	libusb_set_debug(0,LIBUSB_LOG_LEVEL_WARNING);//and let usblib be verbose

    /*
	m_udev = usb2lin06::openDevice();
	if(m_udev == NULL )
	{
 		fprintf(stderr, "Error NO device");
		throw ControlException("could not USB device");
	}
	initDevice(m_udev);
	*/
	unique_lock<mutex> lck(m_cmdMutex);
	m_newCommand = true;
	lck.unlock();
	m_cmdCondition.notify_all();
}

ControlThread::~ControlThread()
{
	libusb_close(m_udev);
	libusb_exit(0);
}

ControlThread::StatusRecord 
ControlThread::cmd(std::string& cmd_line)
{
	StatusRecord sr;

	sr.currentHeight = m_currentHeight;
	sr.operationState = m_operationState;

	cout << __func__ << ": " << cmd_line << endl;
	string cmd;

	istringstream is(cmd_line);
	getline(is, cmd, ' ');

	try{
		Command c = s_commandStr.at(cmd);

		cout << "command=" << cmd << " (" << (int)c << ")" << endl;

		unique_lock<mutex> lck(m_cmdMutex);
		switch(c){
			case Command::status:
				cout << "reading status..." << endl;
				sr.currentHeight = getHeight();
				m_newCommand = true;
				break;
			case Command::up:
				cout << "moving up..." << endl;
				m_targetHeight = MAX_HEIGHT;
				m_newCommand = true;
				break;
			case Command::down:
				cout << "moving down..." << endl;
				m_targetHeight = MIN_HEIGHT;
				m_newCommand = true;
				break;
			case Command::stop:
				cout << "stop moving" << endl;
				m_targetHeight = getHeight();
				m_newCommand = true;
				break;
			case Command::exit:
				cout << "exit..." << endl;
				m_newCommand = true;
				m_stop = true;
				break;
		}

		//m_cmd = c;
		lck.unlock();
		m_cmdCondition.notify_all();

	} catch(out_of_range& e){
	}

	return sr;
}

uint16_t
ControlThread::getHeight()
{
	return 0;
	statusReport str;
	getStatusReport(m_udev, str);

	//m_currentHeight = (
	return getHeightInCM(str);
}

void
ControlThread::run()
{
	unique_lock<mutex> lck(m_cmdMutex);
	while(!m_newCommand) m_cmdCondition.wait(lck);
	lck.unlock();
	cout << "newCommand = false" << endl;
	m_newCommand = false;
	//while(!m_stop){
	do{

		/*
		*/


		m_currentHeight = getHeight();

		if(m_targetHeight == m_currentHeight){
			// stop
			// TODO: how to stop?

			// wait for new command
			cout << "#1" << endl;
			unique_lock<mutex> lck(m_cmdMutex);
			cout << "#2" << endl;
			while(!m_newCommand){
				cout << "waiting condition" << endl;
				m_cmdCondition.wait(lck);
			}
			lck.unlock();
		} else if(m_currentHeight < m_targetHeight){
			//moveUp(m_udev);
			cout << "move UP" << endl;
		} else{
			//moveDown(m_udev);
			cout << "move DOWN" << endl;
		}

		if(m_stop){
			// stop now...
			//break;
		}

		cout << __func__ << "..." << endl;

		cout << "newCommand = false" << endl;
		m_newCommand = false;
	} while(!m_stop);
	cout << __func__ << " done." << endl;

	kill(getpid(), SIGUSR1);
}

void
ControlThread::stop()
{
	unique_lock<mutex> lck(m_cmdMutex);
	m_newCommand = true;
	m_stop = true;
	m_cmdCondition.notify_all();
}

void
ControlThread::join()
{
	m_thread.join();
}
