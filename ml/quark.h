#ifndef _QUARK_H_
#define _QUARK_H_

#include <string>
#include "word2vec.h"

class Quark {
	private:
                std::vector<std::string> Split(const std::string& subject);

	public:
	Quark();
	~Quark();
	void quit( int code );
	void init( std::string file );

};

#endif

