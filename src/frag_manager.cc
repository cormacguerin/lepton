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
#include "util.h"

FragManager::FragManager(Frag::Type type, std::string db, std::string tb, std::string l) : frag_type()
{
    frag_type = type;
    path = "index/" + db + "/" + tb + "/";
    std::replace(path.begin(),path.end(),' ','_');
    lang = l;
    loadFragIndex();
}

FragManager::~FragManager()
{
}

void FragManager::addTerms(std::map<std::string, Frag::Item> doc_grams) {

    for (std::map<std::string, Frag::Item>::const_iterator it = doc_grams.begin(); it != doc_grams.end(); ++it) {
        std::map<std::string, std::map<int, Frag::Item>>::iterator tit = grams_terms.find(it->first);
        if (tit != grams_terms.end()) {
            std::map<int, Frag::Item>::iterator iit = (tit->second).find((it->second).doc_id);
            if (iit != (tit->second).end()) {
                (tit->second).at((it->second).doc_id) = it->second;
            } else {
                (tit->second).insert(std::pair<int,Frag::Item>((it->second).doc_id,it->second));
            }
        } else {
            std::map<int,Frag::Item> termap;
            termap.insert(std::pair<int,Frag::Item>((it->second).doc_id,it->second));
            grams_terms.insert(std::pair<std::string,std::map<int,Frag::Item>>(it->first,termap));
        }
        //std::cout << "grams_terms.size(): " << grams_terms.size() << std::endl;
        if ((grams_terms.size()+1)%BATCH_SIZE==0) {
            // std::cout << "frag_manager.cc : batch size reached grams_terms.size() " << grams_terms.size()+1 << std::endl;
            syncFrags();
        }
    }
}

// function to sync all index loaded terms to frags.
void FragManager::syncFrags() {
    int syncsize = grams_terms.size();
    time_t beforetime = time(0);
    // std::cout << "frag_manager.cc : begin syncFrags " << beforetime << std::endl;
    // load / create frags for new insertions
    loadContinueFrags();

    while (grams_terms.size()>0) {
        phmap::parallel_flat_hash_map<std::string, int>::iterator it = gram_frag_term_index.find(grams_terms.begin()->first);
        if (it != gram_frag_term_index.end()) {
 /*           
           std::cout << "DEB last_frag_id " << last_frag_id << std::endl;
           std::cout << "DEB it->second " << it->second << std::endl;
           std::cout << "DEB filename " << frags[it->second].get()->filename << std::endl;
           std::cout << "DEB frag map size " << frags[it->second].get()->frag_map.size() << std::endl;
 */          
           // this is a known term so insert into the correct fragment (latest part, as loaded in loadContinueFrags())
           frags[it->second].get()->insert(grams_terms.begin()->first, grams_terms.begin()->second);
           grams_terms.erase(grams_terms.begin());
        } else {
           gram_frag_term_index.insert(std::pair<std::string,int>(grams_terms.begin()->first, last_frag_id));
           if (frags[last_frag_id].get()->size() == last_frag_id*FRAG_SIZE_MULTIPLIER) {
                // this frag is now full so write it's permanent index.
                frags[last_frag_id].get()->writeIndex();
                // increment the last/latest frag id and create a new frag for it.
                last_frag_id = last_frag_id+1;
                // insert into a new main fragment  (part 00001)
                frags[last_frag_id] = std::make_unique<Frag>(frag_type, last_frag_id, 1, path + lang);
           }
           frags[last_frag_id].get()->insert(grams_terms.begin()->first, grams_terms.begin()->second);
           grams_terms.erase(grams_terms.begin());
        }
    }
    frags[last_frag_id].get()->writeIndex();
    saveFrags();
    time_t aftertime = time(0);
    double seconds = difftime(aftertime, beforetime);
    // std::cout << "frag_manager.cc : syncFrags of " << syncsize << " terms completed in " << seconds << " seconds. " << aftertime << std::endl;
}

/*
 * In this function we.
 * Read all the frag parts 00001 to 00*x and merge them all into 00001
 * This means that 00001 part should always have the full index.
 * purge_batch is a list of doc_id's that have been recently updated
 */
