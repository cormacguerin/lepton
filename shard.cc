
#include <sstream>
#include "shard.h"
#include "texttools.h"

rapidjson::Document serialized_shard;

Shard::Shard()
{
}

Shard::~Shard()
{
}

rapidjson::Document Shard::serializeTerm(Shard::Term t) {
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

void Shard::serialize() {
	rapidjson::Document d;
	this->serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::cout << (std::string)buffer.GetString() << std::endl;
	//return (std::string)buffer.GetString();
}

void Shard::serialize_(rapidjson::Document &serialized_shard) {
	rapidjson::Document::AllocatorType& allocator = serialized_shard.GetAllocator();
	serialized_shard.Parse("{}");
	/*
	if (this->root == false) {
		serialized_shard.AddMember("root", "false", allocator);
	} else {
		serialized_shard.AddMember("root", "true", allocator);
	}
	if (OperatorList[this->op].c_str()) {
		serialized_shard.AddMember("operator", rapidjson::Value(const_cast<char*>((OperatorList[this->op]).c_str()), allocator).Move(), allocator);
	}
	if ((this->raw_shard).c_str()) {
		serialized_shard.AddMember("raw_shard", rapidjson::Value(const_cast<char*>(this->raw_shard.c_str()), allocator).Move(), allocator);
	}
	

	Shard::Term t = this->term;

	rapidjson::Value serialized_;
	serialized_.SetObject();

	std::ostringstream strs;
	strs << t.idf;

	std::string converted;
	t.term.toUTF8String(converted);
	std::cout << "converted " + converted<< std::endl;

	if ((TypeList[t.type])!=NULL) {
		serialized_.AddMember("type", rapidjson::Value(const_cast<char*>((TypeList[t.type]).c_str()), allocator).Move(), allocator);
	}
	if (converted!=NULL) {
		serialized_.AddMember("term", rapidjson::Value(const_cast<char*>(converted.c_str()), allocator).Move(), allocator);
	}
	if (strs.str()!=NULL) {
		serialized_.AddMember("idf", rapidjson::Value(const_cast<char*>(strs.str().c_str()), allocator).Move(), allocator);
	}
	if (serialized_!=NULL) {
		serialized_shard.AddMember("term", serialized_, allocator);
	}
	std::cout << "CANDIDATES SIZE " << this->candidates.size() << std::endl;
	if (this->candidates.size() > 0) {
		rapidjson::Value candidates(rapidjson::kArrayType);
		for (std::vector<int>::iterator it = this->candidates.begin() ; it != this->candidates.end(); ++it) {
			//	std::cout << *it << std::endl;
			candidates.PushBack(rapidjson::Value().SetInt(*it), allocator);
		}
		serialized_shard.AddMember("candidates", candidates, allocator);
	}

	rapidjson::Document d_;
	rapidjson::Value nodes(rapidjson::kArrayType);
	for (std::vector<Shard::Term>::iterator it = this->leafTerms.begin() ; it != this->leafTerms.end(); ++it) {
		(*it).serialize_(d_);
		nodes.PushBack(d_, d_.GetAllocator());
	}
	serialized_shard.AddMember("nodes", rapidjson::Value(nodes, allocator).Move(), allocator);
	*/

}

/*
 * Function to populate the shard with the best candidate urls.
 * TODO: this just returns urls in order of the incidence of the term.
 * We need a better way to return the 'best' doc matches, to do that we should..
 * - sort by idf rather than incidence
 * - post filter based on pagerank maybe..
 */
void Shard::updateShard() {
	std::cout << "add shard canditates" << std::endl;
	/*
	for (std::vector<Shard::Term>::iterator it = this->leafTerms.begin() ; it != this->leafTerms.end(); ++it) {
		(*it).updateShard();
	}
	*/
}

