
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

//class Message {

//public:

//private:

/*
typedef char* T;
template<bool isRequest, class T>
void write(std::ostream&, Message<isRequest, T> const& msg);
*/
	/*
		enum { header_length = 16 };
		enum { max_body_length = 512 };

		const char* data() const
		{
			return data_;
		}

		char* data()
		{
			return data_;
		}

		size_t length() const
		{
			return header_length + body_length_;
		}

		const char* body() const
		{
			return data_ + header_length;
		}

		char* body()
		{
			return data_ + header_length;
		}

		size_t body_length() const
		{
			return body_length_;
		}

		void body_length(size_t new_length)
		{
			body_length_ = new_length;
			if (body_length_ > max_body_length)
				body_length_ = max_body_length;
		}

		void encode_header()
		{
			using namespace std; // For sprintf and memcpy.
			char header[header_length + 1] = "";
			sprintf(header, "%4d", static_cast<int>(body_length_));
			memcpy(data_, header, header_length);
		}

		size_t body_length_;
		*/
//};

#endif
