#ifndef _CONTROL_THREAD_H
#define _CONTROL_THREAD_H

//#include <boost/thread.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <exception>

#include <libusb-1.0/libusb.h>

class ControlException: public std::exception{
public:
	ControlException(std::string msg) throw():
		m_msg(msg) {}

	virtual ~ControlException() throw() {}

	virtual const char* what() const throw(){
		return m_msg.c_str();
	}
 
protected:
	std::string m_msg;
};

class ControlThread{
public:
	enum class Command{
		status,
		up,
		down,
		stop,
		exit,
	};

	enum class Operation{
		none,
		moving_up,
		moving_down,
		failure,
	};

	struct StatusRecord{
		uint16_t currentHeight;
		Operation operationState;
	};

	ControlThread();
	~ControlThread();

	StatusRecord cmd(std::string& cmd_line);

	void join();
	void stop();

	bool stopped() { return m_stop; }

protected:
	uint16_t getHeight();
	void run();

	std::thread m_thread;
	std::mutex m_cmdMutex;
	std::condition_variable m_cmdCondition;
	libusb_device_handle* m_udev;
	bool m_newCommand;
	//Command m_cmd;
	bool m_stop;
	uint16_t m_targetHeight;
	uint16_t m_currentHeight;
	Operation m_operationState;

	const static std::map<std::string, Command> s_commandStr;
};

/*
std::ostream&
operator<< (std::ostream& os, const ControlThread::Command& cmd){
	switch(cmd){
		case ControlThread::Command::up:
			os << "UP";
			break;
		case ControlThread::Command::down:
			os << "DOWN";
			break;
		case ControlThread::Command::stop:
			os << "STOP";
			break;
		case ControlThread::Command::exit:
			os << "EXIT";
			break;
	}
}
*/

#endif // _CONTROL_THREAD_H
