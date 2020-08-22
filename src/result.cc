
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
	rapidjson::Value results(rapidjson::kArrayType);
	for (std::vector<Result::Item>::const_iterator it = items.begin(); it != items.end(); ++it) {
		rapidjson::Document result;
		result.SetObject();
		result.AddMember("url", rapidjson::Value(const_cast<char*>(it->url.c_str()), allocator).Move(), allocator);
		result.AddMember("weight", rapidjson::Value(const_cast<char*>(std::to_string(it->weight).c_str()), allocator).Move(), allocator);
		result.AddMember("tf", rapidjson::Value(const_cast<char*>(std::to_string(it->tf).c_str()), allocator).Move(), allocator);
		result.AddMember("tdscore", rapidjson::Value(const_cast<char*>(std::to_string(it->tdscore).c_str()), allocator).Move(), allocator);
		result.AddMember("wscore", rapidjson::Value(const_cast<char*>(std::to_string(it->wscore).c_str()), allocator).Move(), allocator);
		result.AddMember("docscore", rapidjson::Value(const_cast<char*>(std::to_string(it->docscore).c_str()), allocator).Move(), allocator);
		result.AddMember("score", rapidjson::Value(const_cast<char*>(std::to_string(it->score).c_str()), allocator).Move(), allocator);
		result.AddMember("snippet", rapidjson::Value(const_cast<char*>(it->snippet.c_str()), allocator).Move(), allocator);

	    rapidjson::Value data(rapidjson::kObjectType);
	    for (std::map<std::string,std::string>::const_iterator dit = it->data.begin(); dit != it->data.end(); ++dit) {
            rapidjson::Value k(dit->first.c_str(), allocator);
            data.AddMember(k, rapidjson::Value(dit->second.c_str(), allocator).Move(), allocator);
        }
	    result.AddMember("data", data, allocator);
		results.PushBack(rapidjson::Value(result, allocator).Move(), allocator);
	}
	serialized_result.AddMember("items", results, allocator);
    serialized_result.AddMember("result_count", rapidjson::Value(const_cast<char*>(std::to_string(this->result_count).c_str()), allocator).Move(), allocator);
    serialized_result.AddMember("page_num", rapidjson::Value(const_cast<char*>(std::to_string(this->page_num).c_str()), allocator).Move(), allocator);
    serialized_result.AddMember("page_result_num", rapidjson::Value(const_cast<char*>(std::to_string(this->page_result_num).c_str()), allocator).Move(), allocator);
    serialized_result.AddMember("query_time", rapidjson::Value(const_cast<char*>(std::to_string(this->query_time).c_str()), allocator).Move(), allocator);
	// TODO: add if debugging enabled add query.
	rapidjson::Document serialized_query;
	serialized_query.Parse("{}");
	query.serialize_(serialized_query);
	// serialized_result.AddMember("query", rapidjson::Value(const_cast<char*>(query.serialize().c_str()), allocator).Move(), allocator);
	serialized_result.AddMember("query", serialized_query, allocator);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	serialized_result.Accept(writer);
	// std::cout << "result.cc : " << (std::string)buffer.GetString() << std::endl;
	return (std::string)buffer.GetString();
}

