#include "storage.h"

#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "osspanapp.h"

namespace {

enum ColumnType {
    CTInteger,
    CTText,
};

enum {
    NotNull = 0x1,
    DefaultNull = 0x2,
    PrimaryKey = 0x4,
};

struct Column {
    std::string_view name;
    ColumnType type;
    unsigned int flags;
};

struct Table {
    std::string_view name;
    std::vector<Column> columns;
    sqlite3_stmt *select;
    sqlite3_stmt *insert;
    sqlite3_stmt *update;
    sqlite3_stmt *remove;
};

namespace TableSiteColumns {
enum {
    id,
    name,
    region,
    keyId,
    keySecret,
    lastLocalPath,
    lastOssPath,
    lastId,
};
}

Table TableSite{
        "sites",
        {
                {"id", CTInteger, PrimaryKey | NotNull},
                {"name", CTText, NotNull},
                {"region", CTText, NotNull},
                {"keyId", CTText, NotNull},
                {"keySecret", CTText, NotNull},
                {"lastLocalPath", CTText, NotNull},
                {"lastOssPath", CTText, NotNull},
        },
        nullptr,
        nullptr,
        nullptr,
        nullptr,
};

namespace TableScheduleColumns {
enum {
    id,
    name,
    srcSite,
    srcPath,
    dstSite,
    dstPath,
    flag,
    routine,
    lastStartTime,
    lastFinishTime,
    lastId,
};
}

Table TableSchedule{
        "schedules",
        {
                {"id", CTInteger, PrimaryKey | NotNull},
                {"name", CTText, NotNull},
                {"srcSite", CTText, NotNull},
                {"srcPath", CTText, NotNull},
                {"dstSite", CTText, NotNull},
                {"dstPath", CTText, NotNull},
                {"flag", CTInteger, NotNull},
                {"routine", CTInteger, NotNull},
                {"lastStartTime", CTInteger, NotNull},
                {"lastFinishTime", CTInteger, NotNull},
        },
        nullptr,
        nullptr,
        nullptr,
        nullptr,
};

namespace TableTaskColumns {
enum {
    id,
    parent,
    schedule,
    type,
    status,
    srcSite,
    srcPath,
    dstSite,
    dstPath,
    size,
    lastModifiedTime,
    flag,
    uploadId,
    offset,
    partId,
    progress,
    startTime,
    finishTime,
    lastId,
};
} // namespace TableTaskColumns

Table TableTask{
        "tasks",
        {
                {"id", CTInteger, PrimaryKey | NotNull},
                {"parent", CTInteger, NotNull},
                {"schedule", CTInteger, NotNull},
                {"type", CTInteger, NotNull},
                {"status", CTInteger, NotNull},
                {"srcSite", CTText, NotNull},
                {"srcPath", CTText, NotNull},
                {"dstSite", CTText, NotNull},
                {"dstPath", CTText, NotNull},
                {"size", CTInteger, NotNull},
                {"lastModifiedTime", CTInteger, NotNull},
                {"flag", CTInteger, NotNull},
                {"uploadId", CTText, NotNull},
                {"offset", CTInteger, NotNull},
                {"partId", CTInteger, NotNull},
                {"progress", CTInteger, NotNull},
                {"startTime", CTInteger, 0},
                {"finishTime", CTInteger, 0},
        },
        nullptr,
        nullptr,
        nullptr,
        nullptr,
};

std::vector<Table *> schemas{
        &TableSite,
        &TableTask,
        &TableSchedule,
};

} // namespace

class Storage::Impl {
public:
    Impl();
    ~Impl();

    OssSiteNodePtrVec LoadSites();
    void AppendSite(OssSiteNodePtr &site);
    void UpdateSite(const OssSiteNodePtr &site);
    void RemoveSite(const OssSiteNodePtr &site);

    SchedulePtrVec LoadSchedules();
    void AppendSchedule(const SchedulePtr &schedule);
    void UpdateSchedule(const SchedulePtr &schedule);
    void RemoveSchedule(const SchedulePtr &schedule);

