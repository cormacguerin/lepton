#include <sstream>
#include <fstream>
#include <iomanip>
//#include <filesystem>
#include "frag.h"
#include "texttools.h"
#include <pqxx/pqxx>
#include <pqxx/strconv.hxx>

rapidjson::Document serialized_frag;

Frag::Frag(Frag::Type type, int _frag_id, int _fragment_id) : prefix_type(), frag_id(), fragment_id()
{
	prefix_type=type;
	frag_id=_frag_id;
	fragment_id = _fragment_id;
	load();
}

Frag::~Frag()
{
}

void Frag::load() {
	std::string filename;
	if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/unigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/bigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/trigram_";
	} else {
		return;
	}

	std::stringstream frag_id_string;
	frag_id_string << std::setw(5) << std::setfill('0') << frag_id;
	
	if (fragment_id==0) {
		filename.append(frag_id_string.str());
		filename.append(".frag");
	} else {
		filename.append(frag_id_string.str());
		filename.append(".frag.");
		std::stringstream fragment_id_string;
		fragment_id_string << std::setw(5) << std::setfill('0') << fragment_id;
		filename.append(fragment_id_string.str());
	}

	time_t beforetime = time(0);
	std::ifstream ifs(filename);

	if (ifs.good()) {
		loadJsonFrag(filename);
		// loadRawFrag(filename);

		time_t aftertime = time(0);
		double seconds = difftime(aftertime, beforetime);
		if (fragment_id==0) {
			std::cout << "frag.cc : Frag " << frag_id << " loaded with size : " << frag_map.size() << " in " << seconds << " seconds." << std::endl;
		} else {
			std::cout << "frag.cc : Frag " << frag_id << "." << fragment_id << " loaded with size : " << frag_map.size() << " in " << seconds << " seconds." << std::endl;
		}
	} else {
		if (fragment_id==0) {
			std::cout << "frag.cc : Creating Frag << " << frag_id << " " << std::endl;
		} else {
			std::cout << "frag.cc : Creating Frag << " << frag_id << "." << fragment_id << " " << std::endl;
		}
		return;
	}
}

/*
 * TODO : Sean to implement.
 */
void Frag::loadRawFrag(std::string filename) {
}

void Frag::loadJsonFrag(std::string filename) {
	rapidjson::Document d;
//	d.ParseInsitu((char*)readFile(filename).c_str());
	d.Parse(readFile(filename).c_str());

	if (d.HasParseError()) {
		if (fragment_id==0) {
			std::cout << "frag.cc : failed to parse JSON in frag << " << frag_id << ", frag will be automatically discarded" << std::endl;
		} else {
			std::cout << "frag.cc : failed to parse JSON in frag << " << frag_id << "." << fragment_id << ", frag will be automatically discarded" << std::endl;
		}
		// wipe the frag and write it.
		frag_map.clear();
		write();
		return;
	}

	for (rapidjson::Value::ConstMemberIterator jit = d.MemberBegin(); jit != d.MemberEnd(); ++jit) {
//			std::cout << "frag.cc item : " << jit->name.GetString() << std::endl;
		std::map<int, Frag::Item> item_map;
		for (rapidjson::Value::ConstMemberIterator jtit = jit->value.MemberBegin(); jtit != jit->value.MemberEnd(); ++jtit) {
			Frag::Item item;
//				std::cout << "frag.cc  nc : " << jtit->name.GetString() << std::endl;
			for (rapidjson::Value::ConstMemberIterator jtit_ = jtit->value.MemberBegin(); jtit_ != jtit->value.MemberEnd(); ++jtit_) {
//					std::cout << "TEST " << jtit_->name.GetString() << std::endl;
				if (strcmp(jtit_->name.GetString(),"url_id")==0) {
//						std::cout << "frag.cc  url_id : " << jtit_->value.GetInt() << std::endl;
					item.url_id=jtit_->value.GetInt();
				}
				if (strcmp(jtit_->name.GetString(),"tf")==0) {
//						std::cout << "frag.cc  tf : " << jtit_->value.GetDouble() << std::endl;
					item.tf=jtit_->value.GetDouble();
				}
				if (strcmp(jtit_->name.GetString(),"weight")==0) {
//						std::cout << "frag.cc  weight : " << jtit_->value.GetDouble() << std::endl;
					item.weight=jtit_->value.GetDouble();
				}
			}
			item_map.insert(std::pair<int, Frag::Item>(atoi(jtit->name.GetString()), item));
		}
		frag_map.insert(std::pair<std::string,std::map<int,Frag::Item>>(jit->name.GetString(), item_map));
	}
}

