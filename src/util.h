#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "rapidjson/document.h"

typedef struct {
  std::string postgres_user;
  std::string postgres_password;
  std::string postgres_host;
  std::string postgres_database;
  std::string postgres_port;
} config_t;

// "172.17.0.2"

inline std::string readFile(std::string filename) {
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (in) {
        std::ostringstream contents;
        contents << in.rdbuf();
        in.close();
        return(contents.str().c_str());
    }
    throw(errno);
}


inline config_t getConfig() {
  config_t c = {"invoke", "test", "172.17.0.2", "test", "5432"};

  rapidjson::Document d;
  d.Parse(readFile("config.json").c_str());
  if (d.HasParseError()) {
    std::cout << "frag.cc : failed to parse config.json using default values " << std::endl;
    return c;
  }

  for (rapidjson::Value::ConstMemberIterator it = d.MemberBegin(); it != d.MemberEnd(); ++it) {
    if (strcmp(it->name.GetString(),"postgres_user")==0) {
      c.postgres_user = it->value.GetString();
    }    
    if (strcmp(it->name.GetString(),"postgres_password")==0) {
      c.postgres_password = it->value.GetString();
    }
    if (strcmp(it->name.GetString(),"postgres_host")==0) {
      c.postgres_host = it->value.GetString();
    }
    if (strcmp(it->name.GetString(),"postgres_database")==0) {
      c.postgres_database = it->value.GetString();
    }
    if (strcmp(it->name.GetString(),"postgres_port")==0) {
      c.postgres_database = it->value.GetString();
    }
  }
  return c;
}


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

inline bool hasDigit(const std::string& s)
{
    return std::any_of(s.begin(), s.end(), ::isdigit);
}

/*
 * Function to try to get a lock on a given mutex for a given period of time.
 * To be used where failure in locking is an option (eg soft index updates)
 * options
 * - int time to wait in microseconds. (default 100)
 * - int number of times to try. (default 5) (0 for infinate)
 */
inline bool softMutexLock(std::mutex &m, int t=100, int n=5) {
  for (int i=0; i<=n*t; i+t) {
    if (m.try_lock()) {
      return true;
    }
  }
  return false;
}

