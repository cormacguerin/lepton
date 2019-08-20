
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

void Query::Node::serialize() {
	rapidjson::Document d;
	this->serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::cout << (std::string)buffer.GetString() << std::endl;
}

void Query::Node::serialize_(rapidjson::Document &serialized_query) {
	//rapidjson::Document serialized_query;
	serialized_query.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = serialized_query.GetAllocator();
	if (this->root == false) {
		serialized_query.AddMember("root", "false", allocator);
	} else {
		serialized_query.AddMember("root", "true", allocator);
	}
	serialized_query.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	serialized_query.AddMember("raw_query", rapidjson::Value(const_cast<char*>(this->raw_query.c_str()), allocator).Move(), allocator);
	rapidjson::Value terms(rapidjson::kArrayType);
	for (std::vector<Query::Term>::iterator it = this->terms.begin() ; it != this->terms.end(); ++it) {
		Query::Term t = *it;

		rapidjson::Value serialized_;
		serialized_.SetObject();

		std::ostringstream strs;
		strs << t.idf;

		std::string converted;
		t.term.toUTF8String(converted);
		std::cout << "converted" << std::endl;
		std::cout << converted << std::endl;

	//	serialized_.Parse("{}");
	//	rapidjson::Document::AllocatorType& allocator_ = serialized_.GetAllocator();
		serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator).Move(), allocator);
		serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
		serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator).Move(), allocator);
		terms.PushBack(serialized_, allocator);
	}
	serialized_query.AddMember("terms", terms, allocator);

	rapidjson::Value nodes(rapidjson::kObjectType);
	for (std::vector<Query::Node>::iterator it = this->childNodes.begin() ; it != this->childNodes.end(); ++it) {
		rapidjson::Document d_;
		d_.SetObject();
		(*it).serialize_(d_);
		serialized_query.AddMember("child", d_, allocator);
	}

}

