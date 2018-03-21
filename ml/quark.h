#ifndef _QUARK_H_
#define _QUARK_H_

#include <string>

class Quark {
	private:

	public:
	Quark();
	~Quark();
	void quit( int code );
	void init( std::string file );

};

#endif

