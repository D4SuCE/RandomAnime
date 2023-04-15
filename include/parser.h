#pragma once

#include <mutex>
#include <thread>
#include "anime.h"
#include <curl/curl.h>

class Parser
{
private:
	std::string url;
	int urlsCount;
	int threadsCount;
	int chunkSize;
	std::mutex mutex;
	std::vector<std::thread> threads;
	std::vector<Anime> buffer;
	
public:
	Parser(int urlsCount = 20);
	~Parser();

private:
	void processUrls();
	void parseRandomUrl();
	void parseCurrentUrl(const std::string& url);
	std::string getAnimeName(const std::string& html);
	std::string getAnimeType(const std::string& html);
	std::string getAnimeRating(const std::string& html);
	std::string getAnimeEpisodes(const std::string& html);
	std::string getRandomAnimeUrl(const std::string& html);
	std::string getCurrentAnimeUrl(const std::string& html);
	std::vector<std::string> getAnimeGenres(const std::string& html);
	static size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* userp);

public:
	void parse();
	int getThreadsCount();
	void exportJSON();
	std::vector<Anime> getAnime();
};