#pragma once

#include "status.h"
#include <alibabacloud/oss/OssClient.h>

namespace oss = AlibabaCloud::OSS;

#define OSSPROTOP "oss://"
#define OSSPROTOPLEN 6

Status getOssClient(std::shared_ptr<oss::OssClient> &ossClient,
                    const std::string &site,
                    const std::string &bucket = std::string(""));

Status getOssClientByRegion(std::shared_ptr<oss::OssClient> &ossClient,
                            const std::string &site,
                            const std::string &region);

void SetBucketLocation(const std::string &site,
                       const std::string &bucket,
                       const std::string &location);
