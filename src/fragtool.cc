#include <iomanip>
#include "frag.h"
#include "util.h"
#include "texttools.h"
#include "c_plus_plus_serializer.h"

int main(int argc, char* argv[]) {
  std::string filename(argv[1]);
  int fd;
  if (fileLock(filename,fd) == true) {
    std::cout << "fileLock(s) true " << fd << std::endl;
    usleep(10000000);
  } else {
    std::cout << "fileLock(s) false " << fd << std::endl;
  }

  std::map<std::string, std::map<int, Frag::Item>> frag_map;
  std::cout << "filename " << filename << std::endl;
	std::ifstream in (filename);
	in >> bits(frag_map);
	in.close();
  /*
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
		for (std::map<int, Frag::Item>::const_iterator vit = (it->second).begin() ; vit != (it->second).end(); ++vit) {
			std::cout << "term : " << it->first << " : url_id : " << vit->second.url_id << std::endl;
		}
	}
  */
}

