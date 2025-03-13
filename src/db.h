#ifndef DB_H
#define DB_H

#include <sqlite3.h>

int open_database(sqlite3 **db, const char *db_file);
void close_database(sqlite3 *db);
void create_table(sqlite3 *db);
void add_task(sqlite3 *db, const char *desc);
void list_tasks(sqlite3 *db);
void mark_done(sqlite3 *db, int id);

#endif