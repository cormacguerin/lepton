#include "proton.h"
#include <iostream>
#include <cpp_redis/cpp_redis>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace std;

cpp_redis::client client;


Proton::Proton()
{
}

Proton::~Proton()
{
}

void Proton::init() {
	client.connect();
}

void Proton::processFeeds() {
	std::cout << "processFeeds" << std::endl;

	std::vector<std::string> feeds;

	client.smembers("feeds", [&feeds](cpp_redis::reply& reply) {
			for (auto k: reply.as_array()) {
				feeds.push_back(k.as_string());
			}
	});

	client.sync_commit();
	/*
	for (std::map<std::string, std::string>::iterator mit = (*mechanics).begin(); mit != (*mechanics).end(); ++mit) {
		Render::Node* node = Render::NodeManager::getInstance().getNode(mit->first);
		cout << " MEHHHHH " << (mit->second).actors.size()  << endl;
		cout << mit->second.test.name << endl;
	*/

	for(std::vector<std::string>::iterator it = feeds.begin(); it != feeds.end(); ++it) {
		std::string unparsed_contents;
		client.hget("content_feed", *it, [it, &unparsed_contents](cpp_redis::reply& reply) {
			rapidjson::Document doc;
			const char *cstr = reply.as_string().c_str();
			doc.Parse(cstr);
			std::string quote = "\"";
			std::string key = quote + "test" + quote;
			std::cout << "key " << key << std::endl;

			const char *ckey = (*it).c_str();
	//		const char *ckey = key.c_str();
			std::cout << " : " << ckey << std::endl;
	//		const rapidjson::Value& a = doc["test"].GetString();
			printf("hello = %s\n", doc[ckey].GetString());
		});
	}
	client.sync_commit();

}