    TaskPtrVec LoadTasks();
    void BuildTaskHierarchy(TaskPtrVec &v);
    void AppendTask(const TaskPtr &task);
    void UpdateTask(const TaskPtr &task);
    void RemoveTask(const TaskPtr &task);

    void CreateTables();
    void CreateColumnDef(std::ostringstream &ss, const Column &column);
    void PrepareSelectStatement(Table *table);
    void PrepareInsertStatement(Table *table);
    void PrepareUpdateStatement(Table *table);
    void PrepareDeleteStatement(Table *table);
    sqlite3_stmt *PrepareStatement(const std::string &query);

    bool Bind(sqlite3_stmt *stmt, int index, int value);
    bool Bind(sqlite3_stmt *stmt, int index, int64_t value);
    bool Bind(sqlite3_stmt *stmt, int index, const std::string &value);

    int GetColumnInt(sqlite3_stmt *stmt, int index, int def);
    int64_t GetColumnInt64(sqlite3_stmt *stmt, int index, int64_t def);
    std::string GetColumnString(sqlite3_stmt *stmt, int index);

    bool BeginTransaction();
    bool EndTransaction(bool rollback);

private:
    sqlite3 *db{nullptr};
};

Storage::Impl::Impl() {
    std::string dataPath = wxGetApp().GetUserDataDir().ToStdString();
    std::string dbFile = dataPath + "/Osspan.db";
    int ret = sqlite3_open(dbFile.c_str(), &db);
    if (ret != SQLITE_OK) {
        throw "sqlite3 open";
    }
    CreateTables();
}

Storage::Impl::~Impl() {
    for (auto *table : schemas) {
        sqlite3_finalize(table->select);
        sqlite3_finalize(table->insert);
        if (table->update) {
            sqlite3_finalize(table->update);
        }
    }
    sqlite3_close(db);
}

#if 0
void PrintTasks(const TaskPtrVec &tasks, int indent = 0) {
    for (const auto &t : tasks) {
        for (int i = 0; i < indent; i++)
            std::cout << "--";
        std::cout << t->id << "," << t->parentId << "," << t->srcSite << ","
                  << t->srcPath << "->" << t->dstSite << "," << t->dstPath
                  << std::endl;
        PrintTasks(t->children, indent + 1);
    }
}
#endif

OssSiteNodePtrVec Storage::Impl::LoadSites() {
    OssSiteNodePtrVec v;
    int rc;
    do {
        rc = sqlite3_step(TableSite.select);
        if (rc == SQLITE_ROW) {
            OssSiteNodePtr ossSiteNode(new OssSiteNode);
            ossSiteNode->id =
                    GetColumnInt64(TableSite.select, TableSiteColumns::id, 0);
            ossSiteNode->name =
                    GetColumnString(TableSite.select, TableSiteColumns::name);
            ossSiteNode->region =
                    GetColumnString(TableSite.select, TableSiteColumns::region);
            ossSiteNode->keyId =
                    GetColumnString(TableSite.select, TableSiteColumns::keyId);
            ossSiteNode->keySecret = GetColumnString(
                    TableSite.select, TableSiteColumns::keySecret);
            ossSiteNode->lastLocalPath = GetColumnString(
                    TableSite.select, TableSiteColumns::lastLocalPath);
            ossSiteNode->lastOssPath = GetColumnString(
                    TableSite.select, TableSiteColumns::lastOssPath);
            v.push_back(ossSiteNode);
        }
    } while (rc == SQLITE_ROW || rc == SQLITE_BUSY);

    return v;
}

void Storage::Impl::AppendSite(OssSiteNodePtr &site) {
    Bind(TableSite.insert, TableSiteColumns::name, site->name);
    Bind(TableSite.insert, TableSiteColumns::region, site->region);
    Bind(TableSite.insert, TableSiteColumns::keyId, site->keyId);
    Bind(TableSite.insert, TableSiteColumns::keySecret, site->keySecret);
    Bind(TableSite.insert,
         TableSiteColumns::lastLocalPath,
         site->lastLocalPath);
    Bind(TableSite.insert, TableSiteColumns::lastOssPath, site->lastOssPath);

    int rc;
    do {
        rc = sqlite3_step(TableSite.insert);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSite.insert);

    if (rc == SQLITE_DONE) {
        site->id = sqlite3_last_insert_rowid(db);
    } else {
        std::cout << "what the hell:" << sqlite3_errstr(rc) << std::endl;
    }
}

