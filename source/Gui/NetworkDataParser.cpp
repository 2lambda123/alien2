#include "NetworkDataParser.h"

std::vector<RemoteSimulationData> NetworkDataParser::decodeRemoteSimulationData(boost::property_tree::ptree const& tree)
{
    std::vector<RemoteSimulationData> result;
    for (auto const& [key, subTree] : tree) {
        RemoteSimulationData entry;
        entry.id = subTree.get<std::string>("id");
        entry.userName = subTree.get<std::string>("userName");
        entry.simName = subTree.get<std::string>("simulationName");
        entry.description= subTree.get<std::string>("description");
        entry.width = subTree.get<int>("width");
        entry.height = subTree.get<int>("height");
        entry.particles = subTree.get<int>("particles");
        entry.version = subTree.get<std::string>("version");
        entry.timestamp = subTree.get<std::string>("timestamp");
        entry.contentSize = std::stoll(subTree.get<std::string>("contentSize"));

        bool isArray = false;
        int counter = 0;
        for (auto const& [likeTypeString, numLikesString] : subTree.get_child("likesByType")) {
            auto likes = std::stoi(numLikesString.data());
            if (likeTypeString.empty()) {
                isArray = true;
            }
            auto likeType = isArray ? counter : std::stoi(likeTypeString);
            entry.numLikesByEmojiType[likeType] = likes;
            ++counter;
        }
        entry.numDownloads = subTree.get<int>("numDownloads");
        entry.fromRelease = subTree.get<int>("fromRelease") == 1;
        entry.type = subTree.get<RemoteDataType>("type");
        result.emplace_back(entry);
    }
    return result;
}

std::vector<UserData> NetworkDataParser::decodeUserData(boost::property_tree::ptree const& tree)
{
    std::vector<UserData> result;
    for (auto const& [key, subTree] : tree) {
        UserData entry;
        entry.userName = subTree.get<std::string>("userName");
        entry.starsReceived = subTree.get<int>("starsReceived");
        entry.starsGiven = subTree.get<int>("starsGiven");
        entry.timestamp = subTree.get<std::string>("timestamp");
        entry.online = subTree.get<bool>("online");
        entry.lastDayOnline = subTree.get<bool>("lastDayOnline");
        entry.timeSpent = subTree.get<int>("timeSpent");
        entry.gpu = subTree.get<std::string>("gpu");
        result.emplace_back(entry);
    }
    return result;
}
