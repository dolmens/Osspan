#pragma once

#include "site.h"

class LocalSite : public Site {
public:
    LocalSite() : Site(STLocal) {}
    ~LocalSite() = default;

    const std::string &name() const override {
        static const std::string empty;
        return empty;
    }

    bool IsOk() const override { return true; }

    static Status GetLocalDir(const std::string &path, DirPtr &dir);
    static Status MakeLocalDir(const std::string &path);
    static Status RemoveLocalDir(const std::string &path);
    static Status SetLastModifiedTime(const std::string &path, time_t tm);

    Status GetDir(const std::string &path, DirPtr &dir) override;
    Status MakeDir(const std::string &path) override;
    Status Remove(const std::string &path) override;

    Status Copy(const std::string &srcPath,
                const std::string &dstPath,
                const FileStat &stat = {}) override;

    Status ConcatParts(const std::string &dstPath, size_t cnt);

    bool IsRoot() const override;

    std::string GetParentPath() const override;

    std::string GetETag(const std::string &path) const override;

    static std::string GetETagStatic(const std::string &path);
};