void Storage::Impl::UpdateSite(const OssSiteNodePtr &site) {
    Bind(TableSite.update, TableSiteColumns::name, site->name);
    Bind(TableSite.update, TableSiteColumns::region, site->region);
    Bind(TableSite.update, TableSiteColumns::keyId, site->keyId);
    Bind(TableSite.update, TableSiteColumns::keySecret, site->keySecret);
    Bind(TableSite.update,
         TableSiteColumns::lastLocalPath,
         site->lastLocalPath);
    Bind(TableSite.update, TableSiteColumns::lastOssPath, site->lastOssPath);
    Bind(TableSite.update, TableSiteColumns::lastId, (int64_t)site->id);

    int rc;
    do {
        rc = sqlite3_step(TableSite.update);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSite.update);
}

void Storage::Impl::RemoveSite(const OssSiteNodePtr &site) {
    Bind(TableSite.remove, 1, (int64_t)site->id);

    int rc;
    do {
        rc = sqlite3_step(TableSite.remove);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSite.remove);
}

SchedulePtrVec Storage::Impl::LoadSchedules() {
    SchedulePtrVec v;
    int rc;
    do {
        rc = sqlite3_step(TableSchedule.select);
        if (rc == SQLITE_ROW) {
            SchedulePtr schedule(new Schedule);
            schedule->id = GetColumnInt64(
                    TableSchedule.select, TableScheduleColumns::id, 0);
            schedule->name = GetColumnString(TableSchedule.select,
                                             TableScheduleColumns::name);
            schedule->srcSite = GetColumnString(TableSchedule.select,
                                                TableScheduleColumns::srcSite);
            schedule->srcPath = GetColumnString(TableSchedule.select,
                                                TableScheduleColumns::srcPath);
            schedule->dstSite = GetColumnString(TableSchedule.select,
                                                TableScheduleColumns::dstSite);
            schedule->dstPath = GetColumnString(TableSchedule.select,
                                                TableScheduleColumns::dstPath);
            schedule->flag = GetColumnInt(
                    TableSchedule.select, TableScheduleColumns::flag, 0);
            schedule->routine = (ScheduleRoutine)GetColumnInt64(
                    TableSchedule.select, TableScheduleColumns::routine, 0);
            schedule->lastStartTime =
                    GetColumnInt64(TableSchedule.select,
                                   TableScheduleColumns::lastStartTime,
                                   0);
            schedule->lastFinishTime =
                    GetColumnInt64(TableSchedule.select,
                                   TableScheduleColumns::lastFinishTime,
                                   0);
            v.push_back(schedule);
        }
    } while (rc == SQLITE_ROW || rc == SQLITE_BUSY);

    return v;
}

void Storage::Impl::AppendSchedule(const SchedulePtr &schedule) {
    Bind(TableSchedule.insert, TableScheduleColumns::name, schedule->name);
    Bind(TableSchedule.insert,
         TableScheduleColumns::srcSite,
         schedule->srcSite);
    Bind(TableSchedule.insert,
         TableScheduleColumns::srcPath,
         schedule->srcPath);
    Bind(TableSchedule.insert,
         TableScheduleColumns::dstSite,
         schedule->dstSite);
    Bind(TableSchedule.insert,
         TableScheduleColumns::dstPath,
         schedule->dstPath);
    Bind(TableSchedule.insert,
         TableScheduleColumns::flag,
         (int64_t)schedule->flag);
    Bind(TableSchedule.insert,
         TableScheduleColumns::routine,
         (int64_t)schedule->routine);
    Bind(TableSchedule.insert,
         TableScheduleColumns::lastStartTime,
         (int64_t)schedule->lastStartTime);
    Bind(TableSchedule.insert,
         TableScheduleColumns::lastFinishTime,
         (int64_t)schedule->lastFinishTime);

    int rc;
    do {
        rc = sqlite3_step(TableSchedule.insert);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSchedule.insert);

    if (rc == SQLITE_DONE) {
        schedule->id = sqlite3_last_insert_rowid(db);
    }
}

