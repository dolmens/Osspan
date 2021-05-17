#include "oss_client.h"
#include "oss_site_config.h"

#include <mutex>
#include <thread>

std::mutex mtx;
std::map<std::string, std::map<std::string, std::shared_ptr<oss::OssClient>>>
        clients;
std::map<std::string, std::map<std::string, std::string>> locations;

const std::string defaultRegion = "oss-cn-hangzhou";

void SetBucketLocation(const std::string &site,
                       const std::string &bucket,
                       const std::string &location) {
    std::lock_guard<std::mutex> lck(mtx);
    locations[site][bucket] = location;
}

Status GetBucketLocation(std::string &location,
                         const std::string &site,
                         const std::shared_ptr<oss::OssClient> &siteClient,
                         const std::string &bucket) {
    auto &siteBuckets = locations[site];
    auto it = siteBuckets.find(bucket);
    if (it != siteBuckets.end()) {
        location = it->second;
        return Status::OK();
    }
    auto outcome = siteClient->GetBucketLocation(bucket);
    if (outcome.isSuccess()) {
        const std::string &result = outcome.result().Location();
        siteBuckets[bucket] = result;
        location = result;
        return Status::OK();
    }
    return Status(EC_FAIL, "");
}

Status getOssSiteClients(
        std::map<std::string, std::shared_ptr<oss::OssClient>> &clientsOfSite,
        const std::string &site) {
    auto it = clients.find(site);
    if (it != clients.end()) {
        clientsOfSite = it->second;
    } else {
        OssSiteNodePtr ossSiteNode = ossSiteConfig()->get(site);
        if (!ossSiteNode) {
            return Status(EC_FAIL, "");
        }

        const std::string &region = ossSiteNode->region.empty()
                                            ? defaultRegion
                                            : ossSiteNode->region;
        std::string endpoint = region + ".aliyuncs.com";
        oss::ClientConfiguration conf;
        // Cause speed may be limit to 100KB, and one part/segment is 10M.
        conf.requestTimeoutMs = 180 * 1000;
        std::shared_ptr<oss::OssClient> client(new oss::OssClient(
                endpoint, ossSiteNode->keyId, ossSiteNode->keySecret, conf));
        clientsOfSite[""] = client;
        clientsOfSite[region] = client;
        clients[site] = clientsOfSite;
    }

    return Status::OK();
}

Status getOssClient(std::shared_ptr<oss::OssClient> &ossClient,
                    const std::string &site,
                    const std::string &bucket) {
    std::lock_guard<std::mutex> lck(mtx);
    Status status;

    std::map<std::string, std::shared_ptr<oss::OssClient>> clientsOfSite;
    status = getOssSiteClients(clientsOfSite, site);
    RETURN_IF_FAIL(status);

    std::shared_ptr<oss::OssClient> &siteClient = clientsOfSite[""];
    if (bucket.empty()) {
        ossClient = siteClient;
        return Status::OK();
    }

    std::string location;
    status = GetBucketLocation(location, site, siteClient, bucket);
    RETURN_IF_FAIL(status);

    auto it = clientsOfSite.find(location);
    if (it != clientsOfSite.end()) {
        ossClient = it->second;
        return Status::OK();
    }

    OssSiteNodePtr ossSiteNode = ossSiteConfig()->get(site);
    if (!ossSiteNode) {
        return Status(EC_FAIL, "");
    }

    std::string endpoint = location + ".aliyuncs.com";
    oss::ClientConfiguration conf;
    // Cause speed may be limit to 100KB, and one part/segment is 10M.
    conf.requestTimeoutMs = 180 * 1000;
    std::shared_ptr<oss::OssClient> bucketClient(new oss::OssClient(
            endpoint, ossSiteNode->keyId, ossSiteNode->keySecret, conf));
    clientsOfSite[location] = bucketClient;
    ossClient = bucketClient;

    return Status::OK();
}

Status getOssClientByRegion(std::shared_ptr<oss::OssClient> &ossClient,
                            const std::string &site,
                            const std::string &region) {
    std::lock_guard<std::mutex> lck(mtx);
    Status status;

    std::map<std::string, std::shared_ptr<oss::OssClient>> clientsOfSite;
    status = getOssSiteClients(clientsOfSite, site);
    RETURN_IF_FAIL(status);

    auto it = clientsOfSite.find(region);
    if (it != clientsOfSite.end()) {
        ossClient = it->second;
        return Status::OK();
    }

    OssSiteNodePtr ossSiteNode = ossSiteConfig()->get(site);
    if (!ossSiteNode) {
        return Status(EC_FAIL, "");
    }

    std::string endpoint = region + ".aliyuncs.com";
    oss::ClientConfiguration conf;
    // Cause speed may be limit to 100KB, and one part/segment is 10M.
    conf.requestTimeoutMs = 180 * 1000;
    std::shared_ptr<oss::OssClient> bucketClient(new oss::OssClient(
            endpoint, ossSiteNode->keyId, ossSiteNode->keySecret, conf));
    clientsOfSite[region] = bucketClient;
    ossClient = bucketClient;

    return Status::OK();
}
