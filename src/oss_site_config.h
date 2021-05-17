#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>

struct OssSiteNode {
    long id{0};
    std::string name;
    std::string region;
    std::string keyId;
    std::string keySecret;
    std::string lastLocalPath;
    std::string lastOssPath;
    std::vector<std::unique_ptr<OssSiteNode>> children;
};

using OssSiteNodePtr = std::shared_ptr<OssSiteNode>;
using OssSiteNodePtrVec = std::vector<OssSiteNodePtr>;

class OssSiteConfig {
public:
    void Load();
    void Save();
    OssSiteNodePtr Add();
    OssSiteNodePtr get(const std::string &name) const;
    void Remove(OssSiteNode *ossSiteNode);
    void Update(OssSiteNodePtr &ossSiteNode);
    void Update(OssSiteNode *ossSiteNode);

    bool NameExists(OssSiteNode *ossSiteNode, const std::string &name);

    OssSiteNodePtrVec sites;

private:
    std::string GetTempSiteName();
    mutable std::mutex mtx_;
};

OssSiteConfig *ossSiteConfig();
