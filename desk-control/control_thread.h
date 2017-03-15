#ifndef _CONTROL_THREAD_H
#define _CONTROL_THREAD_H

#define ENABLE_DEBUG 1
//#define EMULATE_DESK 1

//#include <boost/thread.hpp>
#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>
#include <exception>

#include <libusb-1.0/libusb.h>


class Logger {
public:
	template <typename T>
	Logger& operator <<(T const& value) {
#if ENABLE_DEBUG
        std::cout << value << std::endl;; // also print the level etc.
#endif
        return *this;
    }
};


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
		plus,
		minus,
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

	ControlThread(std::string keyword = "");
	~ControlThread();

	StatusRecord cmd(std::string& cmd_line);

	void join();
	void stop();

	bool stopped() { return m_stop; }

protected:
	uint16_t getHeight() throw(ControlException);
	uint16_t move(Command cmd);
	void run();
	uint parseNumbers(std::istream& is);

	std::string m_keyword;
	std::thread m_thread;
	std::mutex m_cmdMutex;
	std::mutex m_busMutex;
	std::condition_variable m_cmdCondition;
#ifndef EMULATE_DESK
	libusb_device_handle* m_udev;
#endif
	bool m_newCommand;
	Command m_oldCommand;
	//Command m_cmd;
	bool m_stop;
	int m_targetHeight;
	int m_currentHeight;
	Operation m_operationState;
	Logger m_logger;

	const static std::map<std::string, Command> s_commandStr;
	const static std::map<std::string, uint> s_numbers;
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