void Storage::Impl::UpdateSchedule(const SchedulePtr &schedule) {
    Bind(TableSchedule.update, TableScheduleColumns::name, schedule->name);
    Bind(TableSchedule.update,
         TableScheduleColumns::srcSite,
         schedule->srcSite);
    Bind(TableSchedule.update,
         TableScheduleColumns::srcPath,
         schedule->srcPath);
    Bind(TableSchedule.update,
         TableScheduleColumns::dstSite,
         schedule->dstSite);
    Bind(TableSchedule.update,
         TableScheduleColumns::dstPath,
         schedule->dstPath);
    Bind(TableSchedule.update,
         TableScheduleColumns::flag,
         (int64_t)schedule->flag);
    Bind(TableSchedule.update,
         TableScheduleColumns::routine,
         (int64_t)schedule->routine);
    Bind(TableSchedule.update,
         TableScheduleColumns::lastStartTime,
         (int64_t)schedule->lastStartTime);
    Bind(TableSchedule.update,
         TableScheduleColumns::lastFinishTime,
         (int64_t)schedule->lastFinishTime);
    Bind(TableSchedule.update,
         TableScheduleColumns::lastId,
         (int64_t)schedule->id);

    int rc;
    do {
        rc = sqlite3_step(TableSchedule.update);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSchedule.update);
}

void Storage::Impl::RemoveSchedule(const SchedulePtr &schedule) {
    Bind(TableSchedule.remove, 1, (int64_t)schedule->id);

    int rc;
    do {
        rc = sqlite3_step(TableSchedule.remove);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableSchedule.remove);
}

TaskPtrVec Storage::Impl::LoadTasks() {
    TaskPtrVec v;
    int rc;
    do {
        rc = sqlite3_step(TableTask.select);
        if (rc == SQLITE_ROW) {
            TaskPtr task(new Task);
            task->id =
                    GetColumnInt64(TableTask.select, TableTaskColumns::id, 0);
            task->parentId = GetColumnInt64(
                    TableTask.select, TableTaskColumns::parent, 0);
            task->scheduleId = GetColumnInt64(
                    TableTask.select, TableTaskColumns::schedule, 0);
            task->type = (TaskType)GetColumnInt(
                    TableTask.select, TableTaskColumns::type, 0);
            task->status = (TaskStatus)GetColumnInt(
                    TableTask.select, TableTaskColumns::status, 0);
            if (task->status == TSPending || task->status == TSStarted) {
                task->status = TSStopped;
            }
            task->srcSite = GetColumnString(TableTask.select,
                                            TableTaskColumns::srcSite);
            task->srcPath = GetColumnString(TableTask.select,
                                            TableTaskColumns::srcPath);
            task->dstSite = GetColumnString(TableTask.select,
                                            TableTaskColumns::dstSite);
            task->dstPath = GetColumnString(TableTask.select,
                                            TableTaskColumns::dstPath);
            task->fileStat.size =
                    GetColumnInt64(TableTask.select, TableTaskColumns::size, 0);
            task->fileStat.lastModifiedTime = GetColumnInt64(
                    TableTask.select, TableTaskColumns::lastModifiedTime, 0);
            task->flag =
                    GetColumnInt(TableTask.select, TableTaskColumns::flag, 0);
            task->uploadId = GetColumnString(TableTask.select,
                                             TableTaskColumns::uploadId);
            task->offset = GetColumnInt64(
                    TableTask.select, TableTaskColumns::offset, 0);
            task->partId = GetColumnInt64(
                    TableTask.select, TableTaskColumns::partId, 0);
            task->progress = GetColumnInt64(
                    TableTask.select, TableTaskColumns::progress, 0);
            task->startTime = GetColumnInt64(
                    TableTask.select, TableTaskColumns::startTime, 0);
            task->finishTime = GetColumnInt64(
                    TableTask.select, TableTaskColumns::finishTime, 0);
            v.push_back(task);
        }
    } while (rc == SQLITE_ROW || rc == SQLITE_BUSY);

    BuildTaskHierarchy(v);
    // PrintTasks(v);

    return v;
}

