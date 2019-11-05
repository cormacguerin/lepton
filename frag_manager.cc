#include "frag_manager.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
//#include <filesystem>
#include <algorithm>
#include <ctime>
#include <math.h>
#include <unordered_set>
#include "dirent.h"

FragManager::FragManager(Frag::Type type) : frag_type()
{
	frag_type = type;
	loadFragIndex();
}

FragManager::~FragManager()
{
}

void FragManager::addTerms(std::map<std::string, Frag::Item> doc_grams) {

	for (std::map<std::string, Frag::Item>::const_iterator it = doc_grams.begin(); it != doc_grams.end(); ++it) {
		std::map<std::string, std::map<int, Frag::Item>>::iterator tit = grams_terms.find(it->first);
		if (tit != grams_terms.end()) {
			std::map<int, Frag::Item>::iterator iit = (tit->second).find((it->second).url_id);
			if (iit != (tit->second).end()) {
				(tit->second).at((it->second).url_id) = it->second;
			} else {
				(tit->second).insert(std::pair<int,Frag::Item>((it->second).url_id,it->second));
			}
		} else {
			std::map<int,Frag::Item> termap;
			termap.insert(std::pair<int,Frag::Item>((it->second).url_id,it->second));
			grams_terms.insert(std::pair<std::string,std::map<int,Frag::Item>>(it->first,termap));
		}
		//std::cout << "grams_terms.size(): " << grams_terms.size() << std::endl;
		if ((grams_terms.size()+1)%BATCH_SIZE==0) {
			std::cout << "frag_manager.cc : batch size reached grams_terms.size() " << grams_terms.size()+1 << std::endl;
			syncFrags();
		}
	}
}

// function to sync all index loaded terms to frags.
void FragManager::syncFrags() {
	int syncsize = grams_terms.size();
	time_t beforetime = time(0);
	std::cout << "frag_manager.cc : begin syncFrags " << beforetime << std::endl;
	// load / create frags for new insertions
	loadFrags();
	while (grams_terms.size()>0) {
		phmap::parallel_flat_hash_map<std::string, int>::iterator it = gram_frag_term_index.find(grams_terms.begin()->first);
		if (it != gram_frag_term_index.end()) {
			frags[it->second].get()->insert(grams_terms.begin()->first, grams_terms.begin()->second);
			grams_terms.erase(grams_terms.begin());
		} else {
			gram_frag_term_index.insert(std::pair<std::string,int>(grams_terms.begin()->first, last_frag_id));
			if (frags[last_frag_id].get()->size() == last_frag_id*1000) {
				// this frag is now full so write it's permanent index.
				frags[last_frag_id].get()->writeIndex();
				// increment the last/latest frag id and create a new frag for it.
				last_frag_id = last_frag_id+1;
				frags[last_frag_id] = std::make_unique<Frag>(frag_type, last_frag_id, 1);
			}
			frags[last_frag_id].get()->insert(grams_terms.begin()->first, grams_terms.begin()->second);
			grams_terms.erase(grams_terms.begin());
		}
	}
	frags[last_frag_id].get()->writeIndex();
	saveFrags();
	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "frag_manager.cc : syncFrags of " << syncsize << " terms completed in " << seconds << " seconds. " << aftertime << std::endl;
	std::cout << "frag_manager.cc : Index Term Size : "  << gram_frag_term_index.size() << std::endl;
}

/*
 * Merging while crawling is just too slow.. this might mean json is too slow, or it might just be that rapid json is too slow.
 * In fact the problem is loading & parsing the json that is slow.. writing is faster (a lot faster). 
 * This function is some of the logic I had for merging frags. maybe we can run it as a separate process, to clean up the frags
 */
