#include "control_thread.h"

#include <sstream>
#include <algorithm>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "../usb2lin06.hpp"

#define MIN_HEIGHT 0
#define MAX_HEIGHT 52
#define STOP_DELAY 2

using namespace std;
using namespace usb2lin06;

const map<string, ControlThread::Command>
ControlThread::s_commandStr = {
	{"status", Command::status},
	{"up", Command::up},
	{"down", Command::down},
	{"stop", Command::stop},
	{"exit", Command::exit},
	{"plus", Command::plus},
	{"+", Command::plus},
	{"minus", Command::minus},
	{"-", Command::minus},
};

const map<string, uint>
ControlThread::s_numbers = {
	{"zero", 0},
	{"one", 1},
	{"two", 2},
	{"three", 3},
	{"four", 4},
	{"five", 5},
	{"six", 6},
	{"seven", 7},
	{"eight", 8},
	{"nine", 9},
	{"ten", 10},
	{"eleven", 11},
	{"twelve", 12},
	{"thirteen", 13},
	{"fourteen", 14},
	{"fifteen", 15},
	{"sixteen", 16},
	{"seventeen", 17},
	{"eighteen", 18},
	{"nineteen", 19},
	{"twenty", 20},
	{"thirty", 30},
	{"fourty", 40},
	{"fifty", 50},
};


ControlThread::ControlThread(std::string keyword):
	m_thread(&ControlThread::run, this),
	m_keyword(keyword),
#ifndef EMULATE_DESK
	m_udev(nullptr),
#endif
	m_stop(false),
	m_oldCommand(Command::stop),
	m_logger(),
	m_smallStep(false)
{
	std::transform(m_keyword.begin(), m_keyword.end(), m_keyword.begin(), ::tolower);
	unique_lock<mutex> lck(m_cmdMutex);
#ifndef EMULATE_DESK
	if(libusb_init(0)!=0)
	{
        m_logger << "Exiting" << endl;
		fprintf(stderr, "Error failed to init libusb");
		throw ControlException("could not initialize libusb");
	}
	libusb_set_debug(0,LIBUSB_LOG_LEVEL_WARNING);//and let usblib be verbose

	m_udev = usb2lin06::openDevice();
	if(m_udev == NULL )
	{
 		fprintf(stderr, "Error NO device");
		throw ControlException("could not USB device");
	}
	initDevice(m_udev);
#endif
	m_targetHeight = getHeight();
	m_newCommand = true;
	lck.unlock();
	m_cmdCondition.notify_all();
}

ControlThread::~ControlThread()
{
#ifndef EMULATE_DESK
	libusb_close(m_udev);
	libusb_exit(0);
#endif
}

ControlThread::StatusRecord 
ControlThread::cmd(std::string& cmd_line)
{
	StatusRecord sr;

	sr.currentHeight = m_currentHeight;
	sr.operationState = m_operationState;
	
	std::transform(cmd_line.begin(), cmd_line.end(), cmd_line.begin(), ::tolower);

	m_logger << __func__ << ": " << cmd_line << endl;

	istringstream is(cmd_line);

	if(!m_keyword.empty()){
		string kw;
		getline(is, kw, ' ');

		if(kw != m_keyword){
			m_logger << __func__ << ": keyword \"" << kw << "\" did not match!" << endl;
			return sr;
		}
	}

	string cmd;
	getline(is, cmd, ' ');

	try{
		Command c = s_commandStr.at(cmd);

		m_logger << "command=" << cmd << " (" << (int)c << ")" << endl;

		uint delta = 0;

		unique_lock<mutex> lck(m_cmdMutex);
		m_targetHeight = getHeight();
		m_currentHeight = m_targetHeight;
		m_logger << "CurrentHeight= " << m_currentHeight << " TargetHeight= " << m_targetHeight << endl;
		switch(c){
			case Command::status:
				m_logger << "reading status..." << endl;
				try{
					sr.currentHeight = getHeight();
				}catch(ControlException &e){
					cout << "got Exception: " << e.what() << endl;
					break;
				}
				m_newCommand = true;
				break;
			case Command::up:
				if(m_oldCommand == Command::down || m_oldCommand == Command::minus){
					sleep(1);
				}
				m_logger << "moving up..." << endl;
				m_targetHeight = MAX_HEIGHT;
				m_newCommand = true;
				break;
			case Command::down:
				if(m_oldCommand == Command::up || m_oldCommand == Command::plus){
					sleep(1);
				}
				m_logger << "moving down..." << endl;
				m_targetHeight = MIN_HEIGHT;
				m_newCommand = true;
				break;
			case Command::stop:
				m_logger << "stop moving" << endl;
				try{
					m_targetHeight = getHeight();
				}catch(ControlException &e){
					cout << "got Exception: " << e.what() << endl;
					break;
				}
				m_newCommand = true;
				break;
			case Command::plus:
				if(m_oldCommand == Command::down || m_oldCommand == Command::minus){
					sleep(1);
				}
				delta = parseNumbers(is);
				m_smallStep = (delta <= STOP_DELAY? true: false);
				m_logger << "plus " << delta << " ..." << endl;
				m_currentHeight = getHeight();
				m_targetHeight = m_currentHeight + delta;
				if(m_targetHeight > MAX_HEIGHT){
					m_targetHeight = MAX_HEIGHT;
				}
				m_newCommand = true;
				break;
			case Command::minus:
				if(m_oldCommand == Command::up || m_oldCommand == Command::plus){
					sleep(1);
				}
				delta = parseNumbers(is);
				m_smallStep = (delta <= STOP_DELAY? true: false);
				m_logger << "minus " << delta << " ..." << endl;
				m_currentHeight = getHeight();
				m_targetHeight = m_currentHeight - delta;
				if(m_targetHeight < MIN_HEIGHT){
					m_targetHeight = MIN_HEIGHT;
				}
				m_newCommand = true;
				break;
			case Command::exit:
				m_logger << "exit..." << endl;
				m_newCommand = true;
				m_stop = true;
				break;
		}
		m_logger << "CurrentHeight= " << m_currentHeight << " TargetHeight= " << m_targetHeight << endl;

		m_oldCommand = c;

		//m_cmd = c;
		lck.unlock();
		m_cmdCondition.notify_all();

	} catch(out_of_range& e){
	}

	return sr;
}

