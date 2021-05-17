#include "oss_site_config.h"

#include "oss_site.h"
#include "storage.h"
#include <wx/stdpaths.h>

#include <set>
#include <filesystem>
#include <fstream>
#include <thread>

OssSiteConfig *ossSiteConfig() {
    static std::unique_ptr<OssSiteConfig> inst(new OssSiteConfig);
    static std::once_flag once;
    std::call_once(once, []() { inst->Load(); });
    return inst.get();
}

void OssSiteConfig::Load() {
    std::lock_guard<std::mutex> lck(mtx_);
    sites = storage()->LoadSites();
}

void OssSiteConfig::Save() {
    for (const auto &site: sites) {
        storage()->UpdateSite(site);
    }
}

OssSiteNodePtr OssSiteConfig::Add() {
    std::lock_guard<std::mutex> lck(mtx_);
    auto &node = sites.emplace_back(new OssSiteNode);
    node->name = GetTempSiteName();
    return node;
}

OssSiteNodePtr OssSiteConfig::get(const std::string &name) const {
    std::lock_guard<std::mutex> lck(mtx_);
    for (const auto &node : sites) {
        if (node->name == name) {
            return node;
        }
    }

    return {};
}

void OssSiteConfig::Remove(OssSiteNode *ossSiteNode) {
    std::lock_guard<std::mutex> lck(mtx_);
    auto it = sites.begin();
    for (; it != sites.end(); it++) {
        if ((*it).get() == ossSiteNode) {
            OssSiteNodePtr node = *it;
            sites.erase(it);
            storage()->RemoveSite(node);
            break;
        }
    }
}

void OssSiteConfig::Update(OssSiteNodePtr &ossSiteNode) {
    if (ossSiteNode->lastLocalPath.empty()) {
        ossSiteNode->lastLocalPath =
                wxStandardPaths::Get().GetDocumentsDir().ToStdString();
    }
    if (ossSiteNode->lastOssPath.empty()) {
        ossSiteNode->lastOssPath = "oss://";
    }
    if (ossSiteNode->id == 0) {
        storage()->AppendSite(ossSiteNode);
    } else {
        storage()->UpdateSite(ossSiteNode);
    }
}

void OssSiteConfig::Update(OssSiteNode *ossSiteNode) {
    for (auto it = sites.begin(); it != sites.end(); it++) {
        if ((*it).get() == ossSiteNode) {
            OssSiteNodePtr ossSiteNodePtr = *it;
            Update(ossSiteNodePtr);
            break;
        }
    }
}

bool OssSiteConfig::NameExists(OssSiteNode *ossSiteNode,
                               const std::string &name) {
    for (const auto &node : sites) {
        if (node.get() != ossSiteNode && node->name == name) {
            return true;
        }
    }
    return false;
}

std::string OssSiteConfig::GetTempSiteName() {
    std::deque<OssSiteNode *> stack;
    for (const auto &siteNode : sites) {
        stack.push_back(siteNode.get());
    }
    std::set<std::string> names;
    while (!stack.empty()) {
        OssSiteNode *node = stack.front();
        stack.pop_front();
        if (!node->children.empty()) {
            for (const auto &child : node->children) {
                stack.push_back(child.get());
            }
        }
        std::string uname;
        std::transform(node->name.begin(),
                       node->name.end(),
                       std::back_inserter(uname),
                       [](unsigned char c) { return std::toupper(c); });
        names.insert(uname);
    }
    for (int index = 1;; index++) {
        std::string name = "OSS" + std::to_string(index);
        if (!names.count(name)) {
            return name;
        }
    }

    return "";
}
