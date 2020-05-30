#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <algorithm>

inline int getLangInt(std::string l) {
  if (l == "en") {
    return 1; 
  } else if (l == "ja") {
    return 2;
  } else if (l == "zh") {
    return 3;
  } else if (l == "es") {
    return 4;
  } else if (l == "fr") {
    return 5;
  } else if (l == "de") {
    return 6;
  } else {
    return 0;
  }
}

inline std::string getLangCode(int i) {
  switch(i) {
    case 1:
      return "en"; 
      break;
    case 2:
      return "ja"; 
      break;
    case 3:
      return "zh";
      break;
    case 4:
      return "es"; 
      break;
    case 5:
      return "fr";
      break;
    case 6:
      return "de"; 
      break;
    default:
      return "nolang";
  }
}

inline int acquireLock (std::string filename) {
    int lockFd;

    if ((lockFd = open (filename.c_str(), O_CREAT | O_RDWR, 0666))  < 0)
        return -1;

    if (flock (lockFd, LOCK_EX | LOCK_NB) < 0) {
        close (lockFd);
        return -1;
    }

    return lockFd;
}

inline int releaseLock (int lockFd) {
    int fl = flock (lockFd, LOCK_UN);
    close (lockFd);
    std::cout << "fl " << fl << std::endl;
    return fl;
}

/*
 * Get a lock on a file.
 * Retry at 1, 3, 6 seconds.
 */
inline bool fileLock(std::string filename, int &n) {
  n = acquireLock(filename);
  if (n < 0) {
    for (int i=1; i <=3; i++) {
      usleep(1000000*i);
      n = acquireLock(filename);
      if (n > 0) {
        std::cout << "file unlocked" << std::endl;
        return true;
      }
    }
    return false;
  } else {
    return true;
  }
}

inline bool fileUnlock(int n) {
  if (releaseLock(n) < 0) {
    return false;
  } else {
    return true;
  }
}


inline std::string getDbPassword() {
    std::ifstream dbpassword("dbpassword");
	std::string password;

	if (dbpassword.good()) {
	  getline(dbpassword, password);
	} else {
	  std::cout << "no dbpassword file" << std::endl;
	  exit(1);
	}
	dbpassword.close();
    return password;
}

inline bool hasDigit(const std::string& s)
{
    return std::any_of(s.begin(), s.end(), ::isdigit);
}

