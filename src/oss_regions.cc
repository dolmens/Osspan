#include "oss_regions.h"

const std::vector<const char *> &ossRegions() {
    static std::vector<const char *> regions{
            "",
            "华东1（杭州）",
            "华东2（上海）",
            "华北1（青岛）",
            "华北2（北京）",
            "华北 3（张家口）",
            "华北5（呼和浩特）",
            "华北6（乌兰察布）",
            "华南1（深圳）",
            "华南2（河源）",
            "华南3（广州）",
            "西南1（成都）",
            "中国（香港）",
            "美国西部1（硅谷）",
            "美国东部1（弗吉尼亚）",
            "亚太东南1（新加坡）",
            "亚太东南2（悉尼）",
            "亚太东南3（吉隆坡）",
            "亚太东南5（雅加达）",
            "亚太东北1（日本）",
            "亚太南部1（孟买）",
            "欧洲中部1（法兰克福）",
            "英国（伦敦）",
            "中东东部1（迪拜）",
    };

    return regions;
}

const std::vector<const char *> &ossRegionCodes() {
    static std::vector<const char *> codes{
            "",
            "oss-cn-hangzhou",
            "oss-cn-shanghai",
            "oss-cn-qingdao",
            "oss-cn-beijing",
            "oss-cn-zhangjiakou",
            "oss-cn-huhehaote",
            "oss-cn-wulanchabu",
            "oss-cn-shenzhen",
            "oss-cn-heyuan",
            "oss-cn-guangzhou",
            "oss-cn-chengdu",
            "oss-cn-hongkong",
            "oss-us-west-1",
            "oss-us-east-1",
            "oss-ap-southeast-1",
            "oss-ap-southeast-2",
            "oss-ap-southeast-3",
            "oss-ap-southeast-5",
            "oss-ap-northeast-1",
            "oss-ap-south-1",
            "oss-eu-central-1",
            "oss-eu-west-1",
            "oss-me-east-1",
    };

    return codes;
}