void Frag::addToIndex(phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> &index) {
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
		for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end(); ++tit) {
			index[it->first].push_back(tit->second);
		}
		std::sort(index[it->first].begin(), index[it->first].end(),
			[](const Frag::Item& l, const Frag::Item& r) {
			return l.weight > r.weight;
		});
	}
}

void Frag::writeIndex() {
	time_t beforetime = time(0);
	std::string filename;
	if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/unigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/bigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/trigram_";
	} else {
		return;
	}
	
	std::stringstream frag_id_string;
	frag_id_string << std::setw(5) << std::setfill('0') << frag_id;
	filename.append(frag_id_string.str());
	filename.append(".idx");

	rapidjson::Document d_;
	rapidjson::Document::AllocatorType& allocator = serialized_frag.GetAllocator();

	rapidjson::Value index_;
	index_.SetObject();

	rapidjson::Value index_arr(rapidjson::kArrayType);
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
		index_arr.PushBack(rapidjson::Value(const_cast<char*>(it->first.c_str()), allocator).Move(), d_.GetAllocator());
	}
	std::string d_id = std::to_string(frag_id);
	d_.SetObject();
	d_.AddMember(rapidjson::Value(const_cast<char*>(d_id.c_str()), allocator).Move(), index_arr, allocator);

	// d_.AddMember(index_, allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d_.Accept(writer);
	std::ofstream f{filename};
	f << (std::string)buffer.GetString();
	f.close();
}

void Frag::write() {
	time_t beforetime = time(0);
	std::string filename;
	if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/unigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/bigram_";
	} else if (prefix_type==Frag::Type::UNIGRAM){
		filename = "index/trigram_";
	} else {
		return;
	}
	
	std::stringstream frag_id_string;
	frag_id_string << std::setw(5) << std::setfill('0') << frag_id;

	filename.append(frag_id_string.str());
	if (fragment_id==0) {
		filename.append(".frag");
	} else {
		filename.append(".frag.");
		std::stringstream fragment_id_string;
		fragment_id_string << std::setw(5) << std::setfill('0') << fragment_id;
		filename.append(fragment_id_string.str());
	}
	writeJsonFrag(filename);
	// writeRawFrag(filename);

	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "frag.cc : frag " << frag_id << " (" << frag_map.size() << " items) written in " << seconds << " seconds." << std::endl;

	frag_map.clear();
}

/*
 * TODO : Sean to implement.
 */
void Frag::writeRawFrag(std::string filename) {
}

void Frag::writeJsonFrag(std::string filename) {
	std::string tmp_filename = filename;
	tmp_filename.append("_");
	rapidjson::Document d;
	serialize_(d);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);
	std::ofstream f{tmp_filename};
	f << (std::string)buffer.GetString();
	f.close();
	buffer.Clear();

	// finally rename the frag to it's proper name.
	rename(tmp_filename.c_str(),filename.c_str());
}

