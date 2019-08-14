#include "quark.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <algorithm>
#include <unistd.h>

using namespace pqxx;

Quark::Quark()
{
}

Quark::~Quark()
{
}

void Quark::init() {

	// postgres connection
	try {
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
    	if (C->is_open()) {
			std::cout << "Opened database successfully: " << C->dbname() << std::endl;
    	} else {
			std::cout << "Can't open database" << std::endl;
    	}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
}

void Quark::buildIndex() {

	C->prepare("process", "SELECT * FROM docngrams ORDER BY gram_id, incidence DESC");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("process").exec();
	txn.commit();

	std::vector<int> urls;

	std::string gram_tracker;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		pqxx::field url_id = (row)[0];
		pqxx::field gram_id = (row)[1];
		pqxx::field incidence = (row)[2];
		if (gram_tracker != gram_id.c_str()) {
			urls.empty();
		} else {
			urls.push_back(atoi(url_id.c_str()));
		}
		gram_tracker = gram_id.c_str();

		//std::cout << "gram_id : " << gram_id.c_str() << " url_id : " <<  url_id.c_str() << " incidence : " << incidence << std::endl;

		//gramsmap[gram_id].push(url_id);

		if (url_id.is_null()) {
			std::cout << "skip : url is null" << std::endl;;
			continue;
		}
		if (gram_id.is_null()) {
			std::cout << "skip : gram_id is null" << std::endl;;
			continue;
		}
		if (incidence.is_null()) {
			std::cout << "skip : incidence is null" << std::endl;;
			continue;
		}
	}
	
	// ngram_ids = ngram_ids_r[0][0].as<int>();
	// separate queries
	/*
	C->prepare("numgrams", "SELECT id FROM ngrams");
	pqxx::work txn(*C);
	pqxx::result ngram_ids_r = txn.prepared("numgrams").exec();
	txn.commit();
	for (pqxx::result::const_iterator ngram_row = ngram_ids_r.begin(); ngram_row != ngram_ids_r.end(); ++ngram_row) {
		std::cout << "ngram_row "  << ngram_row[0] << std::endl;
		int gram_id = atoi(ngram_row[0].c_str());

		C->prepare("process", "SELECT * FROM docngrams WHERE gram_id=$1 ORDER BY incidence DESC");
		pqxx::work txn(*C);
		pqxx::result docgrams_r = txn.prepared("process")(gram_id).exec();
		txn.commit();

		for (pqxx::result::const_iterator docngrams_row = docgrams_r.begin(); docngrams_row != docgrams_r.end(); ++docngrams_row) {
			const pqxx::field url_id = (docngrams_row)[0];
			const pqxx::field gram_id = (docngrams_row)[1];
			const pqxx::field incidence = (docngrams_row)[2];

			std::cout << "url : " << gram_id.c_str() << " " <<  url_id.c_str() << " " << incidence << std::endl;

			if (url_id.is_null()) {
				std::cout << "skip : url is null" << std::endl;;
				continue;
			}
			if (gram_id.is_null()) {
				std::cout << "skip : gram_id is null" << std::endl;;
				continue;
			}
			if (incidence.is_null()) {
				std::cout << "skip : incidence is null" << std::endl;;
				continue;
			}
		}
	}
	*/

}

void Quark::startServing() {
}

