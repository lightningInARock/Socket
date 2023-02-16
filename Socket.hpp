#ifndef __UNI__SOCKET__
#define __UNI__SOCKET__

#include <exception>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

enum Domain {
	LOCAL = AF_LOCAL,
	IPV4 = AF_INET,
	IPV6 = AF_INET6
};

enum TransportProtocol {
	TCP = SOCK_STREAM,
	UDP = SOCK_DGRAM
};

class SocketException : public std::exception {
	const char* _msg;
public:
	SocketException() {}

	SocketException(const char* msg) : _msg(msg) {}

	const char* what() const noexcept {
		return _msg;
	}
};

class Socket {
	int _fd;
	int _port;
	int _dmn;
	int _tp;
	sockaddr_in _addr;
public:
	Socket() : _fd(-1), _port(-1), _dmn(-1), _tp(-1), _addr{0} {}

	Socket(int domain, int type, int protocol = 0)
		: _fd(-1), _port(-1), _addr{0} 
	{
		_dmn = domain;
		_tp = type;
		_fd = socket(_dmn, _tp, protocol);
		if(_fd < 0) {
			throw SocketException("Error opening socket");
		}
	}

	Socket(int file_desc) : _fd(file_desc), _port(-1), 
		_dmn(-1), _tp(-1), _addr{0} {}

	int get_fd() const {
		return _fd;
	}

	const sockaddr_in* get_addr() const {
		return &_addr;
	}
	
	void set_host(std::string host) {
		hostent* server = gethostbyname(host.c_str());
		if(server == nullptr) {
			throw SocketException("Error, no such host");
		}

		_addr.sin_family = (host.find(':') == std::string::npos)? IPV4: IPV6;
		for(int i = 0; i < server->h_length; ++i) {
			*((char*)&_addr.sin_addr.s_addr + i) =
				*((char*)server->h_addr + i);
		}
	}

	void set_port(int port) {
		_addr.sin_port = htons(port);
	}
	
	void Bind(int portno) {
		_port = portno;
		_addr.sin_family = _dmn;
		_addr.sin_addr.s_addr = INADDR_ANY;
		_addr.sin_port = htons(portno);
		if(bind(_fd, (sockaddr*)&_addr, sizeof(_addr)) < 0) {
			throw SocketException("Error binding socket");
		}
	}

	void Listen() {
		listen(_fd, 5);
	}

	Socket Accept() {
		sockaddr_in cli_addr;
		socklen_t clilen = sizeof(clilen);
		int n_sock_fd = accept(_fd, (sockaddr*)&cli_addr, &clilen);
		if(n_sock_fd < 0) {
			throw SocketException("Error accepting connection");
		}

		Socket sock(n_sock_fd);
		sock._tp = _tp;
		sock._dmn = _dmn;
		sock._addr = _addr;
		sock._port = _port;

		return sock;
	}

	std::string Read(const unsigned int size) {
		// std::string buff(size, 0);
		char buff[size];
		int n = read(_fd, &buff[0], size);
		if(n < 0) {
			throw SocketException("Error reading from socket");	
		}

		return std::string(buff, n);
	}

	int Write(const std::string msg) {
		int n = write(_fd, msg.c_str(), msg.size());
		if(n < 0) {
			throw SocketException("Error writing to socket");
		}
		return n;
	}

	void Connect(std::string host, int port) {
		set_host(host);
		_addr.sin_port = htons(port);

		int res = connect(_fd, (sockaddr*)&_addr, sizeof(_addr));
		if(res < 0) {
			throw SocketException("Error connecting");
		}
	}

	~Socket() {
		close(_fd);
	}
};

#endif