void FragManager::mergeFrags(int num_docs, std::string database, std::map<int,std::string> purge_docs) {

    // std::cout << "frag_manager.cc : mergeFrags - " << database << std::endl;

    int syncsize = grams_terms.size();
    time_t beforetime = time(0);

    // std::cout << "frag_manager.cc : begin mergeFrags " << beforetime << std::endl;

    std::vector<std::string> index_files = getFiles(path,".frag");
    if (index_files.empty()) {
        std::cout << "frag_manager.cc : " << path << "  no index files to merge." << std::endl;
        return;
    } else {
        int this_frag_id = 0;
        std::unique_ptr<Frag> main_frag = std::make_unique<Frag>(frag_type, this_frag_id, 1, path + lang);
        main_frag.get()->purgeDocs(purge_docs);
        std::cout << "frag_manager.cc : " << path << " main frag fragmap size " << main_frag.get()->frag_map.size() << std::endl;

        // bool track if we are in a merge or not.
        bool merged_frag_part = false;
        for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
            
            std::string frag_string = (*it).substr((*it).find("gram_")+5,(*it).length());
            std::string frag_lit = frag_string.substr(0, frag_string.find("."));
            int frag_id = stoi(frag_lit);

            // skip if we only have one frag (we need more than one to merge)
            // std::cout << "frag_manager.cc : mergeFrags for loop " << path << " frag " << this_frag_id << "." << std::endl;
            if (std::count_if(index_files.begin(), index_files.end(), [frag_lit](const std::string& str) {
                    return str.find(frag_lit + ".frag") != std::string::npos; }) < 2) {
                main_frag = std::make_unique<Frag>(frag_type, frag_id, 1, path + lang);
                // std::cout << "frag_manager.cc : " << path << " purge A " << std::endl;
                main_frag.get()->purgeDocs(purge_docs);
                main_frag.get()->write();
                continue;
            }

            // test if the current frag iteration is complete or not
            if (this_frag_id!=frag_id) {
                if (merged_frag_part == true) {
                    main_frag.get()->addWeights(num_docs, database, lang);
                    main_frag.get()->write();
                    merged_frag_part = false;
                }
                main_frag = std::make_unique<Frag>(frag_type, frag_id, 1, path + lang);
                main_frag.get()->purgeDocs(purge_docs);
            }
            if (frag_string.find(".frag.")!=std::string::npos) {
                int frag_part_id = stoi(frag_string.substr(frag_string.find(".frag")+7,frag_string.length()));
                // std::cout << "frag_manager.cc : frag " << frag_id << " : " << frag_part_id << " : " << *it << std::endl;
                if (frag_part_id > 1) {
                    std::unique_ptr<Frag> frag_part = std::make_unique<Frag>(frag_type, frag_id, frag_part_id, path + lang);
                    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it=frag_part.get()->frag_map.begin(); it!=frag_part.get()->frag_map.end(); it++) {
                      /*
                        if ((it->first == "beirut") || (it->first == "cormac")) {
                            std::cout << "frag_manager.cc : DEB found " << it->first << " it->second.size() " << it->second.size() << std::endl;
                        }
                      */
                        main_frag.get()->update(it->first, it->second);
                    }
                    /*
                    if (frag_id == 24 || frag_id == 21) {
                        std::cout << "frag_manager.cc : DEB after main_frag.get()->frag_map[cormac].size()" <<  main_frag.get()->frag_map["cormac"].size() << std::endl;
                        std::cout << "frag_manager.cc : DEB after main_frag.get()->frag_map[beirut].size()" <<  main_frag.get()->frag_map["beirut"].size() << std::endl;
                    }
                    */
                    // delete the parts, remember that we want to keep the 00001 part as that is the main merge file.
                    if (frag_part.get()->fragment_id != 1) {
                        frag_part.get()->remove();
                    }
                    merged_frag_part = true;
                }
            }
            this_frag_id=frag_id;

            // we need to cater for the event that this is the last iteration (otherwise it won't get caught.
            if (std::next(it) == index_files.end()) {
                // std::cout << "frag_manger.cc - we are merging the last file in this group" << std::endl;
                main_frag.get()->addWeights(num_docs, database, lang);
                main_frag.get()->write();
            }
        }
    }
}


/*
 * load new fragments parts for each frag to start where we left off. 
 */
