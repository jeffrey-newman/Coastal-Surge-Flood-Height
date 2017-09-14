//
// Created by a1091793 on 19/8/17.
//


#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include "Pathify.hpp"

CmdLinePaths::CmdLinePaths():
        std::pair<std::string, boost::filesystem::path>()
{}

CmdLinePaths::CmdLinePaths(const std::string& path):
        std::pair<std::string, boost::filesystem::path>()
{
    this->first = path;
}

CmdLinePaths::CmdLinePaths(const char* path):
        std::pair<std::string, boost::filesystem::path>()
{
    this->first = std::string(path);
}

CmdLinePaths::CmdLinePaths(const std::string& _first, const boost::filesystem::path& _second) :
std::pair<std::string, boost::filesystem::path>(_first, _second)
{}

CmdLinePaths::CmdLinePaths(const CmdLinePaths & pr):
        std::pair<std::string, boost::filesystem::path>(pr)
{}

CmdLinePaths CmdLinePaths::operator=(const std::string &path)
{
    this->first = path;
    return *this;
}

CmdLinePaths CmdLinePaths::operator=(const char* path)
{
    this->first = std::string(path);
    return *this;
}

void
pathify(CmdLinePaths & path) throw(std::runtime_error, boost::filesystem::filesystem_error)
{
        path.second = boost::filesystem::path(path.first);
        if (!(boost::filesystem::exists(path.second)))
        {
            std::stringstream ss;
            ss << "Warning: path " << path.first << " does not exist\n";
            throw std::runtime_error(ss.str());
        }
}

void
pathify_mk(CmdLinePaths & path) throw(boost::filesystem::filesystem_error)
{
    path.second = boost::filesystem::path(path.first);
    if (!(boost::filesystem::exists(path.second.parent_path())))
    {
        boost::filesystem::create_directories(path.second.parent_path());
        std::cout << "path " << path.first << " did not exist, so created\n";
    }
}


std::ostream& operator<<(std::ostream& os, const CmdLinePaths& path) {
    os << path.first;
    return os;
}

std::istream& operator>>(std::istream& in, CmdLinePaths& path) {
    std::getline(in,path.first);
    return in;
}