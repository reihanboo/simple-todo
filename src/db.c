#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int exec_nonquery(sqlite3 *db, const char *sql, int (*bind)(sqlite3_stmt*, void*), void *bind_arg) {
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    if (bind) {
        rc = bind(stmt, bind_arg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to bind parameters: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return rc;
        }
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    return rc;
}

static void exec_query(sqlite3 *db, const char *sql, int (*row_handler)(sqlite3_stmt*, void*), void *row_arg) {
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (row_handler) {
            row_handler(stmt, row_arg);
        }
    }
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error while fetching data: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

static int bind_desc(sqlite3_stmt* stmt, void *arg) {
    const char *desc = (const char*) arg;
    return sqlite3_bind_text(stmt, 1, desc, -1, SQLITE_STATIC);
}

static int bind_id(sqlite3_stmt* stmt, void *arg) {
    int id = *(int*)arg;
    return sqlite3_bind_int(stmt, 1, id);
}

static int print_task_row(sqlite3_stmt* stmt, void *arg) {
    (void)arg;
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char *desc = sqlite3_column_text(stmt, 1);
    int is_done = sqlite3_column_int(stmt, 2);
    printf("[%d] %s %s\n", id, desc, is_done ? "(done)" : "");
    return SQLITE_OK;
}

int open_database(sqlite3 **db, const char *db_file) {
    return sqlite3_open(db_file, db);
}

void close_database(sqlite3 *db) {
    sqlite3_close(db);
}

void create_table(sqlite3 *db) {
    const char *sql = "CREATE TABLE IF NOT EXISTS tasks ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "description TEXT NOT NULL, "
                      "is_done INTEGER DEFAULT 0"
                      ");";
    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (create table): %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }
}

void add_task(sqlite3 *db, const char *desc) {
    const char *sql = "INSERT INTO tasks (description) VALUES (?);";
    int rc = exec_nonquery(db, sql, bind_desc, (void*)desc);
    if (rc == SQLITE_DONE) {
        printf("Task added: %s\n", desc);
    }
}

void list_tasks(sqlite3 *db) {
    printf("Todo List:\n");
    const char *sql = "SELECT id, description, is_done FROM tasks WHERE is_done = 0;";
    exec_query(db, sql, print_task_row, NULL);
}

void mark_done(sqlite3 *db, int id) {
    const char *sql = "UPDATE tasks SET is_done = 1 WHERE id = ?;";
    int rc = exec_nonquery(db, sql, bind_id, &id);
    if (rc == SQLITE_DONE) {
         printf("Task %d marked as done.\n", id);
    }
}

void tasks_archive(sqlite3 *db) {
    printf("Archive of All Todos:\n");
    const char *sql = "SELECT id, description, is_done FROM tasks;";
    exec_query(db, sql, print_task_row, NULL);
}
