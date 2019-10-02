
#include <sstream>
#include "result.h"
#include "texttools.h"

static const std::string OperatorList[4] = {"AND","OR","NOT","RESTRICT"};
static const std::string TypeList[4] = { "ORIGINAL", "SYNONYM", "CONCEPT" };
static const std::string modifierList[5] = { "LITERAL", "CONFIDENCE", "COLLECTION", "SYNCONF", "STOPWORD" };

rapidjson::Document serialized_result;

Result::Result()
{
}

Result::~Result()
{
}

const std::string Result::serialize() {
	serialized_result.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = serialized_result.GetAllocator();
	rapidjson::Value urls(rapidjson::kArrayType);
	for (std::vector<Result::Item>::const_iterator it = items.begin(); it != items.end(); ++it) {
		rapidjson::Document result;
		result.SetObject();
		result.AddMember("url",rapidjson::Value(const_cast<char*>(it->url.c_str()), allocator).Move(), allocator);
		urls.PushBack(rapidjson::Value(result, allocator).Move(), allocator);
	}
	serialized_result.AddMember("urls", urls, allocator);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	serialized_result.Accept(writer);
	// std::cout << "result.cc : " << (std::string)buffer.GetString() << std::endl;
	return (std::string)buffer.GetString();
}

void Result::Item::serialize_(rapidjson::Document &serialized_result) {

	/*
	if (OperatorList[this->op].c_str()) {
		serialized_result.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	}
	*/

	/*
	rapidjson::Value serialized_;
	serialized_.SetObject();
	*/

	/*
	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.item.toUTF8String(converted);
	if (!strs.str().empty()) {
		serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator).Move(), allocator);
	}
	if (!TypeList[t.type].empty()) {
		serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator).Move(), allocator);
	}
	if (!converted.empty()) {
		serialized_.AddMember("item", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
	}
	if (serialized_!=NULL) {
		serialized_result.AddMember("item", serialized_, allocator);
	}

	rapidjson::Value candidates(rapidjson::kArrayType);
	rapidjson::Value urls(rapidjson::kArrayType);

	candidates.PushBack(rapidjson::Value().SetInt(it->url_id), allocator);
	std::string u = it->url;
	urls.PushBack(rapidjson::Value(const_cast<char*>(u.c_str()), allocator).Move(), allocator);

	serialized_result.AddMember("candidates", candidates, allocator);
	serialized_result.AddMember("urls", urls, allocator);

	rapidjson::Document d_;
	rapidjson::Value nodes(rapidjson::kArrayType);
	serialized_result.AddMember("result", rapidjson::Value(nodes, allocator).Move(), allocator);
	*/

}

