
#include <sstream>
#include "query.h"
#include "texttools.h"

static const std::string OperatorList[4] = {"AND","OR","NOT","RESTRICT"};
static const std::string TypeList[4] = { "ORIGINAL", "SYNONYM", "CONCEPT" };
static const std::string modifierList[5] = { "LITERAL", "CONFIDENCE", "COLLECTION", "SYNCONF", "STOPWORD" };

Query::Query(bool root, std::string raw_query)
{
}

Query::~Query()
{
}

rapidjson::Document Query::serializeTerm(Query::Term t) {
	//std::map<Modifier, AttributeValue> mods;
	/*
	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.term.toUTF8String(converted);

	rapidjson::Document serialized_;
	serialized_.Parse("{}");
	rapidjson::Document::AllocatorType& allocator_ = serialized_.GetAllocator();
	serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator_).Move(), allocator_);
	serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator_).Move(), allocator_);
	serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator_).Move(), allocator_);
	return serialized_;
	*/
}

std::string Query::Node::serialize() {
	rapidjson::Document d;
	this->serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::cout << (std::string)buffer.GetString() << std::endl;
	return (std::string)buffer.GetString();
}

void Query::Node::serialize_(rapidjson::Document &serialized_query) {
	//rapidjson::Document serialized_query;
	rapidjson::Document::AllocatorType& allocator = serialized_query.GetAllocator();
	serialized_query.Parse("{}");
	if (this->root == false) {
		serialized_query.AddMember("root", "false", allocator);
	} else {
		serialized_query.AddMember("root", "true", allocator);
	}
	serialized_query.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	serialized_query.AddMember("raw_query", rapidjson::Value(const_cast<char*>(this->raw_query.c_str()), allocator).Move(), allocator);

	Query::Term t = this->term;

	rapidjson::Value serialized_;
	serialized_.SetObject();

	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.term.toUTF8String(converted);
	std::cout << "converted " + converted<< std::endl;

	serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator).Move(), allocator);
	serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
	serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator).Move(), allocator);

	serialized_query.AddMember("term", serialized_, allocator);

	rapidjson::Document d_;
	rapidjson::Value nodes(rapidjson::kArrayType);
	for (std::vector<Query::Node>::iterator it = this->leafNodes.begin() ; it != this->leafNodes.end(); ++it) {
		(*it).serialize_(d_);
		nodes.PushBack(d_, d_.GetAllocator());

//rapidjson::StringBuffer buffer;
//rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//d_.Accept(writer);
//std::cout << (std::string)buffer.GetString() << std::endl;
	}
	serialized_query.AddMember("nodes", rapidjson::Value(nodes, allocator).Move(), allocator);

}

