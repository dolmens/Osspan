#include "local_site.h"
#include "oss_client.h"

#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace {

const std::regex &part_regex() {
    static std::regex re("^\\..+\\.osspanpart\\.[0-9]+$");
    return re;
}
} // namespace

Status LocalSite::GetLocalDir(const std::string &path,
                              DirPtr &dir) {
    std::error_code ec;
    auto dirIter = fs::directory_iterator(path, ec);
    if (ec) {
        return Status(EC_FAIL, "");
    }
    dir.reset(new Dir{path});
    if (path != "/") {
        FilePtr parent(new File);
        parent->name = "..";
        parent->type = FTDirectory;
        dir->files.push_back(parent);
    }
    for (const auto &e : dirIter) {
        std::string filename = e.path().filename();
        if (std::regex_match(filename, part_regex())) {
            continue;
        }
        FilePtr file(new File);
        file->cmp = CSNone;
        file->name = std::move(filename);
        auto t = e.last_write_time(ec);
        if (ec) {
            continue;
        }
        file->stat.lastModifiedTime = decltype(t)::clock::to_time_t(t);
        if (e.is_directory()) {
            dir->dirCount++;
            file->type = FTDirectory;
        } else if (e.is_regular_file()) {
            dir->fileCount++;
            file->type = FTFile;
            file->stat.size = e.file_size(ec);
            if (ec) {
                continue;
            }
            dir->totalSize += file->stat.size;
        }
        dir->files.push_back(file);
    }
    std::sort(dir->files.begin(),
              dir->files.end(),
              [](const auto &l, const auto &r) {
                  return (l->type < r->type) ||
                         (l->type == r->type && l->name < r->name);
              });
    return Status::OK();
}

Status LocalSite::MakeLocalDir(const std::string &path) {
    std::error_code ec;
    fs::create_directory(path, ec);
    if (!ec) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status LocalSite::SetLastModifiedTime(const std::string &path, time_t tm) {
    std::error_code ec;
    auto fileTimeT = decltype(
            std::filesystem::last_write_time(path))::clock::from_time_t(tm);
    std::filesystem::last_write_time(path, fileTimeT, ec);
    return Status::OK();
}

Status LocalSite::RemoveLocalDir(const std::string &path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    if (!ec) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status LocalSite::GetDir(const std::string &path, DirPtr &dir) {
    return GetLocalDir(path, dir);
}

Status LocalSite::MakeDir(const std::string &path) {
    return MakeLocalDir(path);
}

Status LocalSite::Remove(const std::string &path) {
    return RemoveLocalDir(path);
}

Status LocalSite::Copy(const std::string &srcPath,
                       const std::string &dstPath,
                       const FileStat &stat) {
    return Status::OK();
}

Status LocalSite::ConcatParts(const std::string &dstPath, size_t cnt) {
    std::ofstream out(dstPath, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        return Status(EC_FAIL, "");
    }

    std::string dstPathPart =
            fs::path(dstPath).parent_path().string() + std::string("/.") +
            fs::path(dstPath).filename().string() + ".osspanpart.";
    for (size_t i = 1; i <= cnt; i++) {
        std::ifstream in(dstPathPart + std::to_string(i),
                         std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return Status(EC_FAIL, "");
        }
        out << in.rdbuf();
    }

    for (size_t i = 1; i <= cnt; i++) {
        std::error_code ec;
        fs::remove(dstPathPart + std::to_string(i), ec);
    }

    return Status::OK();
}

bool LocalSite::IsRoot() const {
    return GetCurrentPath() == "/";
}

/**
 * '/'       -> '/'
 * '/a'      -> '/'
 * '/a/'     -> '/'
 * '/a/b'    -> '/' * '/a/b/'   -> '/a/' * '/a/b/c'  -> '/a/'
 * '/a/b/c/' -> '/a/b/'
 */
std::string LocalSite::GetParentPath() const {
    const std::string &currentPath = GetCurrentPath();
    if (currentPath == "/") {
        return currentPath;
    }
    size_t pos = currentPath.find_last_of('/');
    if (pos == 0) {
        return "/";
    }
    pos = currentPath.find_last_of('/', pos - 1);
    return currentPath.substr(0, pos + 1);
}

std::string LocalSite::GetETag(const std::string &path) const {
    return GetETagStatic(path);
}

std::string LocalSite::GetETagStatic(const std::string &path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    return oss::ComputeContentETag(in);
}