uint 
ControlThread::parseNumbers(istream& is)
{
	uint rv(0);
	string term;

	do{
		getline(is, term, ' ');
		m_logger << __func__ << ": term=" << term << endl;

		try{
			uint term_num = s_numbers.at(term);
			m_logger << __func__ << "  -> " << term_num << endl;
			rv += term_num;
		} catch(out_of_range& e){
			try{
				uint term_num = std::stoul(term);
				rv += term_num;
			} catch(...){
				cout << "could not parse \"" << term << "\" as a number" << endl;
			}
		}
	} while(!term.empty() && is.good());

	return rv;
}

uint16_t
ControlThread::getHeight() throw(ControlException)
{
#ifndef EMULATE_DESK
	statusReport str;
	unique_lock<mutex> lck(m_busMutex);
	bool state = getStatusReport(m_udev, str);
	lck.unlock();
	if(!state){
		throw ControlException("status report problem");
	}

	//m_currentHeight = (
    float height = getHeightInCM(str);
	//cout << "height = " << height << endl;
	//return getHeightInCM(str);
	return height;
#else
	return 0;
#endif
}

uint16_t
ControlThread::move(Command cmd)
{
	uint16_t value = 0;
	unique_lock<mutex> lck(m_busMutex);

#ifndef EMULATE_DESK
	switch(cmd){
		case Command::up:
			//cout << __FUNCTION__ << ": moving up" << endl;
			value = moveUp(m_udev);
			break;
        case Command::down:
			//cout << __FUNCTION__ << ": moving down" << endl;
			value = moveDown(m_udev);
			break;
		default:
			break;
	}
#endif

	usleep(100);
	lck.unlock();
	return value;

}

void
ControlThread::run()
{
	unique_lock<mutex> lck(m_cmdMutex);
	while(!m_newCommand) m_cmdCondition.wait(lck);
	lck.unlock();
	//while(!m_stop){
	do{

		/*
		*/


		unique_lock<mutex> lck(m_cmdMutex);
		m_currentHeight = getHeight();
		lck.unlock();
		m_logger << "currentHeight=" << m_currentHeight << endl;


		//cout << "target height = " << m_targetHeight << " currentHeight = " << m_currentHeight << endl;
		if(m_targetHeight >= (m_currentHeight-STOP_DELAY) && m_targetHeight <=(m_currentHeight+STOP_DELAY) 
			&& !m_smallStep){
			// stop
			// TODO: how to stop?

			// wait for new command
			unique_lock<mutex> lck(m_cmdMutex);
			while(!m_newCommand) m_cmdCondition.wait(lck);
			lck.unlock();
		} else if(m_targetHeight <= (m_currentHeight + STOP_DELAY) && m_targetHeight > m_currentHeight && m_smallStep){
			m_logger << "small step up... " << endl;
			int step = m_targetHeight - m_currentHeight;
			for(int i = 0; i < (step*250); i++) move(Command::up);
			unique_lock<mutex> lck(m_cmdMutex);
			m_currentHeight = getHeight();
			m_targetHeight = m_currentHeight;
			m_smallStep = false;
			lck.unlock();
		} else if(m_targetHeight >= (m_currentHeight - STOP_DELAY) && m_targetHeight < m_currentHeight && m_smallStep){
			m_logger << "small step down... " << endl;
			int step =  m_currentHeight - m_targetHeight ;
			for(int i = 0; i < (step*250); i++) move(Command::down);
			unique_lock<mutex> lck(m_cmdMutex);
			m_currentHeight = getHeight();
			m_targetHeight = m_currentHeight;
			m_smallStep = false;
			lck.unlock();
		} else if(m_currentHeight < m_targetHeight){
			bool state = move(Command::up);
			usleep(10000);
		} else{
			bool state = move(Command::down);
			usleep(10000);
		}

		if(m_stop){
			// stop now...
			//break;
		}

		//cout << __func__ << "..." << endl;

		m_newCommand = false;
	} while(!m_stop);
	m_logger << __func__ << " done." << endl;

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
