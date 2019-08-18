
#include "query.h"
#include "texttools.h"

Query::Query(bool root, std::string raw_query)
{
}

Query::~Query()
{
}

void Query::Node::serialize() {
	rapidjson::Document serialized_query;
	serialized_query.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = serialized_query.GetAllocator();
	if (this->root == false) {
		serialized_query.AddMember("root", "false", allocator);
	} else {
		serialized_query.AddMember("root", "true", allocator);
	}
	serialized_query.AddMember("root", this->root, allocator);
	//serialized_query.AddMember("raw_query", rapidjson::Value(trim(this->raw_query.c_str())), allocator);
	
	serialized_query.AddMember("raw_query", rapidjson::Value(const_cast<char*>(this->raw_query.c_str()), allocator).Move(), allocator);
}