void FragManager::loadContinueFrags() {
    std::vector<std::string> index_files = getFiles(path,".frag.");
    // std::cout << "start loadContinueFrags" << std::endl;

    if (index_files.empty()) {
        // std::cout << "no index files create new frag" << std::endl;
        frags[1] = std::make_unique<Frag>(frag_type,1,1,path + lang);
        last_frag_id = 1;
    } else {
        // frag is .frag, frag_part is .0000* file 
        // both of these were set to 1
        int this_frag_id = 1;
        int this_frag_part_id = 1;
        for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
            std::string frag_string = (*it).substr((*it).find("gram_")+5,(*it).length());

            int frag_id = stoi(frag_string.substr(0, frag_string.find('.')));
            int frag_part_id = stoi(frag_string.substr(frag_string.find(".frag")+7,frag_string.length()));

            // here we find the last part file that was written for each frag and create
            // a new frag part (incremented by one), and assign it to the frag id
            if (this_frag_id!=frag_id) {
                frags[this_frag_id] = std::make_unique<Frag>(frag_type,this_frag_id,this_frag_part_id+1,path + lang);
                // std::cout << "DEB frags["<< this_frag_id <<"]" << std::endl;
            }
            this_frag_id=frag_id;
            this_frag_part_id = frag_part_id;
        }
        last_frag_id=this_frag_id;
        // std::cout << "DEB last frags["<< last_frag_id <<"]" << std::endl;
        // EDOT this might be wrong, testing new below.
        // frags[last_frag_id] = std::make_unique<Frag>(frag_type,this_frag_id+1,this_frag_part_id+1,path + lang);
        frags[last_frag_id] = std::make_unique<Frag>(frag_type,this_frag_id,this_frag_part_id+1,path + lang);
    }

    // std::cout << "end loadContinueFrags" << std::endl;

    // If we have a failed fragment, eg. index exists but frag does not.. then we have a problem,
    // This could happen say during a crash, so check for orphaned fragments and create one if it doesn't exist.
    // This could still cause missing items, but if we keep feeding it will fix itself.
    // If the index is missing, that's not a problem, a new index would be created automatically.
    for (std::vector<int>::iterator iit = indices.begin(); iit != indices.end(); iit++) {
        if (frags.find(*iit) == frags.end()) {
            std::cout << "frag_manager.cc : " << path << "  WARNING frag " << *iit << " not found.. creating." << std::endl;
            frags[*iit] = std::make_unique<Frag>(frag_type, *iit, 1, path + lang);
        }
    }
}


void FragManager::saveFrags() {
    // std::cout << "frag_manager.cc : save frags" << std::endl;
    for (std::map<int,std::unique_ptr<Frag>>::iterator it = frags.begin() ; it != frags.end(); ++it) {
        it->second.get()->write();
        it->second.reset();
    }
    frags.clear();
}


/*
 * Index files just list all the terms in a particular frag, for fast reading.
 */
void FragManager::loadFragIndex() {

    // std::cout << "load frag index" << std::endl;
    std::vector<std::string> index_files = getFiles(path,".idx");

    std::sort(index_files.begin(),index_files.end());
    if (index_files.empty()) {
        std::cout << "no index files, continue." << std::endl;
        return;
    } else {
        for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {

            std::string filename = path;
            filename.append(*it);
            std::cout << filename << std::endl;

            std::ifstream ifs(filename);

            if (ifs.good()) {
                rapidjson::Document d;
                d.Parse(readFile(filename).c_str());

                if (d.HasParseError()) {
                    std::cout << "shrad_manager.cc : unable to parse " << filename << std::endl;
                    if (remove(filename.c_str()) != 0 ) {
                        std::cout << "shrad_manager.cc : " << path << "   unable to remove bad index file " << filename << std::endl;
                    } else {
                        std::cout << "shrad_manager.cc : " << path << "   bad index file " << filename << " removed." << std::endl;
                    }
                }

                for (rapidjson::Value::ConstMemberIterator jit = d.MemberBegin(); jit != d.MemberEnd(); ++jit) {
                    for(const auto& field : d[jit->name.GetString()].GetArray()) {
                        gram_frag_term_index.insert(std::pair<std::string,int>(field.GetString(), atoi(jit->name.GetString())));
                    }
                    // store a list of indices for easy access.
                    indices.push_back(atoi(jit->name.GetString()));
                }

            }
            ifs.close();
        }
    }
}

std::vector<std::string> FragManager::getFiles(std::string path, std::string ext) {

    std::string gram_type;
    if (frag_type == Frag::Type::UNIGRAM) {
        gram_type = "unigram";
    } else if (frag_type == Frag::Type::BIGRAM) {
        gram_type = "bigram";
    } else if (frag_type == Frag::Type::TRIGRAM) {
        gram_type = "trigram";
    }

    std::vector<std::string> index_files;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(path.c_str());
    if (dp == NULL)
    {
        perror("opendir");
        std::cout << "frag_manager.cc : " << path << "   Error , unable to load last frag" << std::endl;;
        closedir(dp);
        return index_files;
    }
    try {
        while (entry = readdir(dp)) {
            std::string e_(entry->d_name);
            if ((e_.find(ext) != std::string::npos) && (e_.find(gram_type) != std::string::npos) && (e_.find(lang) != std::string::npos)) {
                index_files.push_back(entry->d_name);
            }
        }
    } catch (int e) {
        std::cout << "frag_manager.cc : " << path << "   failed to load index files " << e << std::endl;
    }

    closedir(dp);
    std::sort(index_files.begin(),index_files.end());
    return index_files;
}

