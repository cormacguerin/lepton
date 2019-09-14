#include "shard_manager.h"
#include <string>
#include <iostream>
#include <filesystem>

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
		std::unordered_map<std::string, std::map<int, Shard::Term>>::iterator tit = unigram_term_index.find(it->first);
		if (tit != unigram_term_index.end()) {
			std::map<int, Shard::Term>::iterator iit = (tit->second).find((it->second).url_id);
			if (iit != (tit->second).end()) {
				(tit->second).at((it->second).url_id) = it->second;
			} else {
				(tit->second).insert(std::pair<int,Shard::Term>((it->second).url_id,it->second));
			}
		} else {
			std::map<int,Shard::Term> termap;
			termap.insert(std::pair<int,Shard::Term>((it->second).url_id,it->second));
			unigram_term_index.insert(std::pair<std::string,std::map<int,Shard::Term>>(it->first,termap));
		}
	}
	std::cout << "unigram_term_index.size(): " << unigram_term_index.size() << std::endl;
	if (unigram_term_index.size()+1%10000==0) {
		syncShards();
	}
}

// function to sync all index loaded terms to shards.
void ShardManager::syncShards() {
	std::cout << "sync shards " << std::endl;
	while (unigram_term_index.size()>0) {
		// serach each shard and find which contains the first term.
		std::unordered_map<std::string,int>::iterator it = unigram_shard_term_index.find(unigram_term_index.begin()->first);
		if (it != unigram_shard_term_index.end()) {
			// load shard and find
			// find and move / merge all terms into the shard.
			// save the shard
			// break (out of for loop and back into while loop)
		} else {
			// this is a new term. find the next available shard.
			if (unigram_shard_term_index.size() < SHARD_SIZE) {
				// load shard
				for (int j=SHARD_SIZE-unigram_shard_term_index.size(); j--;) {
					// push new terms into shard, while removing from unigram_term_index
					// and updating unigram_shard_term_index
					// save shard
				}
			} else {
				// if it doesnt exist spawn a new shard and update unigram_shard_term_index
				Shard shard;
				loadLatestShard();
			}
		}
	}
}

void ShardManager::loadLatestShard() {
	std::string path = "index/";
	    for (const auto & entry : std::filesystem::directory_iterator(path))
			    std::cout << entry.path() << std::endl;
}