void Storage::Impl::BuildTaskHierarchy(TaskPtrVec &v) {
    std::map<long, const TaskPtr &> m;

    for (auto it = v.begin(); it != v.end();) {
        const TaskPtr &t = *it;
        if (t->parentId == 0) {
            m.emplace(t->id, t);
            it++;
        } else {
            auto pit = m.find(t->parentId);
            if (pit != m.end()) {
                const auto &parent = pit->second;
                parent->children.push_back(t);
                t->parent = parent;
            } else {
                RemoveTask(t);
            }
            it = v.erase(it);
        }
    }
}

void Storage::Impl::AppendTask(const TaskPtr &task) {
    Bind(TableTask.insert, TableTaskColumns::parent, (int64_t)task->parentId);
    Bind(TableTask.insert,
         TableTaskColumns::schedule,
         (int64_t)task->scheduleId);
    Bind(TableTask.insert, TableTaskColumns::type, task->type);
    Bind(TableTask.insert, TableTaskColumns::status, task->status);
    Bind(TableTask.insert, TableTaskColumns::srcSite, task->srcSite);
    Bind(TableTask.insert, TableTaskColumns::srcPath, task->srcPath);
    Bind(TableTask.insert, TableTaskColumns::dstSite, task->dstSite);
    Bind(TableTask.insert, TableTaskColumns::dstPath, task->dstPath);
    Bind(TableTask.insert,
         TableTaskColumns::size,
         (int64_t)task->fileStat.size);
    Bind(TableTask.insert,
         TableTaskColumns::lastModifiedTime,
         (int)task->fileStat.lastModifiedTime);
    Bind(TableTask.insert, TableTaskColumns::flag, task->flag);
    Bind(TableTask.insert, TableTaskColumns::uploadId, task->uploadId);
    Bind(TableTask.insert, TableTaskColumns::offset, (int64_t)task->offset);
    Bind(TableTask.insert, TableTaskColumns::partId, (int64_t)task->partId);
    Bind(TableTask.insert, TableTaskColumns::progress, (int64_t)task->progress);
    Bind(TableTask.insert,
         TableTaskColumns::startTime,
         (int64_t)task->startTime);
    Bind(TableTask.insert,
         TableTaskColumns::finishTime,
         (int64_t)task->finishTime);

    int rc;
    do {
        rc = sqlite3_step(TableTask.insert);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableTask.insert);

    if (rc == SQLITE_DONE) {
        task->id = sqlite3_last_insert_rowid(db);
    }
}

void Storage::Impl::UpdateTask(const TaskPtr &task) {
    Bind(TableTask.update, TableTaskColumns::parent, (int64_t)task->parentId);
    Bind(TableTask.update,
         TableTaskColumns::schedule,
         (int64_t)task->scheduleId);
    Bind(TableTask.update, TableTaskColumns::type, task->type);
    Bind(TableTask.update, TableTaskColumns::status, task->status);
    Bind(TableTask.update, TableTaskColumns::srcSite, task->srcSite);
    Bind(TableTask.update, TableTaskColumns::srcPath, task->srcPath);
    Bind(TableTask.update, TableTaskColumns::dstSite, task->dstSite);
    Bind(TableTask.update, TableTaskColumns::dstPath, task->dstPath);
    Bind(TableTask.update,
         TableTaskColumns::size,
         (int64_t)task->fileStat.size);
    Bind(TableTask.update,
         TableTaskColumns::lastModifiedTime,
         (int)task->fileStat.lastModifiedTime);
    Bind(TableTask.update, TableTaskColumns::flag, task->flag);
    Bind(TableTask.update, TableTaskColumns::uploadId, task->uploadId);
    Bind(TableTask.update, TableTaskColumns::offset, (int64_t)task->offset);
    Bind(TableTask.update, TableTaskColumns::partId, (int64_t)task->partId);
    Bind(TableTask.update, TableTaskColumns::progress, (int64_t)task->progress);
    Bind(TableTask.update,
         TableTaskColumns::startTime,
         (int64_t)task->startTime);
    Bind(TableTask.update,
         TableTaskColumns::finishTime,
         (int64_t)task->finishTime);
    Bind(TableTask.update, TableTaskColumns::lastId, (int64_t)task->id);

    int rc;
    do {
        rc = sqlite3_step(TableTask.update);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableTask.update);
}

