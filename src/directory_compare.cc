#include "directory_compare.h"
#include "global_executor.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <map>

void CompareDirectory(const SitePtr &lsite,
                      const SitePtr &rsite,
                      bool checkContent) {
    const DirPtr &ldir = lsite->GetCurrentDir();
    const DirPtr &rdir = rsite->GetCurrentDir();

    if (ldir->comparePath == rdir->path && rdir->comparePath == ldir->path) {
        if (!ldir->compareEnable) {
            ldir->compareEnable = true;
            if (!ldir->compareRunning) {
                lsite->NotifyChanged();
            }
        }
        if (!rdir->compareEnable) {
            rdir->compareEnable = true;
            if (!rdir->compareRunning) {
                rsite->NotifyChanged();
            }
        }
        return;
    }

    ldir->comparePath = rdir->path;
    rdir->comparePath = ldir->path;
    ldir->compareEnable = rdir->compareEnable = true;
    ldir->compareRunning = rdir->compareRunning = true;

    DirPtr ldirCopy = ldir->CopyBasic();
    DirPtr rdirCopy = rdir->CopyBasic();

    globalExecutor()->submit(
            [lsite, ldir, ldirCopy, rsite, rdir, rdirCopy, checkContent]() {
                int rc = CompareDirectory(
                        lsite, ldirCopy, rsite, rdirCopy, checkContent);
                wxTheApp->CallAfter(
                        [lsite, ldir, ldirCopy, rsite, rdir, rdirCopy]() {
                            if (ldir->comparePath == rdirCopy->path &&
                                rdir->comparePath == ldirCopy->path) {
                                for (size_t i = 0; i < ldir->files.size();
                                     i++) {
                                    ldir->files[i]->cmp =
                                            ldirCopy->files[i]->cmp;
                                }
                                for (size_t i = 0; i < rdir->files.size();
                                     i++) {
                                    rdir->files[i]->cmp =
                                            rdirCopy->files[i]->cmp;
                                }
                                ldir->compareRunning = false;
                                rdir->compareRunning = false;
                            }
                            if (lsite->GetCurrentDir() == ldir &&
                                rsite->GetCurrentDir() == rdir) {
                                lsite->NotifyChanged();
                                rsite->NotifyChanged();
                            }
                        });
            });
}

int CompareDirectory(const SitePtr &lsite,
                     const DirPtr &ldir,
                     const SitePtr &rsite,
                     const DirPtr &rdir,
                     bool checkContent) {
    assert(ldir->path.front() == '/');
    int rc = 0;

    std::map<std::string, const FilePtr &> lfiles;
    for (const auto &file : ldir->files) {
        if (file->name == "..") {
            file->cmp = CSNone;
            continue;
        }
        lfiles.emplace(file->name, file);
    }
    std::map<std::string, const FilePtr &> rfiles;
    for (auto &file : rdir->files) {
        if (file->name == "..") {
            file->cmp = CSNone;
            continue;
        }
        rfiles.emplace(file->name, file);
    }

    for (auto &[name, lfile] : lfiles) {
        auto it = rfiles.find(name);
        if (it == rfiles.end()) {
            lfile->cmp = CSNew;
            rc = 1;
        } else {
            auto &rfile = it->second;
            if (lfile->type != rfile->type) {
                lfile->cmp = rfile->cmp = CSConflict;
                rc = 1;
            } else {
                if (lfile->type == FTDirectory) {
                    // 文件夹结果为None，但需要进一步比较，所以rc=1
                    lfile->cmp = rfile->cmp = CSNone;
                    rc = 1;
                } else if (checkContent) {
                    bool equal = lfile->stat.size == rfile->stat.size;
                    if (equal) {
                        if (lfile->stat.etag.empty()) {
                            std::string path = ldir->path + lfile->name;
                            lfile->stat.etag = lsite->GetETag(path);
                        }
                        if (rfile->stat.etag.empty()) {
                            std::string path = rdir->path + rfile->name;
                            rfile->stat.etag = rsite->GetETag(path);
                        }
                        equal = lfile->stat.etag == rfile->stat.etag;
                    }
                    if (equal) {
                        lfile->cmp = rfile->cmp = CSEqual;
                    } else if (lfile->stat.lastModifiedTime <
                               rfile->stat.lastModifiedTime) {
                        lfile->cmp = CSOutdated;
                        rfile->cmp = CSUpdated;
                        rc = 1;
                    } else {
                        lfile->cmp = CSUpdated;
                        rfile->cmp = CSOutdated;
                        rc = 1;
                    }
                }
            }
        }
    }

    for (auto &[name, file] : rfiles) {
        if (lfiles.count(name) == 0) {
            file->cmp = CSNew;
            rc = 1;
        }
    }

    return rc;
}
