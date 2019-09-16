#include <sstream>
#include <fstream>
#include <iomanip>
#include "shard.h"
#include "texttools.h"

rapidjson::Document serialized_shard;

Shard::Shard(Shard::Type type, int shard_id) : prefix_type(), id()
{
	prefix_type=type;
	id=shard_id;
}

Shard::~Shard()
{
}

rapidjson::Document Shard::serializeTerm(Shard::Term t) {
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

void Shard::write() {
	std::string filename;
	if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/unigram_shard.";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/bigram_shard.";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/trigram_shard.";
	} else {
		return;
	}
	std::stringstream postfix;
	postfix << std::setw(5) << std::setfill('0') << id;
	filename.append(postfix.str());

	rapidjson::Document d;
	serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::ofstream f{filename};
	std::cout << (std::string)buffer.GetString() << std::endl;
	f << (std::string)buffer.GetString();
}

void Shard::serialize_(rapidjson::Document &serialized_shard) {
	rapidjson::Document::AllocatorType& allocator = serialized_shard.GetAllocator();
	serialized_shard.Parse("{}");

	std::ostringstream strs;

	std::map<std::string, std::map<int, Shard::Term>>::iterator it;
	for (it = shard_map.begin(); it != shard_map.end(); it++) {
		rapidjson::Value shardTerm_;
		shardTerm_.SetObject();
		std::map<int, Shard::Term>::iterator tit;
		for (tit = it->second.begin(); tit != it->second.end(); tit++) {
			rapidjson::Value term_;
			term_.SetObject();
			term_.AddMember("url_id", rapidjson::Value().SetInt((tit->second).url_id), allocator);
			term_.AddMember("tf", rapidjson::Value().SetDouble((tit->second).tf), allocator);
			term_.AddMember("weight", rapidjson::Value().SetDouble((tit->second).weight), allocator);
			shardTerm_.AddMember(rapidjson::Value(const_cast<char*>(std::to_string(tit->first).c_str()), allocator).Move(), term_, allocator);
		}
		serialized_shard.AddMember(rapidjson::Value(const_cast<char*>(it->first.c_str()), allocator).Move(), shardTerm_, allocator);
	}

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
			// std::cout << *it << std::endl;
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

size_t Shard::size() {
	return shard_map.size();
}

void Shard::insert(std::string s, std::map<int,Shard::Term> m) {
	shard_map.insert(std::pair<std::string,std::map<int,Shard::Term>>(s,m));
}