void Frag::serialize_(rapidjson::Document &serialized_frag) {
	rapidjson::Document::AllocatorType& allocator = serialized_frag.GetAllocator();
	serialized_frag.Parse("{}");

	std::ostringstream strs;

	std::map<std::string, std::map<int, Frag::Item>>::iterator it;
	for (it = frag_map.begin(); it != frag_map.end(); it++) {
		rapidjson::Value fragItem_;
		fragItem_.SetObject();
		std::map<int, Frag::Item>::iterator tit;
		for (tit = it->second.begin(); tit != it->second.end(); tit++) {
			rapidjson::Value item_;
			item_.SetObject();
			item_.AddMember("url_id", rapidjson::Value().SetInt((tit->second).url_id), allocator);
			item_.AddMember("tf", rapidjson::Value().SetDouble((tit->second).tf), allocator);
			item_.AddMember("weight", rapidjson::Value().SetDouble((tit->second).weight), allocator);
			fragItem_.AddMember(rapidjson::Value(const_cast<char*>(std::to_string(tit->first).c_str()), allocator).Move(), item_, allocator);
		}
		serialized_frag.AddMember(rapidjson::Value(const_cast<char*>(it->first.c_str()), allocator).Move(), fragItem_, allocator);
	}
}

size_t Frag::size() {
	return frag_map.size();
}

std::vector<std::string> Frag::getItemKeys() {
	std::vector<std::string> v;
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it=frag_map.begin(); it!=frag_map.end(); it++) {
		v.push_back(it->first);
	}
	return v;
}

/*
 * function to assign a weight to each item for each url.
 * standard calculation for this is the item frequency times the idf(inverse document frequency).
 * idf is usually calculated as log(no. docs in corpus/no. of documents that contain the item)
 */
void Frag::addWeights(int num_docs) {
	pqxx::connection* C;

	try {
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
		if (C->is_open()) {
			std::cout << "Opened database successfully: " << C->dbname() << std::endl;
		} else {
			std::cout << "Can't open database" << std::endl;
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		exit;
	}

	pqxx::work txn(*C);
	C->prepare("update_unigram_idf", "INSERT INTO unigrams_en (idf,gram) VALUES ($1,$2) ON CONFLICT "
		       "ON CONSTRAINT unigrams_en_gram_key DO UPDATE SET idf = $1 WHERE unigrams_en.gram = $2");
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
		// TODO store the idf somewhere also.
		double idf = log((double)num_docs/(double)it->second.size());
		pqxx::result r = txn.prepared("update_unigram_idf")(std::to_string(idf))(it->first.c_str()).exec();
		// each word item in the index has an idf value.
		// std::cout << "frag.cc : idf value for item " << it->first << " : " << idf << std::endl;
		// each item for each url has a weight value.
		for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end(); ++tit) {
			tit->second.weight = idf*tit->second.tf;
			// std::cout << "frag.cc : weight value for item " << it->first << " in url id " << tit->first << " : " << tit->second.weight << std::endl;
		}
	}

	C->disconnect();
	delete C;

	txn.commit();
}

void Frag::insert(std::string s, std::map<int,Frag::Item> m) {
	frag_map.insert(std::pair<std::string,std::map<int,Frag::Item>>(s,m));
}

void Frag::update(std::string s, std::map<int,Frag::Item> m) {
	/*
	std::cout << "frag.cc  - - - -   - - - -   - - - -  UPDATE SHARD " << id << " : WITH ["<< s << "]  - - - -   - - - -  - - - -  " << std::endl;
	std::cout << "frag.cc before : " << std::endl;
	std::map<int, Frag::Item>::iterator it;
	for (it = frag_map.at(s).begin(); it != frag_map.at(s).end(); it++) {
		std::cout << "frag.cc b - " << it->first << std::endl;
	}
	*/
	// frag_map.at(s).insert(m.begin(), m.end());
	frag_map[s].insert(m.begin(), m.end());
	/*
	std::cout << "frag.cc after : " << std::endl;
	std::map<int, Frag::Item>::iterator it_;
	for (it_ = frag_map.at(s).begin(); it_ != frag_map.at(s).end(); it_++) {
		std::cout << "frag.cc a - " << it_->first << std::endl;
	}
	*/
}

std::string Frag::readFile(std::string filename) {
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

