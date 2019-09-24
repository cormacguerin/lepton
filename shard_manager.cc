#include "shard_manager.h"
#include <string>
#include <iostream>
//#include <filesystem>
#include <algorithm>
#include <ctime>
#include <unordered_set>
#include "dirent.h"

ShardManager::ShardManager()
{
}

ShardManager::~ShardManager()
{
}

void ShardManager::addTerms(std::map<std::string, Shard::Term> doc_unigrams,
				std::map<std::string, Shard::Term> doc_bigrams,
				std::map<std::string, Shard::Term> doc_trigrams) {

	// add unigrams
	for (std::map<std::string, Shard::Term>::const_iterator it = doc_unigrams.begin(); it != doc_unigrams.end(); ++it) {
		std::map<std::string, std::map<int, Shard::Term>>::iterator tit = unigram_terms.find(it->first);
		if (tit != unigram_terms.end()) {
			std::map<int, Shard::Term>::iterator iit = (tit->second).find((it->second).url_id);
			if (iit != (tit->second).end()) {
				(tit->second).at((it->second).url_id) = it->second;
			} else {
				(tit->second).insert(std::pair<int,Shard::Term>((it->second).url_id,it->second));
			}
		} else {
			std::map<int,Shard::Term> termap;
			termap.insert(std::pair<int,Shard::Term>((it->second).url_id,it->second));
			unigram_terms.insert(std::pair<std::string,std::map<int,Shard::Term>>(it->first,termap));
		}
		//std::cout << "unigram_terms.size(): " << unigram_terms.size() << std::endl;
		if ((unigram_terms.size()+1)%BATCH_SIZE==0) {
			std::cout << "shard_manager.cc : batch size reached unigram_terms.size() " << std::endl;
			std::cout << "shard_manager.cc : unigram_terms.size() " << unigram_terms.size()+1 << std::endl;
			syncShards();
		}
	}
}

// function to sync all index loaded terms to shards.
void ShardManager::syncShards() {
	int syncsize = unigram_terms.size();
	time_t beforetime = time(0);
	std::cout << "shard_manager.cc : begin syncShards " << beforetime << std::endl;
	// load last shard (for new insertions)
	//loadLastShard();
	loadShards();
	while (unigram_terms.size()>0) {
		phmap::parallel_flat_hash_map<std::string, int>::iterator it = unigram_shard_term_index.find(unigram_terms.begin()->first);
		if (it != unigram_shard_term_index.end()) {
			shards[it->second].get()->insert(unigram_terms.begin()->first, unigram_terms.begin()->second);
		} else {
			unigram_shard_term_index.insert(std::pair<std::string,int>(unigram_terms.begin()->first, last_shard_id));
			if (shards[last_shard_id].get()->size() == SHARD_SIZE) {
				// write this shard fragment and create a new one for new entries.
				shards[last_shard_id].get()->write();
				shards[last_shard_id] = std::make_unique<Shard>(Shard::Type::UNIGRAM, last_shard_id);
				// increment creating a new shard and first fragment.
				last_shard_id = last_shard_id+1;
				shards[last_shard_id] = std::make_unique<Shard>(Shard::Type::UNIGRAM, last_shard_id, 1);
			}
			shards[last_shard_id].get()->insert(unigram_terms.begin()->first, unigram_terms.begin()->second);
			unigram_terms.erase(unigram_terms.begin());
		}
	}
	// finially write our last (probably not full) shard.
	// last_shard.get()->write();
	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "shard_manager.cc : syncShards of " << syncsize << " terms completed in " << seconds << " seconds. " << aftertime << std::endl;
	std::cout << "shard_manager.cc : Index Term Size : "  << unigram_shard_term_index.size() << std::endl;
}

/*
 * Merging while crawling is just too slow.. this might mean json is too slow, or it might just be that rapid json is too slow.
 * In fact the problem is loading / parsing the json that is slow.. writing is fast (order of magnitude faster). 
 * This function is some of the logic I had for merging shards. maybe we can run it as a separate process, to clean up the shards
 */
