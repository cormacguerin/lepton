
#include <sstream>
#include "query.h"
#include "texttools.h"

static const std::string OperatorList[4] = {"AND","OR","NOT","RESTRICT"};
static const std::string TypeList[4] = { "ORIGINAL", "SYNONYM", "CONCEPT" };
static const std::string modifierList[5] = { "LITERAL", "CONFIDENCE", "COLLECTION", "SYNCONF", "STOPWORD" };

rapidjson::Document serialized_query;

Query::Query(bool root, std::string raw_query)
{
}

Query::~Query()
{
}

rapidjson::Document Query::serializeTerm(Query::Term t) {
	rapidjson::Document serialized_;
	//std::map<Modifier, AttributeValue> mods;
	/*
	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.term.toUTF8String(converted);

	serialized_.Parse("{}");
	rapidjson::Document::AllocatorType& allocator_ = serialized_.GetAllocator();
	serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator_).Move(), allocator_);
	serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator_).Move(), allocator_);
	serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator_).Move(), allocator_);
	*/
	return serialized_;
}

std::string Query::Node::serialize() {
	std::cout << "raw_query " << raw_query << std::endl;
	std::cout << "raw_query length " << (raw_query).length() << std::endl;
	if ((this->raw_query).length()==0) {
		return "";
	}
	rapidjson::Document d;
	this->serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::cout << (std::string)buffer.GetString() << std::endl;
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
	if (OperatorList[this->op].c_str()) {
		serialized_query.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	}
	if ((this->raw_query).c_str()) {
		serialized_query.AddMember("raw_query", rapidjson::Value(const_cast<char*>(this->raw_query.c_str()), allocator).Move(), allocator);
	}

	Query::Term t = this->term;

	rapidjson::Value serialized_;
	serialized_.SetObject();

	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.term.toUTF8String(converted);
	std::cout << "converted " + converted<< std::endl;

	if (!TypeList[t.type].empty()) {
		serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator).Move(), allocator);
	}
	if (!converted.empty()) {
		serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
	}
	if (!strs.str().empty()) {
		serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator).Move(), allocator);
	}
	if (serialized_!=NULL) {
		serialized_query.AddMember("term", serialized_, allocator);
	}
	std::cout << "CANDIDATES SIZE " << this->candidates.size() << std::endl;
	if (this->candidates.size() > 0) {
		rapidjson::Value candidates(rapidjson::kArrayType);
		rapidjson::Value debug_urls(rapidjson::kArrayType);
		for (std::vector<Query::Term>::iterator it = this->candidates.begin() ; it != this->candidates.end(); ++it) {
			candidates.PushBack(rapidjson::Value().SetInt(it->debug_url_id), allocator);
			std::string u = it->debug_url;
			debug_urls.PushBack(rapidjson::Value(const_cast<char*>(u.c_str()), allocator).Move(), allocator);
		}
		serialized_query.AddMember("candidates", candidates, allocator);
		serialized_query.AddMember("debug_urls", debug_urls, allocator);
	}

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
	std::cout << "add query canditates" << std::endl;
	for (std::vector<Query::Node>::iterator it = this->leafNodes.begin() ; it != this->leafNodes.end(); ++it) {
		(*it).updateQuery();
	}
}

