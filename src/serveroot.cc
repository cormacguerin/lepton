#include "serveroot.h"
#include "session.h"
#include "query_server.h"

using namespace std;

ServeRoot::ServeRoot() {
}

ServeRoot::~ServeRoot()
{
}

/*
 * spin up a new connection to interface with and manage the service.
 */
void ServeRoot::run() {
  ManagementServer m(3333);
  m.run();
}

int main(int argc, char** argv) {
  ServeRoot sr;
  sr.run();
	return 0;
}

