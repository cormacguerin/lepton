#include "shard_manager.h"
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>

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
		std::unordered_map<std::string, std::map<int, Shard::Term>>::iterator tit = unigram_terms.find(it->first);
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
		std::cout << "unigram_terms.size(): " << unigram_terms.size() << std::endl;
		if ((unigram_terms.size()+1)%30==0) {
			syncShards();
		}
	}
}

// function to sync all index loaded terms to shards.
// function can definitely be more efficient.
// probably should do all existing shards first and
// then batch copy the rest into new shard(s)
void ShardManager::syncShards() {
	std::cout << "sync shards " << std::endl;
	// load last shard (for new insertions)
	loadLastShard();
	while (unigram_terms.size()>0) {
		// find shard by first term.
		std::unordered_map<std::string, int>::iterator it = unigram_shard_term_index.find(unigram_terms.begin()->first);
		if (it != unigram_shard_term_index.end()) {
			std::cout << "Existing term " << it->first << " found in shard " << it->second << std::endl;
			// load shard
			Shard shard(Shard::Type::UNIGRAM, it->second);
			// find and move / merge all terms into the shard.
			std::vector<std::string> shard_keys = shard.getTermKeys();
			for (std::vector<std::string>::iterator kit=shard_keys.begin(); kit!=shard_keys.end(); kit++) {
				std::unordered_map<std::string, std::map<int, Shard::Term>>::iterator tit = unigram_terms.find(*kit);
				std::cout << *kit << std::endl;
				if (tit != unigram_terms.end()) {
					// insert the term and data into the last shard.
					shard.insert(tit->first, tit->second);
					// insert the shard number for the term to the index.
					// unigram_shard_term_index.insert(std::pair<std::string,int>(tit->first, shard.id));
					// erase the completed terms
					unigram_terms.erase(tit++);
				}
			}
		} else {
			// this is a new term. find the next available shard.
			if (last_shard.get()->size() >= SHARD_SIZE) {
				last_shard.get()->write();
				int last_shard_id = last_shard.get()->id;
				std::cout << "CREATING SHARD " << last_shard_id+1 << std::endl;
				last_shard = std::make_shared<Shard>(Shard::Type::UNIGRAM, last_shard_id+1);
			}
			// insert the term and data into the last shard.
			last_shard.get()->insert(unigram_terms.begin()->first, unigram_terms.begin()->second);
			// insert the shard number for the term to the index.
			unigram_shard_term_index.insert(std::pair<std::string,int>(unigram_terms.begin()->first, last_shard.get()->id));
			// remove the term from the current map
			unigram_terms.erase(unigram_terms.begin());
		}
	}
}

void ShardManager::loadLastShard() {
	std::vector<std::string> index_files;
	std::string path = "index/";
	for (const auto & entry : std::filesystem::directory_iterator(path)) {
		index_files.push_back(entry.path());
	}
	std::sort(index_files.begin(),index_files.end());
	/*
	for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
		std::cout << *it << std::endl;
	}
	*/
	if (index_files.empty()) {
		std::cout << "no index files create new shard" << std::endl;
		last_shard = std::make_shared<Shard>(Shard::Type::UNIGRAM,0);
	} else {
		int shard_id = stoi(index_files.back().substr(index_files.back().find('_')+1,(index_files.back()).find('.')));
		last_shard = std::make_shared<Shard>(Shard::Type::UNIGRAM,shard_id);
	}
}

