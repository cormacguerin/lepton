/*
 *  &copy; Cormac Guerin , Invoke Network
 *  Copyright 2021 All Rights Reserved.
 *
 *  All information contained herein is, and remains
 *  the property of Cormac Guerin & Invoke Network
 *
 *  The intellectual and technical concepts within remain
 *  the sole property of the aforementioned owners.
 *
 *  Reproduction and / or distriptions of this software is 
 *  strictly prohibited.
 */

#include <iomanip>
#include "frag.h"
#include "texttools.h"
#include "c_plus_plus_serializer.h"
#include "util.h"
#include <chrono>

rapidjson::Document serialized_frag;

Frag::Frag(Frag::Type type, int _frag_id, int _fragment_id, std::string p, bool _s) : prefix_type(type), frag_id(_frag_id), fragment_id(_fragment_id), path(p), is_serving(_s)
{
    prefix_type = type;
    frag_id = _frag_id;
    fragment_id = _fragment_id;
    path = p;
    is_serving = _s;

    if (prefix_type==Frag::Type::UNIGRAM){
        filename = path + "_unigram_";
    } else if (prefix_type==Frag::Type::BIGRAM){
        filename = path + "_bigram_";
    } else if (prefix_type==Frag::Type::TRIGRAM){
        filename = path + "_trigram_";
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
    
    load();
}

Frag::~Frag()
{
}

void Frag::load() {
    std::ifstream in (filename);
    if (in.good()) {
      in >> bits(frag_map);
      in.close();
    }

    if (is_serving == true) {
        writeRawMemMapFrag();
        loadMmap();
    }

    /*
       for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
         for (std::map<int, Frag::Item>::const_iterator vit = (it->second).begin() ; vit != (it->second).end(); ++vit) {
           std::cout << filename << " : " << it->first << " : " << vit->first << " : " << (vit->second).doc_id << std::endl;
         }
       }
    */

}

void Frag::loadMmap() {
    const char *frag_file;
    struct stat s;
    int fd;

    std::string serve_filename = filename;
    serve_filename.append(".serve");
    if ((fd = open (serve_filename.c_str(), O_RDONLY)) < 0) {
      std::cout << "can't open " << serve_filename << " for reading" << std::endl;
      return;
    }
    if (fstat (fd,&s) < 0) {
      std::cout << "can't stat file " << serve_filename << std::endl;
      return;
    }
    // frag_file = (char*)mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    frag_mem_map = (Frag::Item*)mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (frag_file == (caddr_t) -1) {
      std::cout << "frag.cc : read error mmapping " << serve_filename << std::endl;
      return;
    }

    serve_filename.append(".x");
    if ((fd = open (serve_filename.c_str(), O_RDONLY)) < 0) {
      std::cout << "can't open " << serve_filename << " for reading" << std::endl;
      return;
    }
    if (fstat (fd,&s) < 0) {
      std::cout << "can't stat file " << serve_filename << std::endl;
      return;
    }
    // frag_file = (char*)mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    frag_mem_map_pos = (int*)mmap(0, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (frag_file == (caddr_t) -1) {
      std::cout << "frag.cc : read error mmapping " << serve_filename << std::endl;
      return;
    }

    std::cout << "frag.cc : loadMmap sizeof(frag_mem_map_pos)" << std::endl;
    std::cout << sizeof(frag_mem_map_pos) << std::endl;
    for( unsigned int a = 0; a < sizeof(frag_mem_map_pos)/sizeof(frag_mem_map_pos[0]); a = a + 1 ) {
        std::cout << "frag_mem_map_pos[a] " << std::endl;
        std::cout << frag_mem_map_pos[a] << std::endl;
    }
}

