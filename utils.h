#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

namespace utils {

typedef unsigned int uint;

class io_exception : public std::exception {
public:
	io_exception(const char *ptr = "IO Exception") : msg(ptr) {}
	virtual const char *what() const throw() { return msg; }
private:
	const char *msg;
};

class file_not_found_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "File not found"; }
};

class File {
public:
	File();
	File(const std::string &name);
	void read(); // throw(file_not_found_exception, io_exception);
	void write(const uint8_t *data, int size); // throw(io_exception);
	void close();

	bool exists();
	uint8_t *getPtr();
	const std::string &getName() const { return fileName; }
	int getSize() const { return size; }
	std::vector<std::string> getLines();
private:
	std::string fileName;
	std::vector<uint8_t> data;
	uint size;
	bool loaded;
	FILE *writeFP;
	FILE *readFP;
};

class StringTokenizer {
public:
	StringTokenizer(const std::string &s, const std::string &delim);
	int noParts() { return args.size(); }
	const std::string &getString(int no) { return args[no]; }
	const char getDelim(int no) { return delims[no]; }
private:
	std::vector<std::string> args;
	std::vector<char> delims;
};

std::vector<std::string> split(const std::string &s, const std::string &delim = " ");

std::string urlencode(const std::string &s, const std::string &chars);
std::string urldecode(const std::string &s, const std::string &chars);

void sleepms(uint ms);
void makedir(const std::string &name);
void makedirs(const std::string &name);

bool endsWith(const std::string &name, const std::string &ext);
void makeLower(std::string &s);


// FORMAT

class Printable {
public:
	virtual std::string toText() const = 0;
};


bool parse_format(std::stringstream &ss, std::string &fmt);

void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<int8_t> &bytes);
void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<uint8_t> &bytes);
void format_stream(std::stringstream &ss, std::string &fmt, const Printable &printable);

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const T *arg) {
	if(parse_format(ss, fmt))
		ss << arg;
}

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const T& arg) {
	if(parse_format(ss, fmt))
		ss << arg;
}

template <class T> void format_stream(std::stringstream &ss, std::string &fmt, const std::vector<T>& arg) {
	if(parse_format(ss, fmt)) {
		bool first = true;
		int w = ss.width();
		for(auto b : arg) {
			if(!first) ss << " ";
			ss.width(w);
			ss << b;
			first = false;
		}
	}
}


template <class A, class... B>
void format_stream(std::stringstream &ss, std::string &fmt, const A &head, const B& ... tail)
{
	format_stream(ss, fmt, head);
	format_stream(ss, fmt, tail...);
}

template <class T>
std::string format(const std::string &fmt, const T& arg) {
	std::string fcopy = fmt;
	std::stringstream ss;
	format_stream(ss, fcopy, arg);
	ss << fcopy;
	return ss.str();  
}

std::string format(const std::string &fmt);

template <class A, class... B> std::string format(const std::string &fmt, const A &head, const B& ... tail)
{
	std::string fcopy = fmt;
	std::stringstream ss;
	format_stream(ss, fcopy, head);
	format_stream(ss, fcopy, tail...);
	ss << fcopy;
	return ss.str();
}


};

#endif // UTILS_H