void FragManager::mergeFrags(int num_docs, std::string lang) {

	int syncsize = grams_terms.size();
	time_t beforetime = time(0);

	std::cout << "frag_manager.cc : begin mergeFrags " << beforetime << std::endl;

	std::vector<std::string> index_files = getFiles("index/",".frag");
	if (index_files.empty()) {
		std::cout << "no index files to merge." << std::endl;
		return;
	} else {
		int this_frag_id = 0;
		std::unique_ptr<Frag> main_frag;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			std::cout << "fram_manager.cc merge " << *it << std::endl;

			std::string frag_string = (*it).substr((*it).find('_')+1,(*it).length());
			int frag_id = stoi(frag_string.substr(0, frag_string.find(".")));

			if (this_frag_id!=frag_id) {
				if (this_frag_id != 0) {
					main_frag.get()->addWeights(num_docs);
					main_frag.get()->write();
					std::cout << " - - - FRAG " << this_frag_id << " DONE - - - " << std::endl;
				}
				main_frag = std::make_unique<Frag>(frag_type,frag_id);
			}
			if (frag_string.find(".frag.")!=std::string::npos) {
				int frag_part_id = stoi(frag_string.substr(frag_string.find(".frag")+7,frag_string.length()));
				std::cout << "frag_manager.cc : frag " << frag_id << " : " << frag_part_id << " : " << *it << std::endl;
				std::unique_ptr<Frag> frag_part = std::make_unique<Frag>(frag_type,frag_id,frag_part_id);
				for (std::map<std::string, std::map<int, Frag::Item>>::iterator it=frag_part.get()->frag_map.begin(); it!=frag_part.get()->frag_map.end(); it++) {
					main_frag.get()->update(it->first, it->second);
				}
			}
			this_frag_id=frag_id;
		}
	}

	/*
	std::unordered_set<std::string> unigram_term_index;
	for (std::map<std::string, std::map<int, Frag::Item>>::iterator it=grams_terms.begin(); it!=grams_terms.end(); it++) {

		if (unigram_term_index.find(it->first) != unigram_term_index.end()) {
//			std::cout << "frag_manager.cc " << it->first  << " indexed" << std::endl;
			continue;
		} else {
//			std::cout << "frag_manager.cc " << it->first  << " not indexed" << std::endl;
		}
		phmap::parallel_flat_hash_map<std::string, int>::iterator iit = gram_frag_term_index.find(it->first);

		if (iit != gram_frag_term_index.end()) {
			
			// std::cout << "Existing term " << it->first << " found in frag " << it->second << std::endl;
			// load frag
			Frag frag(frag_type, iit->second);

			// find and move / merge all terms into the frag.
			std::vector<std::string> frag_keys = frag.getTermKeys();
			int counter=0;
			for (std::vector<std::string>::iterator kit=frag_keys.begin(); kit!=frag_keys.end(); kit++) {
				std::map<std::string, std::map<int, Frag::Item>>::iterator tit = grams_terms.find(*kit);
				if (tit != grams_terms.end()) {
					// insert the term and data into the last frag.
					frag.update(tit->first, tit->second);
					// insert the frag number for the term to the index.
					unigram_term_index.insert(tit->first);
					// erase the completed terms
					// grams_terms.erase(tit++);
					counter++;
				}
			}
			std::cout << "frag_manager.cc : " << counter << " url terms synced into frag " << frag.frag_id << std::endl;
			// were should be finished with this frag, so write it.
			frag.write();
			std::cout << "frag_manager.cc : " << grams_terms.size() << " terms left to sync." << std::endl;
		} else {
			// this is a new term. find the next available frag.
			if (last_frag.get()->size() == FRAG_SIZE) {
			//	std::cout << "frag_manager.cc : max frag size reached, write this frag and create new." << std::endl;
				last_frag.get()->write();
				last_frag = std::make_unique<Frag>(frag_type, last_frag_id+1);
			}
			// insert the term and data into the last frag.
			// std::cout << "frag_manager.cc : insert to last frag (" << last_frag.get()->id <<  ")" << it->first << " " << std::endl;

			last_frag.get()->insert(it->first, it->second);
			// insert the frag number for the term to the index.
			gram_frag_term_index.insert(std::pair<std::string,int>(it->first, last_frag.get()->frag_id));
			// remove the term from the current map
			// grams_terms.erase(grams_terms.begin());
		}
	}
	grams_terms.clear();
	// finially write our last (probably not full) frag.
	last_frag.get()->write();
	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "frag_manager.cc : syncFrags of " << syncsize << " terms completed in " << seconds << " seconds. " << aftertime << std::endl;
	std::cout << "frag_manager.cc : Index Term Size : "  << gram_frag_term_index.size() << std::endl;
	*/
}


