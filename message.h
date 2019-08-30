
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

struct BaseMessage {

	int version;
	char len[6];
	size_t body_length;
	enum {header_length = 14};
	enum {max_body_length = 1048576};
	char header[header_length];
	const bool deserialize() {
	}
};

// struct template Message
template<bool isRequest, class T>
struct Message;

// struct template request specialization:
template<class T>
struct Message<true, T> : BaseMessage {
	const bool decode_message() {
		std::cout << "header "<< header << std::endl;
		strncpy(len, header+strlen("length:"), 13);
		std::cout << "message len " << len << std::endl;
		if (body_length > max_body_length) {
			body_length = 0;
			return false;
		} else {
			body_length = atoi(len);
			std::cout << "request message body_length " << body_length << std::endl;
			body = (char*)malloc(body_length);
			std::cout << "sizeof body " << sizeof body << std::endl;
			// memset(body, 0, sizeof body);
			body[body_length] = '\0';
			return true;
		}
	}
	T body;
};

// struct template response specialization:
template<class T>
struct Message<false, T> : BaseMessage {
	const bool encode_message(char* msg) {
		if (body_length > max_body_length) {
			body_length = 0;
			return false;
		} else {
			body_length = strlen(msg);
			body = (char*)malloc(body_length);
			//memset(body, 0, sizeof body);
			memcpy(body, msg, body_length);
			//body[body_length] = '\0';
			return true;
		}
	}
	T body;
};

template<class T>
using request = Message<true, T>;

template<class T>
using response = Message<false, T>;

#endif