void Frag::loadJsonFrag() {
    rapidjson::Document d;
    //	d.ParseInsitu((char*)readFile(filename).c_str());
    d.Parse(readFile(filename).c_str());

    if (d.HasParseError()) {
        if (fragment_id==0) {
            // std::cout << "frag.cc : failed to parse JSON in frag << " << frag_id << ", frag will be automatically discarded" << std::endl;
        } else {
            // std::cout << "frag.cc : failed to parse JSON in frag << " << frag_id << "." << fragment_id << ", frag will be automatically discarded" << std::endl;
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
                if (strcmp(jtit_->name.GetString(),"doc_id")==0) {
                    //						std::cout << "frag.cc  doc_id : " << jtit_->value.GetInt() << std::endl;
                    item.doc_id=jtit_->value.GetInt();
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

/*
 * Function to load fragments into the main index map
 * Options
 * index: reference to the main index map.
 * m : reference to the mutex used to allow access.
 * TODO, only update fragments that have been updated.
 */
void Frag::addToIndex(phmap::parallel_flat_hash_map<std::string, int[3]> &index, std::mutex &m) {
  int frag_mem_map_pos_len = sizeof(frag_mem_map_pos)/sizeof(frag_mem_map_pos[0]);
  std::cout << "frag_mem_map_pos_len " << frag_mem_map_pos_len << std::endl;
  if (frag_mem_map_pos_len > 0) {
      for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
          if (softMutexLock(m)==true) {
              // here we return a list of pairs for each document for the given term all sorted by their term weight
              // d[0] = frag id (indicating we should search in this frag)
              // d[1] = position in the .serve mmap file that the frags start at.
              // d[2] = position in the .serve mmap file that the frags ends at. 

              int pos_start = frag_mem_map_pos[std::distance(frag_map.begin(), it)];
              int pos_end;
              std::map<std::string, std::map<int, Frag::Item>>::iterator last_it = std::prev(frag_map.end(), 1);
              std::map<std::string, std::map<int, Frag::Item>>::iterator next_it = std::next(it, 1);
              if (it == last_it) {
                  pos_end = frag_mem_map_pos[frag_mem_map_pos_len];
              } else {
                  pos_end = frag_mem_map_pos[std::distance(frag_map.begin(), next_it)];
              }
              index[it->first][0] = frag_id;
              index[it->first][1] = pos_start;
              index[it->first][2] = pos_end;
              m.unlock();
          } else {
              continue;
          }
      }
  }
  frag_map.clear();
}

/*
 * formats the map as a sorted vector ready to be mmapped and served.
 * TODO, only update fragments that have been updated.
 */
void Frag::writeRawMemMapFrag() {
    usleep(20);
    int i = 0;
    int j = 0;
    int arr_size = 0;
    int frag_arr_pos[frag_map.size()] = {0};
    
    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it_ = frag_map.begin(); it_ != frag_map.end(); ++it_) {
        arr_size+=it_->second.size();
        frag_arr_pos[i]=arr_size;
        i++;
    }
    
    // Frag::Item frag_arr[arr_size*sizeof(Frag::Item)];
    Frag::Item* frag_arr = new Frag::Item[arr_size];
    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
        usleep(1);
        std::vector<Frag::Item> tmp;
        /*
        std::cout << "it->second.size()" << std::endl;
        std::cout << it->second.size() << std::endl;
        */
        for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end(); ++tit) {
            // std::cout << "frag.cc : addToIndex " << it->first << " : " << tit->first << " : " << tit->second.doc_id << std::endl;
            tmp.push_back(tit->second);
        }
        std::sort(tmp.begin(), tmp.end(),
            [](const Frag::Item& l, const Frag::Item& r) {
            return l.weight > r.weight;
            });
        //std::cout << j << " : " << " - array size " << arr_size << std::endl;
        
        /*
        Frag::Item* f = tmp.data();
        std::copy(f, f + tmp.size(), *frag_arr + j);
        j += tmp.size();
        */
        
        // TODO assign directly
        for (std::vector<Frag::Item>::iterator tmpit = tmp.begin(); tmpit != tmp.end(); ++tmpit) {
          /*
          std::cout <<j << " (*tmpit).doc_id " << tmpit->doc_id << std::endl;
          std::cout <<j << " (*tmpit).tf" << tmpit->tf << std::endl;
          std::cout <<j << " (*tmpit).weight " << tmpit->weight << std::endl;
          */
          frag_arr[j] = *tmpit;
          j++;
        }
    }

    std::string serve_filename = filename;
    serve_filename.append(".serve");
    std::string tmp_serve_filename = serve_filename;
    tmp_serve_filename.append("_");

    FILE * f;
    f = fopen(tmp_serve_filename.c_str(), "wb");
    if (f != NULL) {
        fwrite(frag_arr, sizeof(Frag::Item)*j, 1, f);
        fclose(f);
    } else {
        std::cout << "failed to save serving frag" << std::endl;
    }

    /*
    std::ofstream f(tmp_serve_filename,std::ios::out | std::ios::binary);
    F << frag_arr << "\n";
    f.close();
    */

    int lck;
    if (fileLock(serve_filename,lck) == true) {
        rename(tmp_serve_filename.c_str(),serve_filename.c_str());
        fileUnlock(lck);
    } else {
        std::cout << "frag.cc : WARNING : unable to get lock on writeRawFrag." << std::endl;
    }

    serve_filename.append(".x");
    tmp_serve_filename = serve_filename;
    tmp_serve_filename.append("_");
    
    FILE * f_;
    f_ = fopen(serve_filename.c_str(), "wb");
    if (f_ != NULL) {
        // fwrite(frag_arr_pos, (sizeof(frag_arr_pos)/sizeof(frag_arr_pos[0])), 1, f_);
        fwrite(frag_arr_pos, sizeof(int)*i, 1, f_);
        fclose(f_);
    } else {
        std::cout << "failed to save serving frag" << std::endl;
    }

    /*
    std::ofstream f_(tmp_serve_filename,std::ios::out | std::ios::binary);
    f_ << bits(frag_arr_pos) << "\n";
    f_.close();
    */

    int lck_;
    if (fileLock(serve_filename,lck_) == true) {
        rename(tmp_serve_filename.c_str(),serve_filename.c_str());
        fileUnlock(lck_);
    } else {
        std::cout << "frag.cc : WARNING : unable to get lock on writeRawFrag." << std::endl;
    }
    delete frag_arr;

}