void Storage::Impl::RemoveTask(const TaskPtr &task) {
    for (const auto &t : task->children) {
        RemoveTask(t);
    }
    Bind(TableTask.remove, 1, (int64_t)task->id);

    int rc;
    do {
        rc = sqlite3_step(TableTask.remove);
    } while (rc == SQLITE_BUSY);

    sqlite3_reset(TableTask.remove);
}

void Storage::Impl::CreateTables() {
    for (auto *table : schemas) {
        std::ostringstream ss;
        ss << "CREATE TABLE IF NOT EXISTS ";
        ss << table->name << " ";
        ss << "(";
        for (size_t i = 0; i < table->columns.size(); i++) {
            if (i) {
                ss << ", ";
            }
            const auto &column = table->columns[i];
            CreateColumnDef(ss, column);
        }
        ss << ")";
        std::string query = ss.str();
        if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK) {
            throw "sqlite3 exec";
        }
        PrepareSelectStatement(table);
        PrepareInsertStatement(table);
        PrepareUpdateStatement(table);
        PrepareDeleteStatement(table);
    }
}

void Storage::Impl::CreateColumnDef(std::ostringstream &ss,
                                    const Column &column) {
    ss << column.name;
    if (column.type == CTInteger) {
        ss << " INTEGER";
    } else {
        ss << " TEXT";
    }
    if (column.flags & PrimaryKey) {
        ss << " PRIMARY KEY AUTOINCREMENT";
    }
    if (column.flags & NotNull) {
        ss << " NOT NULL";
    }
    if (column.flags & DefaultNull) {
        ss << " DEFAULT NULL";
    }
}

void Storage::Impl::PrepareSelectStatement(Table *table) {
    std::ostringstream ss;
    ss << "SELECT ";
    for (size_t i = 0; i < table->columns.size(); i++) {
        if (i) {
            ss << ", ";
        }
        ss << table->columns[i].name;
    }
    ss << " FROM " << table->name;
    std::string query = ss.str();
    table->select = PrepareStatement(query);
}

void Storage::Impl::PrepareInsertStatement(Table *table) {
    std::ostringstream ss;
    ss << "INSERT INTO " << table->name << " (";
    for (size_t i = 1; i < table->columns.size(); i++) {
        if (i > 1) {
            ss << ", ";
        }
        ss << table->columns[i].name;
    }
    ss << ") VALUES (";
    for (size_t i = 1; i < table->columns.size(); i++) {
        if (i > 1) {
            ss << ", ";
        }
        ss << ":" << table->columns[i].name;
    }
    ss << ")";
    std::string query = ss.str();
    table->insert = PrepareStatement(query);
}

void Storage::Impl::PrepareUpdateStatement(Table *table) {
    std::ostringstream ss;
    ss << "UPDATE " << table->name << " SET ";
    for (size_t i = 1; i < table->columns.size(); i++) {
        if (i > 1) {
            ss << ", ";
        }
        ss << table->columns[i].name << " = :" << table->columns[i].name;
    }
    ss << " WHERE " << table->columns[0].name
       << " = :" << table->columns[0].name;
    std::string query = ss.str();
    table->update = PrepareStatement(query);
}

