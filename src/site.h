#pragma once

#include <ctime>
#include <deque>
#include <string>
#include <thread>
#include <vector>

#include "status.h"

/**
 * directory ends with '/'
 */

enum SiteType {
    STLocal,
    STOss,
};

enum FileType { FTDirectory, FTFile };

std::wstring fileTypeToStdWstring(FileType fileType);

enum CmpResult {
    CSNone,
    CSEqual,
    CSNew,
    CSUpdated,
    CSOutdated,
    CSConflict, // dir and file have same name
};

struct FileStat {
    size_t size{0};
    std::time_t lastModifiedTime{0};
    std::string etag;
};

struct File {
    std::string name;
    FileType type;
    CmpResult cmp;
    FileStat stat;
    std::string location; // for bucket
    int icon{-2};
};

using FilePtr = std::shared_ptr<File>;

struct Dir;
using DirPtr = std::shared_ptr<Dir>;

struct Dir {
    Dir(const std::string &path) : path(path) {}

    std::string path;
    int tag{0}; // can hold additional info

    int dirCount{0};
    int fileCount{0};
    int totalSize{0};

    std::string comparePath;
    bool compareRunning{false};
    bool compareEnable{false};

    std::vector<FilePtr> files;

    DirPtr CopyBasic();
};

struct DirectoryListener {
    virtual void directoryUpdated(const std::string &site,
                                  const std::string &path) = 0;
};

// All methods should be called in main thread.
struct DirectoryCenter {
    DirectoryCenter();
    ~DirectoryCenter();
    void Register(DirectoryListener *listener);
    void Unregister(DirectoryListener *listener);
    void NotifyDirectoryUpdated(const std::string &site,
                                const std::string &path);

protected:
    std::mutex mtx_;
    std::vector<DirectoryListener *> listeners_;
};

DirectoryCenter *directoryCenter();

class Site;
using SitePtr = std::shared_ptr<Site>;

class SiteListener {
public:
    virtual ~SiteListener() = default;
    virtual void SiteUpdated(Site *site) = 0;
};

using SiteUpdatedCallback = std::function<void(Site *, Status)>;

class Site : public DirectoryListener {
public:
    Site(SiteType type);

    virtual ~Site();

    void WatchDirectoryCenter();

    void Attach(SiteListener *l);
    void Detach(SiteListener *l);

    virtual const std::string &name() const = 0;
    virtual bool IsOk() const = 0;

    /**
     * ChangeDir, ChangeToSubDir, Up, Refresh will submit task to thread pool
     * cb will always be called whether success or not.
     */

    void ChangeDir(const std::string &path, SiteUpdatedCallback cb = {});
    void ChangeToSubDir(const std::string &subdir, SiteUpdatedCallback cb = {});
    void Up(SiteUpdatedCallback cb = {});
    void Refresh(bool immediately = true, SiteUpdatedCallback cb = {});

    void Backward();
    void Forward();

    void directoryUpdated(const std::string &site,
                          const std::string &path) override;

    // block method to list dir, caller should put this into thread
    virtual Status GetDir(const std::string &path, DirPtr &dir) = 0;
    virtual Status MakeDir(const std::string &path) = 0;
    virtual Status Remove(const std::string &path) = 0;

    virtual Status Copy(const std::string &srcPath,
                        const std::string &dstPath,
                        const FileStat &stat = {}) = 0;

    virtual bool IsRoot() const = 0;

    virtual std::string GetParentPath() const = 0;

    virtual std::string GetETag(const std::string &path) const = 0;

    const DirPtr &GetCurrentDir() { return currentDir_; }
    const std::string &GetCurrentPath() const { return currentDir_->path; }

    bool CanBackward() const { return !backwards_.empty(); }
    bool CanForward() const { return !forwards_.empty(); }
    std::string GetBackwardPath() const;
    std::string GetForwardPath() const;

    const std::vector<FilePtr> &GetFiles() const { return currentDir_->files; }

    static std::pair<std::string, std::string> Split(const std::string &path);
    static std::string NamePart(const std::string &path);
    static std::string Combine(const std::string &base, const FilePtr &file);
    static std::string Combine(const std::string &base,
                               const std::string &path);

    const std::deque<DirPtr> &GetBackwards() const { return backwards_; }

    const std::deque<DirPtr> &GetForwards() const { return forwards_; }

    bool is_updating() const { return !updatingPath_.empty(); }

    bool is_refreshing() const {
        return (!updatingPath_.empty() &&
                (currentDir_->path == updatingPath_)) &&
               (pendingPath_.empty() || (currentDir_->path == pendingPath_));
    }

    SiteType type() const { return type_; }

    void NotifyChanged();

    std::string comparePath_;

protected:
    SiteType type_;
    bool directoryCenterWatched_{false};

    std::string updatingPath_;
    std::string pendingPath_;


    std::vector<SiteListener *> listeners_;

    DirPtr currentDir_;
    std::deque<DirPtr> backwards_;
    std::deque<DirPtr> forwards_;
};

class SiteHolder : public SiteListener {
public:
    SiteHolder() {}

    SiteHolder(const SitePtr &site) { AttachSite(site); }

    virtual ~SiteHolder() { DetachSite(); }

    void AttachSite(const SitePtr &site) {
        DetachSite();
        site_ = site;
        site_->Attach(this);
    }

    void DetachSite() {
        if (site_) {
            site_->Detach(this);
        }
    }

    const SitePtr &GetSite() const { return site_; }

protected:
    SitePtr site_;
};

// base and path both ended with '/'
bool IsSubDir(const std::string &base, const std::string &path);

// require IsSubDir
std::string RelativePath(const std::string &base, const std::string &path);