void ShardManager::mergeShards() {
	int syncsize = unigram_terms.size();
	time_t beforetime = time(0);
	std::cout << "shard_manager.cc : begin syncShards " << beforetime << std::endl;
	// load last shard (for new insertions)
	loadLastShard();
	std::unordered_set<std::string> unigram_term_index;
	for (std::map<std::string, std::map<int, Shard::Term>>::iterator it=unigram_terms.begin(); it!=unigram_terms.end(); it++) {

		if (unigram_term_index.find(it->first) != unigram_term_index.end()) {
//			std::cout << "shard_manager.cc " << it->first  << " indexed" << std::endl;
			continue;
		} else {
//			std::cout << "shard_manager.cc " << it->first  << " not indexed" << std::endl;
		}
		phmap::parallel_flat_hash_map<std::string, int>::iterator iit = unigram_shard_term_index.find(it->first);

		if (iit != unigram_shard_term_index.end()) {
			
	//		std::cout << "Existing term " << it->first << " found in shard " << it->second << std::endl;
			// load shard
			Shard shard(Shard::Type::UNIGRAM, iit->second);

			// find and move / merge all terms into the shard.
			std::vector<std::string> shard_keys = shard.getTermKeys();
			int counter=0;
			for (std::vector<std::string>::iterator kit=shard_keys.begin(); kit!=shard_keys.end(); kit++) {
				std::map<std::string, std::map<int, Shard::Term>>::iterator tit = unigram_terms.find(*kit);
				if (tit != unigram_terms.end()) {
					// insert the term and data into the last shard.
					shard.update(tit->first, tit->second);
					// insert the shard number for the term to the index.
					unigram_term_index.insert(tit->first);
					// erase the completed terms
					// unigram_terms.erase(tit++);
					counter++;
				}
			}
			std::cout << "shard_manager.cc : " << counter << " url terms synced into shard " << shard.shard_id << std::endl;
			// were should be finished with this shard, so write it.
			shard.write();
			std::cout << "shard_manager.cc : " << unigram_terms.size() << " terms left to sync." << std::endl;
		} else {
			// this is a new term. find the next available shard.
			if (last_shard.get()->size() == SHARD_SIZE) {
			//	std::cout << "shard_manager.cc : max shard size reached, write this shard and create new." << std::endl;
				last_shard.get()->write();
				last_shard = std::make_unique<Shard>(Shard::Type::UNIGRAM, last_shard_id+1);
			}
			// insert the term and data into the last shard.
			// std::cout << "shard_manager.cc : insert to last shard (" << last_shard.get()->id <<  ")" << it->first << " " << std::endl;

			last_shard.get()->insert(it->first, it->second);
			// insert the shard number for the term to the index.
			unigram_shard_term_index.insert(std::pair<std::string,int>(it->first, last_shard.get()->shard_id));
			// remove the term from the current map
			// unigram_terms.erase(unigram_terms.begin());
		}
	}
	unigram_terms.clear();
	// finially write our last (probably not full) shard.
	last_shard.get()->write();
	time_t aftertime = time(0);
	double seconds = difftime(aftertime, beforetime);
	std::cout << "shard_manager.cc : syncShards of " << syncsize << " terms completed in " << seconds << " seconds. " << aftertime << std::endl;
	std::cout << "shard_manager.cc : Index Term Size : "  << unigram_shard_term_index.size() << std::endl;
}

void ShardManager::loadLastShard() {
	std::vector<std::string> index_files;
	std::string path = "index/";
	/*
	for (const auto & entry : std::filesystem::directory_iterator(path)) {
		index_files.push_back(entry.path());
	}
	*/
	struct dirent *entry;
	DIR *dp;

	dp = opendir(path.c_str());
	if (dp == NULL)
	{
	perror("opendir");
		std::cout << "shard_manager.cc : Error , unable to load last shard" << std::endl;;
		exit;
	}
	std::string ext = ".shard";
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
		if ((e_.find(ext) != std::string::npos)) {
			index_files.push_back(entry->d_name);
		}
	}
	closedir(dp);

	std::sort(index_files.begin(),index_files.end());
	for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
		std::cout << *it << std::endl;
	}
	if (index_files.empty()) {
		std::cout << "no index files create new shard" << std::endl;
		last_shard = std::make_unique<Shard>(Shard::Type::UNIGRAM,1,1);
	} else {
		int shard_id = stoi(index_files.back().substr(index_files.back().find('_')+1,(index_files.back()).find('.')));
		std::cout << "loading last shard with id " << shard_id << std::endl;
		last_shard = std::make_unique<Shard>(Shard::Type::UNIGRAM,shard_id);
	}
}


void ShardManager::loadShards() {
	std::vector<std::string> index_files;
	std::string path = "index/";
	/*
	for (const auto & entry : std::filesystem::directory_iterator(path)) {
		index_files.push_back(entry.path());
	}
	*/
	struct dirent *entry;
	DIR *dp;

	dp = opendir(path.c_str());
	if (dp == NULL)
	{
	perror("opendir");
		std::cout << "shard_manager.cc : Error , unable to load last shard" << std::endl;;
		exit;
	}
	std::string ext = ".shard";
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
		if ((e_.find(ext) != std::string::npos)) {
			index_files.push_back(entry->d_name);
		}
	}
	closedir(dp);

	std::sort(index_files.begin(),index_files.end());
	if (index_files.empty()) {
		std::cout << "no index files create new shard" << std::endl;
		shards[1] = std::make_unique<Shard>(Shard::Type::UNIGRAM,1,1);
		last_shard_id = 1;
	} else {
		int this_shard_id = 1;
		int this_frag_id = 1;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			std::cout << *it << std::endl;
			std::string shard_string = (*it).substr((*it).find('_')+1,(*it).length());

			std::cout << shard_string.substr(0, shard_string.find('.')) << std::endl;
			int shard_id = stoi(shard_string.substr(0, shard_string.find('.')));
			std::cout << shard_string.substr(shard_string.find('.')+7,shard_string.length()) << std::endl;
			int shard_frag_id = stoi(shard_string.substr(shard_string.find(".shard")+7,shard_string.length()));

			std::cout << "shard_id " << shard_id << std::endl;
			std::cout << "shard_frag_id " << shard_frag_id << std::endl;
			if (this_shard_id!=shard_id) {
				shards[this_shard_id] = std::make_unique<Shard>(Shard::Type::UNIGRAM,this_shard_id,this_frag_id);
			}
			this_shard_id=shard_id;
			this_frag_id=shard_frag_id;
		}
	}
}

