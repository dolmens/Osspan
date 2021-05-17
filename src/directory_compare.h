#pragma once

#include "site.h"

void CompareDirectory(const SitePtr &lsite,
                      const SitePtr &rsite,
                      bool checkContent = true);

int CompareDirectory(const SitePtr &lsite,
                     const DirPtr &ldir,
                     const SitePtr &rsite,
                     const DirPtr &rdir,
                     bool checkContent = true);
