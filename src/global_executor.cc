#include "global_executor.h"

Executor *globalExecutor() {
    static ScheduledThreadPoolExecutor executor;
    return &executor;
}
