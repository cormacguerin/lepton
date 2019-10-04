
#include <sstream>
#include "query.h"
#include "texttools.h"

static const std::string OperatorList[5] = { "TERM", "AND", "OR", "NOT", "RESTRICT" };
static const std::string TypeList[4] = { "ORIGINAL", "SYNONYM", "CONCEPT" };
static const std::string modifierList[5] = { "LITERAL", "CONFIDENCE", "COLLECTION", "SYNCONF", "STOPWORD" };

Query::Query(bool root, std::string raw_query)
{
}

Query::~Query()
{
}

std::string Query::Node::serialize() {
	rapidjson::Document serialized_query;
	serialized_query.Parse("{}");
	std::cout << "query.cc : raw_query " << raw_query << std::endl;
	std::cout << "query.cc : raw_query length " << (raw_query).length() << std::endl;
	if ((this->raw_query).length()==0) {
		return "";
	}
	rapidjson::Document d;
	this->serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	// std::cout << "query.cc : " << (std::string)buffer.GetString() << std::endl;
	return (std::string)buffer.GetString();
}

void Query::Node::serialize_(rapidjson::Document &serialized_query) {
	rapidjson::Document::AllocatorType& allocator = serialized_query.GetAllocator();
	serialized_query.Parse("{}");
	if (this->root == false) {
		serialized_query.AddMember("root", "false", allocator);
	} else {
		serialized_query.AddMember("root", "true", allocator);
	}

	std::string converted;
	this->term.term.toUTF8String(converted);

	if (!TypeList[this->term.type].empty()) {
	       serialized_query.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[this->term.type]).c_str()), allocator).Move(), allocator);
	}
	if (!converted.empty()) {
	       serialized_query.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
	}
	if (OperatorList[this->op].c_str()) {
		serialized_query.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	}
	if ((this->raw_query).c_str()) {
		serialized_query.AddMember("raw_query", rapidjson::Value(const_cast<char*>(this->raw_query.c_str()), allocator).Move(), allocator);
	}

	rapidjson::Value serialized_;
	serialized_.SetObject();

	rapidjson::Document d_;
	rapidjson::Value nodes(rapidjson::kArrayType);
	for (std::vector<Query::Node>::iterator it = this->leafNodes.begin() ; it != this->leafNodes.end(); ++it) {
		(*it).serialize_(d_);
		nodes.PushBack(d_, d_.GetAllocator());
	}
	serialized_query.AddMember("nodes", rapidjson::Value(nodes, allocator).Move(), allocator);

}

/*
 * Function to populate the query with the best candidate urls.
 * TODO: this just returns urls in order of the incidence of the term.
 * We need a better way to return the 'best' doc matches, to do that we should..
 * - sort by idf rather than incidence
 * - post filter based on pagerank maybe..
 */
void Query::Node::updateQuery() {
	for (std::vector<Query::Node>::iterator it = this->leafNodes.begin() ; it != this->leafNodes.end(); ++it) {
		(*it).updateQuery();
	}
}