void Storage::Impl::PrepareDeleteStatement(Table *table) {
    std::ostringstream ss;
    ss << "DELETE FROM " << table->name << " WHERE " << table->columns[0].name
       << " = :" << table->columns[0].name;
    std::string query = ss.str();
    table->remove = PrepareStatement(query);
}

sqlite3_stmt *Storage::Impl::PrepareStatement(const std::string &query) {
    sqlite3_stmt *stmt{nullptr};

    int rc;
    do {
        rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, 0);
    } while (rc == SQLITE_BUSY);

    if (rc != SQLITE_OK) {
        std::cout << "eee: " << query << std::endl;
        throw "sqlite3 prepare";
    }

    return stmt;
}

bool Storage::Impl::Bind(sqlite3_stmt *stmt, int index, int value) {
    return sqlite3_bind_int(stmt, index, value) == SQLITE_OK;
}

bool Storage::Impl::Bind(sqlite3_stmt *stmt, int index, int64_t value) {
    return sqlite3_bind_int64(stmt, index, value) == SQLITE_OK;
}

bool Storage::Impl::Bind(sqlite3_stmt *stmt,
                         int index,
                         const std::string &value) {
    return sqlite3_bind_text(stmt,
                             index,
                             value.c_str(),
                             value.size(),
                             SQLITE_TRANSIENT) == SQLITE_OK;
}

int Storage::Impl::GetColumnInt(sqlite3_stmt *stmt, int index, int def) {
    if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
        return def;
    } else {
        return sqlite3_column_int(stmt, index);
    }
}

int64_t Storage::Impl::GetColumnInt64(sqlite3_stmt *stmt,
                                      int index,
                                      int64_t def) {
    if (sqlite3_column_type(stmt, index) == SQLITE_NULL) {
        return def;
    } else {
        return sqlite3_column_int64(stmt, index);
    }
}

std::string Storage::Impl::GetColumnString(sqlite3_stmt *stmt, int index) {
    const char *s = (const char *)sqlite3_column_text(stmt, index);
    if (s) {
        int len = sqlite3_column_bytes(stmt, index);
        return std::string(s, len);
    }

    return std::string();
}

bool Storage::Impl::BeginTransaction() {
    return sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0) == SQLITE_OK;
}

bool Storage::Impl::EndTransaction(bool rollback) {
    if (rollback) {
        return sqlite3_exec(db, "ROLLBACK", 0, 0, 0) == SQLITE_OK;
    } else {
        return sqlite3_exec(db, "END TRANSACTION", 0, 0, 0) == SQLITE_OK;
    }
}

Storage::Storage() {
    impl.reset(new Impl);
}

Storage::~Storage() {
}

OssSiteNodePtrVec Storage::LoadSites() {
    return impl->LoadSites();
}

void Storage::AppendSite(OssSiteNodePtr &site) {
    impl->AppendSite(site);
}

void Storage::UpdateSite(const OssSiteNodePtr &site) {
    impl->UpdateSite(site);
}

void Storage::RemoveSite(const OssSiteNodePtr &site) {
    impl->RemoveSite(site);
}

SchedulePtrVec Storage::LoadSchedules() {
    return impl->LoadSchedules();
}

void Storage::AppendSchedule(const SchedulePtr &schedule) {
    impl->AppendSchedule(schedule);
}

void Storage::UpdateSchedule(const SchedulePtr &schedule) {
    impl->UpdateSchedule(schedule);
}

void Storage::RemoveSchedule(const SchedulePtr &schedule) {
    impl->RemoveSchedule(schedule);
}

TaskPtrVec Storage::LoadTasks() {
    return impl->LoadTasks();
}

void Storage::AppendTask(const TaskPtr &task) {
    impl->AppendTask(task);
}

void Storage::UpdateTask(const TaskPtr &task) {
    impl->UpdateTask(task);
}

void Storage::RemoveTask(const TaskPtr &task) {
    impl->RemoveTask(task);
}

Storage *storage() {
    static std::unique_ptr<Storage> inst(new Storage);
    return inst.get();
}