void Frag::writeIndex() {
    time_t beforetime = time(0);
    std::string filename;
    if (prefix_type==Frag::Type::UNIGRAM){
        filename = path + "_unigram_";
    } else if (prefix_type==Frag::Type::BIGRAM){
        filename = path + "_bigram_";
    } else if (prefix_type==Frag::Type::TRIGRAM){
        filename = path + "_trigram_";
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
    if (softMutexLock(m_frag_map)==true) {
        for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
            index_arr.PushBack(rapidjson::Value(const_cast<char*>(it->first.c_str()), allocator).Move(), d_.GetAllocator());
        }
        m_frag_map.unlock();
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
    // writeJsonFrag(filename);
    writeRawFrag();

    time_t aftertime = time(0);
    double seconds = difftime(aftertime, beforetime);
    // std::cout << "frag.cc : frag " << frag_id << " (" << frag_map.size() << " items) written in " << seconds << " seconds." << std::endl;

    if (softMutexLock(m_frag_map)==true) {
        frag_map.clear();
        m_frag_map.unlock();
    }
}

/*
 * Write the raw frags, one for serving and one for indexing.
 * We first write to a temporary file with _ extension.
 * When done we attempt to get a lock on the original file as it may be getting read by the index server.
 * If we get a lock we move the _ file to the original filename
 * If we don't get a log we print an error and continue, the next crawl should fix it.
 */
void Frag::writeRawFrag() {
    std::string tmp_frag_filename = filename;
    tmp_frag_filename.append("_");

    std::ofstream f(tmp_frag_filename,std::ios::out | std::ios::binary);
    f << bits(frag_map) << "\n";
    f.close();

    int lck;
    if (fileLock(filename,lck) == true) {
        rename(tmp_frag_filename.c_str(),filename.c_str());
        fileUnlock(lck);
    } else {
        std::cout << "frag.cc : WARNING : unable to get lock on writeRawFrag." << std::endl;
    }
}

void Frag::writeJsonFrag() {
    std::string tmp_filename = filename;
    tmp_filename.append("_");

    rapidjson::Document d;
    serializeJSON(d);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    std::ofstream f{tmp_filename};
    f << (std::string)buffer.GetString();
    f.close();
    buffer.Clear();

    rename(tmp_filename.c_str(),filename.c_str());
}

void Frag::serializeJSON(rapidjson::Document &serialized_frag) {
    serialized_frag.Parse("{}");

    std::map<std::string, std::map<int, Frag::Item>>::iterator it;
    // std::cout << "serialize_ frag_map.size() " << frag_map.size() << std::endl;
    for (it = frag_map.begin(); it != frag_map.end(); it++) {
        rapidjson::Document::AllocatorType& allocator = serialized_frag.GetAllocator();
        rapidjson::Value fragItem_;
        fragItem_.SetObject();
        std::map<int, Frag::Item>::iterator tit;
        for (tit = it->second.begin(); tit != it->second.end(); tit++) {
            rapidjson::Value item_;
            item_.SetObject();
            item_.AddMember("doc_id", rapidjson::Value().SetInt((tit->second).doc_id), allocator);
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

std::vector<Frag::Item> Frag::getItems(int start, int end) {
    std::cout << "frag.cc : getItems" << std::endl;
    time_t beforeload = getTime();
    std::vector<Frag::Item> x(frag_mem_map + start, frag_mem_map + end);
    time_t afterload = getTime();
    double seconds = difftime(afterload, beforeload);
    std::cout << "frag.cc : " << "read " << x.size() << "docs from mmap in " << seconds << " miliseconds" << std::endl;
    return x;
}

/*
 * function to assign a weight to each item for each url.
 * standard calculation for this is the item frequency times the idf(inverse document frequency).
 * idf is usually calculated as log(no. docs in corpus/no. of documents that contain the item)
 * udpate can take time, do avoid deadlock/mutexes open a new connection just for this proceess
 * TODO : beacuse this is done for every indexfile regularly, opening a new connection each time is absurd.
 * We should batch these into large transactions
 */
void Frag::addWeights(int num_docs, std::string database, std::string lang) {
    pqxx::connection* C;
    auto config = getConfig();

    try {
		    C = new pqxx::connection("dbname = " + database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C->is_open()) {
            std::cout << "Opened database successfully: " << C->dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit;
    }

    std::string update_gram_idf;
    std::string gram;
    float gram_boost = 1.0;
    if (prefix_type==Frag::Type::UNIGRAM){
        update_gram_idf = "update_unigram_idf";
        gram = "unigrams";
    } else if (prefix_type==Frag::Type::BIGRAM){
        update_gram_idf = "update_bigram_idf";
        gram = "bigrams";
        gram_boost = 2.0;
    } else if (prefix_type==Frag::Type::TRIGRAM){
        update_gram_idf = "update_trigram_idf";
        gram = "trigrams";
        gram_boost = 3.0;
    } else {
        C->close();
        delete C;
        return;
    }
    time_t beforeload = getTime();

    pqxx::work txn(*C);

    C->prepare(update_gram_idf, "INSERT INTO " + gram + " (idf,gram,lang) VALUES ($1,$2,$3) ON CONFLICT "
            "ON CONSTRAINT " + gram + "_lang_gram_key DO UPDATE SET idf = $1, lang = $3 WHERE " + gram + ".gram = $2");

    if (softMutexLock(m_frag_map)==true) {
        for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
            int counter = 1;
            std::string pn_str = "(";
            std::string pv_str = "(";
            std::vector<std::string> pv;

            double idf = log((double)num_docs/(double)it->second.size());
            for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end(); ++tit) {
                tit->second.weight = idf*tit->second.tf*gram_boost;
            }
            // std::cout << "frag.cc addWeights it->first.c_str() " << " " << gram << " " << it->first.c_str() << std::endl;
            pqxx::result r = txn.exec_prepared(update_gram_idf, std::to_string(idf), it->first.c_str(), lang);
            /*
               int n = pv.size();
               for (int i=1; i<=n;i++) {
               pn_str += "$";
               pn_str += std::to_string(counter++);
               if (i == n) {
               pn_str += ") ";
               } else if (i % 3 == 0) {
               pn_str += "),(";
               } else {
               pn_str += ",";
               }
               }

               std::string ss =  "INSERT INTO " + gram + " (gram,lang,idf) VALUES " + pn_str + " ON CONFLICT "
               "ON CONSTRAINT " + gram + "_gram_key DO UPDATE SET idf = (SELECT v.v_idf FROM (VALUES " + pn_str + ") "
               "AS v(v_gram,v_lang,v_idf) WHERE " + gram + ".gram = v.v_gram AND " + gram + ".lang = v_lang)";
               std::cout << ss << std::endl;

               C->prepare(update_gram_idf, "INSERT INTO " + gram + " (gram,lang,idf) VALUES " + pn_str + " ON CONFLICT "
               "ON CONSTRAINT " + gram + "_gram_key DO UPDATE SET idf = (SELECT v.v_idf FROM (VALUES " + pn_str + ") "
               "AS v(v_gram,v_lang,v_idf) WHERE " + gram + ".gram = v.v_gram AND " + gram + ".lang = v_lang)");
               pqxx::prepare::invocation w_invocation = txn.prepared(update_gram_idf);
               prep_dynamic(pv, w_invocation);
               std::cout << " - exec start " << pv.size() << std::endl;
               pqxx::result r = w_invocation.exec();
               std::cout << " - exec end " << std::endl;
               pv.clear();
               */
        }
        m_frag_map.unlock();
    }
    time_t afterload = getTime();
    double seconds = difftime(afterload, beforeload);
    std::cout << "finish addWeights for " << update_gram_idf << " executed in " << seconds << " milliseconds." << std::endl;

    txn.commit();
    C->close();
    delete C;
}

/*
 * Similar to add weights above, the difference here is that we only purge docs from index.
 */
void Frag::purgeDocs(std::map<int,std::string> purge_docs) {
	time_t beforeload = getTime();
    if (softMutexLock(m_frag_map)==true) {
        for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end();) {
            int removed = 0;
            for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end();) {
                std::map<int,std::string>::iterator pit = purge_docs.find(tit->first);
                /*
                if ((it->first == "beirut") || (it->first == "cormac")) {
                    std::cout << "frag.cc debug  " << it->first << " " << path << frag_id << " - term : " << it->first <<  " - id : " << tit->first << " - doc_id : " << tit->second.doc_id << std::endl;
                }
                */
                if (pit != purge_docs.end()) {
                  /*
                    if ((it->first == "beirut") || (it->first == "cormac")) {
                        std::cout << "frag.cc debug "<<  it->first << " FOUND in doc id " << tit->second.doc_id << std::endl;
                    }
                   */
                    tit = it->second.erase(tit);
                    removed++;
                } else {
                    ++tit;
                }
            }
            // I've encountered scenarios where you can have empty maps in unknown circumstances.
            // probably from me messing with the index, but this could probably happen in the real world too say during crashes
            // this causes problems at serving time as you have have a good index (.idx) but the same terms in more than one frag
            // to heal this, just delete if empty.
            if (it->second.empty()) {
              /*
                if ((it->first == "beirut") || (it->first == "cormac")) {
                    std::cout << "frag.cc "<< path << " " << frag_id <<  " found empty map for " << it->first << std::endl;
                }
                if (removed > 0) {
                    std::cout << "frag.cc purged " << removed << " docs for term " << it->first << " from frag " << frag_id << std::endl; 
                }
               */
                it = frag_map.erase(it);
            } else {
                if (removed > 0) {
                //    std::cout << "frag.cc purged " << removed << " docs for term " << it->first << " from frag " << frag_id << std::endl; 
                }
                ++it;
            }
        }
        m_frag_map.unlock();
    }
    /*
    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
        if ((it->first == "beirut") || (it->first == "cormac")) {
            std::cout << "frag.cc after purge " << path << " - term : " << it->first <<  " map size " << it->second.size() << std::endl;
            for (std::map<int, Frag::Item>::iterator tit = it->second.begin(); tit != it->second.end(); ++tit) {
                std::cout << "frag.cc after purge_ " << path << " " << frag_id << " - term : " << it->first <<  " - id : " << tit->first << " - doc_id : " << tit->second.doc_id << std::endl;
            }
        }
    }
    */
	time_t afterload = getTime();
	double seconds = difftime(afterload, beforeload);
}

void Frag::insert(std::string s, std::map<int,Frag::Item> m) {
    /*
       std::cout << "frag_map.size()" << std::endl;
       std::cout << frag_map.size() << std::endl;
       std::cout << "s" << std::endl;
       std::cout << s << std::endl;
       for (std::map<int, Frag::Item>::iterator it = m.begin(); it != m.end(); ++it) {
         std::cout << " m : " << it->first << " : "  << it->second.doc_id << std::endl;
       }
    */
       
    if (softMutexLock(m_frag_map)==true) {
        frag_map.insert(std::pair<std::string,std::map<int,Frag::Item>>(s,m));
        m_frag_map.unlock();
    }
}

void Frag::update(std::string s, std::map<int,Frag::Item> m) {
    /*
    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
        std::cout << "frag.cc : " << path << " : before : " << it->first << " : size()  " << (it->second).size() << std::endl;
    }
    if ((s == "beirut") || (s == "cormac")) {
        for (std::map<int, Frag::Item>::iterator it = m.begin(); it != m.end(); ++it) {
            std::cout << "frag.cc frag id : " << s << " " << frag_id << " : new insert : " << s << " : " << it->first << " : " << it->second.doc_id << std::endl;
        }
    }
    for (std::map<std::string, std::map<int, Frag::Item>>::iterator it = frag_map.begin(); it != frag_map.end(); ++it) {
        std::cout << "frag.cc frag_id : " << frag_id << " : after : " << it->first << " : size()  " << (it->second).size() << std::endl;
    }
    */
    if (softMutexLock(m_frag_map)==true) {
        frag_map[s].insert(m.begin(), m.end());
        m_frag_map.unlock();
    }
    /*
    for (std::map<int, Frag::Item>::iterator it = m.begin(); it != m.end(); ++it) {
       frag_map[s].insert(std::pair<int, Frag::Item>(m->first, m->second);
    }
    */
}

void Frag::remove() {
    std::string esc_filename = filename;
    //  std::replace(esc_filename.begin(),esc_filename.end(),' ','_');
    if (std::remove(esc_filename.c_str()) != 0) {
        std::cout << "frag.cc : ERROR delete " << filename << " failed." << std::endl;
    } else {
        std::cout << "frag.cc : " << filename << " deleted." << std::endl;
    }
}

int Frag::getTime() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
}