void FragManager::loadFrags() {
	std::vector<std::string> index_files = getFiles("index/",".frag.");

	if (index_files.empty()) {
		std::cout << "no index files create new frag" << std::endl;
		frags[1] = std::make_unique<Frag>(frag_type,1,1);
		last_frag_id = 1;
	} else {
		// frag is .frag, frag_part is .0000* file
		int this_frag_id = 1;
		int this_frag_part_id = 1;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			std::cout << *it << std::endl;
			std::string frag_string = (*it).substr((*it).find('_')+1,(*it).length());

			std::cout << frag_string.substr(0, frag_string.find('.')) << std::endl;
			int frag_id = stoi(frag_string.substr(0, frag_string.find('.')));
			std::cout << frag_string.substr(frag_string.find('.')+7,frag_string.length()) << std::endl;
			int frag_part_id = stoi(frag_string.substr(frag_string.find(".frag")+7,frag_string.length()));

			std::cout << "frag_id " << frag_id << std::endl;
			std::cout << "frag_part_id " << frag_part_id << std::endl;
			if (this_frag_id!=frag_id) {
				frags[this_frag_id] = std::make_unique<Frag>(frag_type,this_frag_id,this_frag_part_id+1);
			}
			this_frag_id=frag_id;
			this_frag_part_id=frag_part_id;
		}
		last_frag_id=this_frag_id;
		frags[last_frag_id] = std::make_unique<Frag>(frag_type,this_frag_id+1,this_frag_part_id+1);
	}
}


void FragManager::saveFrags() {
	std::cout << "frag_manager.cc : save frags" << std::endl;
	for (std::map<int,std::unique_ptr<Frag>>::iterator it = frags.begin() ; it != frags.end(); ++it) {
		it->second.get()->write();
		it->second.reset();
	}
	frags.clear();
}


void FragManager::loadFragIndex() {

	std::cout << "load frag index" << std::endl;
	std::vector<std::string> index_files = getFiles("index/",".idx");

	std::sort(index_files.begin(),index_files.end());
	if (index_files.empty()) {
		std::cout << "no index files, continue." << std::endl;
		return;
	} else {
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {

			std::string filename = "index/";
			filename.append(*it);
			std::cout << filename << std::endl;

			std::ifstream ifs(filename);

			if (ifs.good()) {
				rapidjson::Document d;
				d.Parse(readFile(filename).c_str());

				if (d.HasParseError()) {
					std::cout << "shrad_manager.cc : unable to parse " << filename << std::endl;
					if (remove(filename.c_str()) != 0 ) {
						std::cout << "shrad_manager.cc : unable to remove bad index file " << filename << std::endl;
					} else {
						std::cout << "shrad_manager.cc : bad index file " << filename << " removed." << std::endl;
					}
				}

				std::string gram_type;
				if (frag_type == Frag::Type::UNIGRAM) {
					gram_type = "unigram";
				} else if (frag_type == Frag::Type::BIGRAM) {
					gram_type = "bigram";
				} else if (frag_type == Frag::Type::TRIGRAM) {
					gram_type = "trigram";
				} else {return;}

				if (filename.find(gram_type) != std::string::npos) {
					for (rapidjson::Value::ConstMemberIterator jit = d.MemberBegin(); jit != d.MemberEnd(); ++jit) {
						std::cout << "frag.cc term : " << jit->name.GetString() << std::endl;
						for(const auto& field : d[jit->name.GetString()].GetArray()) {
							gram_frag_term_index.insert(std::pair<std::string,int>(field.GetString(), atoi(jit->name.GetString())));
						}
					}
				}

			}
		}
		std::cout << "frag_manager.cc : index frag mapping loaded." << std::endl;
	}
}


std::string FragManager::readFile(std::string filename) {
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

std::vector<std::string> FragManager::getFiles(std::string path, std::string ext) {

	struct dirent *entry;
	DIR *dp;

	dp = opendir(path.c_str());
	if (dp == NULL)
	{
	perror("opendir");
		std::cout << "frag_manager.cc : Error , unable to load last frag" << std::endl;;
		exit;
	}
	std::vector<std::string> index_files;
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
		if ((e_.find(ext) != std::string::npos)) {
			index_files.push_back(entry->d_name);
		}
	}
	closedir(dp);
	std::sort(index_files.begin(),index_files.end());
	return index_files;
}

