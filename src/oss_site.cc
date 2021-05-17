#include "oss_site.h"
#include "local_site.h"
#include "options.h"
#include "oss_client.h"

#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

OssSite::OssSite(const std::string &name) : Site(STOss), name_(name) {
}

bool OssSite::IsOk() const {
    return true;
}

Status OssSite::GetDir(const std::string &path, DirPtr &dir) {
    if (path == OSSPROTOP) {
        return ListBuckets(dir);
    } else {
        return ListObjects(path, dir);
    }
}

Status OssSite::MakeDir(const std::string &path) {
    auto [bucket, pathPart] = SplitPath(path);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    // TODO if wstring, char literal should use wchar_t
    if (pathPart.back() != '/') {
        pathPart += '/';
    }
    std::shared_ptr<std::iostream> content =
            std::make_shared<std::stringstream>();
    auto outcome = ossClient->PutObject(bucket, pathPart, content);
    if (outcome.isSuccess()) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status OssSite::Remove(const std::string &path) {
    auto [bucket, name] = SplitPath(path);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    return Remove(ossClient, path);
}

Status OssSite::Remove(const std::shared_ptr<oss::OssClient> &ossClient,
                       const std::string &path) {
    if (path.back() == '/') {
        DirPtr dir;
        Status status = GetDir(path, dir);
        for (const auto &f : dir->files) {
            if (f->name == "..") {
                continue;
            }
            Status status = Remove(ossClient,
                                   path + f->name +
                                           (f->type == FTDirectory ? "/" : ""));
            RETURN_IF_FAIL(status);
        }
    }
    // 删除本身，目录本身也需要删除
    auto [bucket, name] = SplitPath(path);

    oss::DeleteObjectRequest delRequest(bucket, name);
    auto outcome = ossClient->DeleteObject(delRequest);
    if (outcome.isSuccess()) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status OssSite::CreateBucket(const std::string &name,
                             const std::string &region,
                             oss::StorageClass storageClass,
                             oss::CannedAccessControlList acl) {
    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClientByRegion(ossClient, name_, region);
    RETURN_IF_FAIL(status);

    oss::CreateBucketRequest request(name, storageClass, acl);
    auto outcome = ossClient->CreateBucket(request);
    if (outcome.isSuccess()) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status OssSite::RemoveBucket(const std::string &name) {
    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, name);
    RETURN_IF_FAIL(status);

    auto outcome = ossClient->DeleteBucket(name);
    if (outcome.isSuccess()) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

bool OssSite::IsRoot() const {
    return GetCurrentPath() == OSSPROTOP;
}

// see LocalSite::GetParentpath
std::string OssSite::GetParentPath() const {
    const std::string &currentPath = GetCurrentPath();
    if (currentPath == OSSPROTOP) {
        return currentPath;
    }
    size_t pos = currentPath.find_last_of('/');
    if (pos == 5) {
        return OSSPROTOP;
    }
    pos = currentPath.find_last_of('/', pos - 1);
    return currentPath.substr(0, pos + 1);
}

std::string OssSite::GetETag(const std::string &path) const {
    auto [bucket, name] = SplitPath(path);
    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    if (!status.ok()) {
        return "";
    }
    auto header = ossClient->HeadObject(bucket, name);
    if (header.isSuccess()) {
        auto &objectMetaData = header.result();
        auto &userMetaData = objectMetaData.UserMetaData();
        auto it = userMetaData.find("userETag");
        if (it != userMetaData.end()) {
            return it->second;
        }
    }
    return "";
}

bool OssSite::IsInBuckets() const {
    return GetCurrentPath() == OSSPROTOP;
}

std::pair<std::string, std::string> OssSite::SplitPath(
        const std::string &fullPath) {
    size_t bucketEnd = fullPath.find_first_of('/', 6);
    std::string bucket = fullPath.substr(6, bucketEnd - 6);
    std::string path = fullPath.substr(bucketEnd + 1);
    return std::make_pair(std::move(bucket), std::move(path));
}

Status OssSite::ListBuckets(DirPtr &dir) {
    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_);
    RETURN_IF_FAIL(status);

    oss::ListBucketsRequest request;
    oss::ListBucketsOutcome outcome = ossClient->ListBuckets(request);
    if (outcome.isSuccess()) {
        dir.reset(new Dir{OSSPROTOP});
        dir->tag = OssBucketsTag;
        for (const auto &bucket : outcome.result().Buckets()) {
            FilePtr file(new File);
            file->type = FTDirectory;
            file->name = bucket.Name();
            file->location = bucket.Location();
            SetBucketLocation(name_, file->name, file->location);
            std::istringstream intm(bucket.CreationDate());
            std::tm tm;
            intm >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            file->stat.lastModifiedTime = timegm(&tm);
            dir->files.push_back(file);
        }
        return Status::OK();
    } else {
        std::cout << outcome.error().Code() << ":" << outcome.error().Message()
                  << std::endl;
        return Status(EC_FAIL, "");
    }
}

Status OssSite::ListObjects(const std::string &path, DirPtr &dir) {
    dir.reset(new Dir{path});
    dir->tag = OssFilesTag;
    if (dir->path.back() != '/') {
        dir->path += "/";
    }

    FilePtr parent(new File);
    parent->name = "..";
    parent->type = FTDirectory;
    dir->files.push_back(parent);

    auto [bucket, prefix] = SplitPath(dir->path);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    bool isTruncated = false;
    std::string nextMarker = "";
    do {
        oss::ListObjectsRequest request(bucket);
        request.setPrefix(prefix);
        request.setDelimiter("/");
        request.setMaxKeys(640);
        request.setMarker(nextMarker);
        oss::ListObjectOutcome outcome = ossClient->ListObjects(request);
        if (outcome.isSuccess()) {
            for (const auto &p : outcome.result().CommonPrefixes()) {
                FilePtr file(new File);
                file->cmp = CSNone;
                file->type = FTDirectory;
                file->name =
                        p.substr(prefix.size(), p.size() - prefix.size() - 1);
                file->stat.lastModifiedTime = 0;
                dir->files.push_back(file);
                dir->dirCount++;
            }
            for (const auto &o : outcome.result().ObjectSummarys()) {
                if (o.Key() != prefix) {
                    FilePtr file(new File);
                    file->cmp = CSNone;
                    file->type = FTFile;
                    const auto &key = o.Key();
                    file->name = key.substr(prefix.size(),
                                            key.size() - prefix.size());
                    file->stat.size = o.Size();
                    std::istringstream intm(o.LastModified());
                    std::tm tm;
                    intm >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
                    file->stat.lastModifiedTime = timegm(&tm);
                    if (o.ETag().size() == 32) {
                        file->stat.etag = o.ETag();
                    }
                    dir->files.push_back(file);
                    dir->fileCount++;
                    dir->totalSize += file->stat.size;
                }
            }
            isTruncated = outcome.result().IsTruncated();
            nextMarker = outcome.result().NextMarker();
        } else {
            break;
        }
    } while (isTruncated);

    return Status::OK();
}

Status OssSite::Copy(const std::string &srcPath,
                     const std::string &dstPath,
                     const FileStat &stat) {
    if (srcPath.front() == '/') {
        return CopyFileFromLocal(srcPath, dstPath, stat);
    } else {
        return CopyFileToLocal(srcPath, dstPath, stat);
    }
}

Status OssSite::CopyFileFromLocal(const std::string &srcPath,
                                  const std::string &dstPath,
                                  const FileStat &stat) {
    auto [bucket, path] = SplitPath(dstPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    auto content = std::make_shared<std::fstream>(
            srcPath, std::ios_base::in | std::ios_base::binary);
    oss::PutObjectRequest request(bucket, path, content);
    bool uploadSpeedEnable = options().get_bool(OPTION_UPLOAD_SPEED_ENABLE);
    int uploadSpeedLimit = options().get_int(OPTION_UPLOAD_SPEED_LIMIT);
    if (uploadSpeedEnable && uploadSpeedLimit > 0) {
        request.setTrafficLimit(uploadSpeedLimit * 1024 * 8);
    }
    auto outcome = ossClient->PutObject(request);
    if (outcome.isSuccess()) {
        return Status::OK();
    } else {
        return Status(EC_FAIL, "");
    }
}

Status OssSite::CopyFileToLocal(const std::string &srcPath,
                                const std::string &dstPath,
                                const FileStat &stat) {
    // Ensure file was created
    std::ofstream ofs(dstPath);
    ofs.close();
    auto [bucket, path] = OssSite::SplitPath(srcPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    oss::GetObjectRequest request(bucket, path);
    bool downloadSpeedEnable = options().get_bool(OPTION_DOWNLOAD_SPEED_ENABLE);
    int downloadSpeedLimit = options().get_int(OPTION_DOWNLOAD_SPEED_LIMIT);
    if (downloadSpeedEnable && downloadSpeedLimit > 0) {
        request.setTrafficLimit(downloadSpeedLimit * 1024 * 8);
    }
    request.setResponseStreamFactory([&dstPath]() {
        return std::make_shared<std::fstream>(dstPath,
                                              (std::ios_base::out |
                                               std::ios_base::trunc |
                                               std::ios_base::binary));
    });
    auto outcome = ossClient->GetObject(request);
    if (outcome.isSuccess()) {
        if (stat.lastModifiedTime) {
            std::error_code ec;
            auto fileTimeT = decltype(std::filesystem::last_write_time(
                    dstPath))::clock::from_time_t(stat.lastModifiedTime);
            std::filesystem::last_write_time(dstPath, fileTimeT, ec);
        }
        return Status::OK();
    } else {
        return Status(EC_FAIL, "");
    }
}

Status OssSite::InitMultipartUpload(const std::string &srcPath,
                                    const std::string &dstPath,
                                    std::string &uploadId) {
    auto [bucket, pathPart] = SplitPath(dstPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    oss::InitiateMultipartUploadRequest multipartUploadRequest(bucket,
                                                               pathPart);
    multipartUploadRequest.MetaData().UserMetaData()["userETag"] =
            LocalSite::GetETagStatic(srcPath);
    auto multipartUploadResult =
            ossClient->InitiateMultipartUpload(multipartUploadRequest);
    if (multipartUploadResult.isSuccess()) {
        uploadId = multipartUploadResult.result().UploadId();
        return Status::OK();
    }

    return Status(EC_FAIL, "");
}

Status OssSite::CopyFileFromLocalPart(const std::string &srcPath,
                                      const std::string &dstPath,
                                      const std::string &uploadId,
                                      int partId,
                                      size_t offset,
                                      size_t size) {
    auto [bucket, path] = SplitPath(dstPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(
            srcPath, std::ios::in | std::ios::binary);
    content->seekg(offset, std::ios::beg);

    oss::UploadPartRequest uploadPartRequest(bucket, path, content);
    bool uploadSpeedEnable = options().get_bool(OPTION_UPLOAD_SPEED_ENABLE);
    int uploadSpeedLimit = options().get_int(OPTION_UPLOAD_SPEED_LIMIT);
    if (uploadSpeedEnable && uploadSpeedLimit > 0) {
        uploadPartRequest.setTrafficLimit(uploadSpeedLimit * 1024 * 8);
    }
    uploadPartRequest.setContentLength(size);
    uploadPartRequest.setUploadId(uploadId);
    uploadPartRequest.setPartNumber(partId);
    auto uploadPartOutcome = ossClient->UploadPart(uploadPartRequest);
    if (uploadPartOutcome.isSuccess()) {
        return Status::OK();
    } else {
        std::cout << uploadPartOutcome.error().Message() << std::endl;
    }

    return Status(EC_FAIL, "");
}

Status OssSite::CopyFileFromLocalPartFinish(const std::string &dstPath,
                                            const std::string &uploadId) {
    auto [bucket, path] = SplitPath(dstPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    oss::PartList partList;
    oss::ListPartsRequest listuploadrequest(bucket, path);
    listuploadrequest.setUploadId(uploadId);
    for (;;) {
        auto listUploadResult = ossClient->ListParts(listuploadrequest);
        if (listUploadResult.isSuccess()) {
            partList.insert(partList.end(),
                            listUploadResult.result().PartList().begin(),
                            listUploadResult.result().PartList().end());
        } else {
            return Status(EC_FAIL, "'");
        }
        if (!listUploadResult.result().IsTruncated()) {
            break;
        }
        listuploadrequest.setPartNumberMarker(
                listUploadResult.result().NextPartNumberMarker());
    }
    oss::CompleteMultipartUploadRequest request(bucket, path);
    request.setUploadId(uploadId);
    request.setPartList(partList);

    auto outcome = ossClient->CompleteMultipartUpload(request);
    if (outcome.isSuccess()) {
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status OssSite::CopyFileFromLocalPartAbort(const std::string &bucket,
                                           const std::string &path,
                                           const std::string &uploadId) {
    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    std::vector<std::pair<std::string, std::string>> uploads;
    if (!uploadId.empty()) {
        assert(!path.empty());
        uploads.push_back({path, uploadId});
    } else {
        oss::ListMultipartUploadsRequest listMultiUploadRequest(bucket);
        for (;;) {
            auto listResult =
                    ossClient->ListMultipartUploads(listMultiUploadRequest);
            if (listResult.isSuccess()) {
                for (const auto &part :
                     listResult.result().MultipartUploadList()) {
                    if (path.empty() || path == part.Key) {
                        uploads.push_back({part.Key, part.UploadId});
                    }
                }
            } else {
                return Status(EC_FAIL, "");
            }
            if (!listResult.result().IsTruncated()) {
                break;
            }
            listMultiUploadRequest.setKeyMarker(
                    listResult.result().NextKeyMarker());
            listMultiUploadRequest.setUploadIdMarker(
                    listResult.result().NextUploadIdMarker());
        }
    }

    for (auto &[path, uploadId] : uploads) {
        oss::AbortMultipartUploadRequest abortUploadRequest(
                bucket, path, uploadId);
        auto abortUploadIdResult =
                ossClient->AbortMultipartUpload(abortUploadRequest);
        if (!abortUploadIdResult.isSuccess()) {
        }
    }

    return Status::OK();
}

Status OssSite::CopyFileToLocalPart(std::shared_ptr<std::fstream> &out,
                                    const std::string &srcPath,
                                    size_t begin,
                                    size_t end) {
    auto [bucket, path] = OssSite::SplitPath(srcPath);

    std::shared_ptr<oss::OssClient> ossClient;
    Status status = getOssClient(ossClient, name_, bucket);
    RETURN_IF_FAIL(status);

    oss::GetObjectRequest request(bucket, path);
    bool downloadSpeedEnable = options().get_bool(OPTION_DOWNLOAD_SPEED_ENABLE);
    int downloadSpeedLimit = options().get_int(OPTION_DOWNLOAD_SPEED_LIMIT);
    if (downloadSpeedEnable && downloadSpeedLimit > 0) {
        request.setTrafficLimit(downloadSpeedLimit * 1024 * 8);
    }
    request.setRange(begin, end);
    request.setResponseStreamFactory([&out]() { return out; });
    auto outcome = ossClient->GetObject(request);
    if (outcome.isSuccess()) {
        return Status::OK();
    } else {
        return Status(EC_FAIL, "");
    }
}
