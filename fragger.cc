#include <iomanip>
//#include <filesystem>
#include "frag.h"
#include "texttools.h"
#include "c_plus_plus_serializer.h"

rapidjson::Document serialized_frag;

Frager::Frager(Frag::Type type, int _frag_id, int _fragment_id) : prefix_type(), frag_id(), fragment_id()
{
}

Frager::~Frag()
{
}

void Frag::loadRawFrag(std::string filename) {
	std::ifstream in (filename);
	in >> bits(frag_map);
	in.close();
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
		for (std::map<int, Frag::Item>::const_iterator vit = (it->second).begin() ; vit != (it->second).end(); ++vit) {
			std::cout << "index_server.cc -  : " << it->first << " : " << (vit->second).url_id << std::endl;
		}
	}
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


