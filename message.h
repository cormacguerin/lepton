//
// message.hpp
//

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

// class template message
template<bool isRequest, class Body>
struct message {
	Body b;
};

// class template request specialization:
template<class Body>
struct message<true, Body> {

	int version;
	enum {header_length};
	enum {max_body_length};

	char* header;
	Body body;
};

// class template response specialization:
template<class Body>
struct message<false, Body> {

	int version;
	enum {header_length};
	enum {max_body_length};

	char* header;
	Body body;
//	Response response;
};

template<class Body>
using request = message<true, Body>;

template<class Body>
using response = message<false, Body>;

//class Message {

//public:

//private:

/*
typedef char* Body;
template<bool isRequest, class Body>
void write(std::ostream&, message<isRequest, Body> const& msg);
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

		bool decode_header()
		{
			using namespace std; // For strncat and atoi.
			char header[header_length + 1] = "";
			strncat(header, data_, header_length);
			body_length_ = atoi(header);
			if (body_length_ > max_body_length)
			{
				body_length_ = 0;
				return false;
			}
			return true;
		}

		void encode_header()
		{
			using namespace std; // For sprintf and memcpy.
			char header[header_length + 1] = "";
			sprintf(header, "%4d", static_cast<int>(body_length_));
			memcpy(data_, header, header_length);
		}

		char data_[header_length + max_body_length];
		size_t body_length_;
		*/
//};

#endif
