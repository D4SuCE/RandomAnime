#include <ctime>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "parser.h"
#include <iostream>
#include "nlohmann/json.hpp"

Parser::Parser(int urlsCount)
{
	url = "https://animego.org/anime/random";
	this->urlsCount = urlsCount;
	threadsCount = urlsCount <= 20 ? std::thread::hardware_concurrency() - 1 :
		std::thread::hardware_concurrency() - 1 > 4 ? 4 : std::thread::hardware_concurrency() - 1;
	chunkSize = (urlsCount + threadsCount - 1) / threadsCount;
}

Parser::~Parser()
{
}

size_t Parser::writeCallBack(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void Parser::parseCurrentUrl(const std::string& url)
{
	std::this_thread::sleep_for(std::chrono::seconds(2));
	CURL* curl = curl_easy_init();
	std::string html;
	if (curl)
	{
		long httpCode = 0;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
		curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		curl_easy_cleanup(curl);
		if (httpCode == 200)
		{
			std::string name = getAnimeName(html);
			std::string rating = getAnimeRating(html);
			std::string type = getAnimeType(html);
			std::string episodes = getAnimeEpisodes(html);
			std::vector<std::string> genres = getAnimeGenres(html);
			std::string link = getCurrentAnimeUrl(html);
			std::lock_guard<std::mutex> lock(mutex);
			buffer.emplace_back(name, rating, type, episodes, genres, link);
		}
	}
}

void Parser::parseRandomUrl()
{
	std::this_thread::sleep_for(std::chrono::seconds(2));
	CURL* curl = curl_easy_init();
	std::string html;
	if (curl)
	{
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res == CURLE_OK)
		{
			std::string url = getRandomAnimeUrl(html);
			parseCurrentUrl(url);
		}
	}
}

void Parser::processUrls()
{
	for (int i = 0; i < chunkSize; i++)
	{
		parseRandomUrl();
	}
}

std::string Parser::getRandomAnimeUrl(const std::string& html)
{
	try
	{
		std::string animeUrl = "https://animego.org" + html.substr(html.find("/anime/"));
		animeUrl = animeUrl.substr(0, animeUrl.find("\'\" />"));
		return animeUrl;
	}
	catch (...)
	{
		return "";
	}
}

std::string Parser::getCurrentAnimeUrl(const std::string& html)
{
	const std::string tag = "<link href=\"";
	std::string animeUrl = html.substr(html.find(tag + "https://animego.org") + tag.size());
	animeUrl = animeUrl.substr(0, animeUrl.find("\" "));
	return animeUrl;
}

std::vector<std::string> Parser::getAnimeGenres(const std::string& html)
{
	std::vector<std::string> genres;

	const std::string genreMarker = "<dd class=\"col-6 col-sm-8 mb-1 overflow-h\">";
	const std::string genreEndMarker = "</dd>";
	std::string genreHtml = html.substr(html.find(genreMarker));
	genreHtml = genreHtml.substr(0, genreHtml.find(genreEndMarker));
	auto genreStartPos = genreHtml.find(genreMarker);

	while (genreStartPos != std::string::npos)
	{
		auto anchorStartPos = genreHtml.find("<a href=", genreStartPos);
		while (anchorStartPos != std::string::npos)
		{
			auto anchorEndPos = genreHtml.find(">", anchorStartPos);
			if (anchorEndPos != std::string::npos)
			{
				std::string genre = genreHtml.substr(anchorEndPos + 1, genreHtml.find("</a>", anchorEndPos) - (anchorEndPos + 1));
				genres.push_back(genre);
			}
			anchorStartPos = genreHtml.find("<a href=", anchorEndPos);
		}
		genreStartPos = genreHtml.find(genreMarker, genreStartPos + 1);
	}

	return genres;
}

std::string Parser::getAnimeName(const std::string& html)
{
	const int offset = 22;
	std::string name = html.substr(html.find("anime-title") + offset);
	name = name.substr(0, name.find("</h1>"));
	return name;
}

std::string Parser::getAnimeType(const std::string& html)
{
	const std::string tag = "col-6 col-sm-8 mb-1\">";
	std::string type = html.substr(html.find(tag) + tag.size());
	type = type.substr(0, type.find("</dd>"));
	return type;
}

std::string Parser::getAnimeRating(const std::string& html)
{
	std::string rating;

	if (html.find("rating-value") != std::string::npos)
	{
		const int offset = 14;
		rating = html.substr(html.find("rating-value") + offset);
		rating = rating.substr(0, rating.find("</span>"));
		std::replace(rating.begin(), rating.end(), ',', '.');
	}
	else
	{
		rating = "Нет оценок";
	}

	return rating;
}

std::string Parser::getAnimeEpisodes(const std::string& html)
{
	const std::string tag = "text-gray-dark-6\">Эпизоды</dt>";

	if (html.find(tag) == std::string::npos)
	{
		return "";
	}

	std::string episodes = html.substr(html.find(tag) + tag.size());
	episodes = episodes.substr(0, episodes.find("</dd>"));
	const std::string episodeTag = "col-6 col-sm-8 mb-1\">";
	const std::string episodeSpanTag = "<span>";

	if (episodes.find(episodeSpanTag) == std::string::npos)
	{
		episodes = episodes.substr(episodes.find(episodeTag) + episodeTag.size());
	}
	else
	{
		episodes = episodes.substr(episodes.find(episodeTag) + episodeTag.size());
		std::string maxEpisodes = episodes.substr(episodes.find(episodeSpanTag) + episodeSpanTag.size());
		maxEpisodes = maxEpisodes.substr(0, maxEpisodes.find("</span>"));
		episodes = episodes.substr(0, episodes.find("<span>"));
		episodes += maxEpisodes;
	}

	return episodes;
}

void Parser::parse()
{
	curl_global_init(CURL_GLOBAL_ALL);

	for (int i = 0; i < threadsCount && i < urlsCount; i++)
	{
		threads.emplace_back(&Parser::processUrls, this);
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	curl_global_cleanup();
}

int Parser::getThreadsCount()
{
	return threadsCount;
}

void Parser::exportJSON()
{
	using namespace nlohmann;
	
	ordered_json data;

	for (const auto& anime : buffer)
	{
		ordered_json animeData;
		animeData["Название"] = anime.name;
		animeData["Рейтинг"] = anime.rating;

		if (!anime.episodes.empty())
		{
			animeData["Эпизоды"] = anime.episodes;
		}

		animeData["Тип"] =  anime.type;

		std::string genreString;
		for (const auto& genre : anime.genres)
		{
			genreString += genre + ", ";
		}
		genreString = genreString.substr(0, genreString.size() - 2);

		animeData["Жанр"] = genreString;
		animeData["Ссылка"] = anime.link;
		data.push_back(animeData);
	}

	auto now = std::chrono::system_clock::now();

	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm = *std::localtime(&now_time);

	std::ostringstream os;
	os << std::put_time(&now_tm, "%H.%M.%S");
	std::string filename = "anime_" + os.str() + ".json";

	std::ofstream file(filename);
	file << data.dump(4) << std::endl;
	file.close();
}

std::vector<Anime> Parser::getAnime()
{
	return buffer;
}