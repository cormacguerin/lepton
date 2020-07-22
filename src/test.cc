#include <iostream>

std::string lcs( std::string a, std::string b )
{
    if( a.empty() || b.empty() ) return {} ;

	std::string current_lcs = "";

    for(int i=0; i< a.length(); i++) {
        size_t fpos = b.find(a[i], 0);
        while(fpos != std::string::npos) {
			std::string tmp_lcs = "";
            tmp_lcs += a[i];
            for (int x = fpos+1; x < b.length(); x++) {
                tmp_lcs+=b[x];
                size_t spos = a.find(tmp_lcs, 0);
                if (spos == std::string::npos) {
                    break;
                } else {
					i+=fpos;
                    if (tmp_lcs.length() > current_lcs.length()) {
                        current_lcs = tmp_lcs;
                    }
                }   
            }
            fpos = b.find(a[i], fpos+1);
        }
    }
    return current_lcs;
}

int main(int argc, char** argv)
{
	std::cout << lcs(std::string(argv[1]), std::string(argv[2])) << std::endl;
}

