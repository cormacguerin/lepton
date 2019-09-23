#include <sstream>
#include <fstream>
#include <iomanip>
//#include <filesystem>
#include "shard.h"
#include "texttools.h"

rapidjson::Document serialized_shard;

Shard::Shard(Shard::Type type, int shard_id) : prefix_type(), id()
{
	prefix_type=type;
	id=shard_id;
	load(shard_id);
}

Shard::~Shard()
{
}

void Shard::load(int shard_id) {
	std::string filename;
	if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/unigram_";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/bigram_";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/trigram_";
	} else {
		return;
	}
	
	std::stringstream postfix;
	postfix << std::setw(5) << std::setfill('0') << shard_id;
	filename.append(postfix.str());
	filename.append(".shard");

	//if (std::filesystem::exists(filename)) {
	time_t beforetime = time(0);
	std::ifstream ifs(filename);

	if (ifs.good()) {
		rapidjson::Document d;
	//	d.ParseInsitu((char*)readFile(filename).c_str());
		d.Parse(readFile(filename).c_str());

		if (d.HasParseError()) {
			std::cout << "shard.cc : failed to parse JSON in shard << " << shard_id << ", shard will be automatically discarded" << std::endl;
			// wipe the shard and write it.
			shard_map.clear();
			write();
			return;
		}

		for (rapidjson::Value::ConstMemberIterator jit = d.MemberBegin(); jit != d.MemberEnd(); ++jit) {
//			std::cout << "shard.cc term : " << jit->name.GetString() << std::endl;
			std::map<int, Shard::Term> term_map;
			for (rapidjson::Value::ConstMemberIterator jtit = jit->value.MemberBegin(); jtit != jit->value.MemberEnd(); ++jtit) {
				Term term;
//				std::cout << "shard.cc  n: " << jtit->name.GetString() << std::endl;
				for (rapidjson::Value::ConstMemberIterator jtit_ = jtit->value.MemberBegin(); jtit_ != jtit->value.MemberEnd(); ++jtit_) {
//					std::cout << "shard.cc  n_: " << jtit_->name.GetString() << std::endl;
//					std::cout << "shard.cc  v_: " << jtit_->value.GetDouble() << std::endl;
					if (jtit_->name.GetString()=="url_id") {
						 term.url_id=jtit_->value.GetInt();
					}
					if (jtit_->name.GetString()=="tf") {
						 term.tf=jtit_->value.GetDouble();
					}
					if (jtit_->name.GetString()=="weight") {
						 term.weight=jtit_->value.GetDouble();
					}
				}
				term_map.insert(std::pair<int, Shard::Term>(atoi(jtit->name.GetString()), term));
			}
			shard_map.insert(std::pair<std::string,std::map<int,Shard::Term>>(jit->name.GetString(), term_map));
		}

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		d.Accept(writer);
		time_t aftertime = time(0);
		double seconds = difftime(aftertime, beforetime);
		std::cout << "shard.cc : Shard "<< shard_id << " loaded with size : " << shard_map.size() << " in " << seconds << " seconds." << std::endl;
	} else {
		std::cout << "shard.cc : Creating new shard(" << shard_id << ")" << std::endl;
		return;
	}
}

void Shard::addToIndex(phmap::parallel_flat_hash_map<std::string, std::map<int, Shard::Term>> &index) {
	for (std::map<std::string, std::map<int, Shard::Term>>::iterator tit = shard_map.begin(); tit != shard_map.end(); ++tit) {
		// std::cout << tit->first << std::endl;
		index[tit->first].insert(tit->second.begin(), tit->second.end());
	}
}

void Shard::write() {
	time_t beforetime = time(0);
	std::string filename;
	if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/unigram_";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/bigram_";
	} else if (prefix_type==Shard::Type::UNIGRAM){
		filename = "index/trigram_";
	} else {
		return;
	}
	std::stringstream postfix;
	postfix << std::setw(5) << std::setfill('0') << id;
	filename.append(postfix.str());
	filename.append(".shard");

	rapidjson::Document d;
	serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::ofstream f{filename};
//	std::cout << (std::string)buffer.GetString() << std::endl;
	f << (std::string)buffer.GetString();
	f.close();

	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "shard.cc : shard " << id << " (" << shard_map.size() << " terms) written in " << seconds << " seconds." << std::endl;

	buffer.Clear();
	shard_map.clear();
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
}

size_t Shard::size() {
	return shard_map.size();
}

std::vector<std::string> Shard::getTermKeys() {
	std::vector<std::string> v;
	for (std::map<std::string, std::map<int, Shard::Term>>::iterator it=shard_map.begin(); it!=shard_map.end(); it++) {
		v.push_back(it->first);
	}
	return v;
}

void Shard::insert(std::string s, std::map<int,Shard::Term> m) {
	shard_map.insert(std::pair<std::string,std::map<int,Shard::Term>>(s,m));
}

void Shard::update(std::string s, std::map<int,Shard::Term> m) {
	/*
	std::cout << "shard.cc  - - - -   - - - -   - - - -  UPDATE SHARD " << id << " : WITH ["<< s << "]  - - - -   - - - -  - - - -  " << std::endl;
	std::cout << "shard.cc before : " << std::endl;
	std::map<int, Shard::Term>::iterator it;
	for (it = shard_map.at(s).begin(); it != shard_map.at(s).end(); it++) {
		std::cout << "shard.cc b - " << it->first << std::endl;
	}
	*/
	shard_map.at(s).insert(m.begin(), m.end());
	/*
	std::cout << "shard.cc after : " << std::endl;
	std::map<int, Shard::Term>::iterator it_;
	for (it_ = shard_map.at(s).begin(); it_ != shard_map.at(s).end(); it_++) {
		std::cout << "shard.cc a - " << it_->first << std::endl;
	}
	*/
}

std::string Shard::readFile(std::string filename) {
//	std::cout << filename << std::endl;
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
	if (in) {
		std::ostringstream contents;
		contents << in.rdbuf();
		in.close();
		return(contents.str().c_str());
	}
	throw(errno);
}

