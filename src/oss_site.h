#pragma once

#include "oss_client.h"
#include "site.h"

#define OssBucketsTag 0x1
#define OssFilesTag 0x2

class OssSite : public Site {
public:
    OssSite(const std::string &name);

    ~OssSite() = default;

    const std::string &name() const override { return name_; }
    bool IsOk() const override;

    Status GetDir(const std::string &path, DirPtr &dir) override;
    Status MakeDir(const std::string &path) override;
    Status Remove(const std::string &path) override;

    Status CreateBucket(const std::string &name,
                        const std::string &reigon,
                        oss::StorageClass storageClass,
                        oss::CannedAccessControlList acl);
    Status RemoveBucket(const std::string &name);

    bool IsRoot() const override;

    std::string GetParentPath() const override;

    std::string GetETag(const std::string &path) const override;

    bool IsInBuckets() const;

    static std::pair<std::string, std::string> SplitPath(
            const std::string &fullPath);

    Status Copy(const std::string &srcPath,
                const std::string &dstPath,
                const FileStat &stat = {}) override;

    Status CopyFileFromLocal(const std::string &srcPath,
                             const std::string &dstPath,
                             const FileStat &stat = {});

    Status CopyFileToLocal(const std::string &srcPath,
                           const std::string &dstPath,
                           const FileStat &stat = {});

    Status InitMultipartUpload(const std::string &srcPath,
                               const std::string &dstPath,
                               std::string &uploadId);

    Status CopyFileFromLocalPart(const std::string &srcPath,
                                 const std::string &dstPath,
                                 const std::string &uploadId,
                                 int partId,
                                 size_t offset,
                                 size_t size);

    Status CopyFileFromLocalPartFinish(const std::string &dstPath,
                                       const std::string &uploadId);

    Status CopyFileFromLocalPartAbort(const std::string &bucket,
                                      const std::string &path,
                                      const std::string &uploadId);

    Status CopyFileToLocalPart(std::shared_ptr<std::fstream> &out,
                               const std::string &srcPath,
                               size_t begin,
                               size_t end);

    static bool CheckProto(const std::string &path) {
        return path.substr(0, 6) == OSSPROTOP;
    }

private:
    Status ListBuckets(DirPtr &dir);
    Status ListObjects(const std::string &path, DirPtr &dir);
    Status Remove(const std::shared_ptr<oss::OssClient> &ossClient,
                  const std::string &path);

    std::string name_;

    std::mutex mtx_;
    std::map<std::string, std::shared_ptr<oss::OssClient>> clients_;
};
