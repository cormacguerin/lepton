
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

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

// class template Message
template<bool isRequest, class T>
struct Message;

// class template request specialization:
template<class T>
struct Message<true, T> : BaseMessage {
	const bool decode_header() {
	       	strncpy(len, header+strlen("length:"), 13);
		if (body_length > max_body_length) {
			body_length = 0;
			return false;
		} else {
			body_length = atoi(len);
			body = (char*)malloc(body_length);
			return true;
		}
	}
	T body;
};

// class template response specialization:
template<class T>
struct Message<false, T> : BaseMessage {
	T body;
};

template<class T>
using request = Message<true, T>;

template<class T>
using response = Message<false, T>;

#endif
