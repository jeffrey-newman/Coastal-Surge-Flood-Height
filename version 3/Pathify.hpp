#ifndef PATHIFY_H
#define PATHIFY_H

#include <string>
#include <boost/filesystem.hpp>
#include <exception>

struct CmdLinePaths : public std::pair<std::string, boost::filesystem::path>
{
    CmdLinePaths();
    CmdLinePaths(const std::string & path);
    CmdLinePaths(const char* path);
    CmdLinePaths(const std::string& _first, const boost::filesystem::path& _second);
    CmdLinePaths(const CmdLinePaths & pr);

    CmdLinePaths operator = (const std::string & path);
    CmdLinePaths operator=(const char* path);
};

void
pathify(CmdLinePaths & path) throw(std::runtime_error, boost::filesystem::filesystem_error);

void
pathify_mk(CmdLinePaths & path) throw( boost::filesystem::filesystem_error);

std::ostream& operator<<(std::ostream& os, const CmdLinePaths& path);
std::istream& operator>>(std::istream& in, CmdLinePaths& path);


#endif // PATHIFY_H
