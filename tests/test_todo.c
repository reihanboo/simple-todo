#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "db.h"

int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open(":memory:", &db);
    assert(rc == SQLITE_OK);

    create_table(db);
    add_task(db, "Test task 1");
    add_task(db, "Test task 2");

    sqlite3_stmt *stmt;
    const char *sql_count = "SELECT COUNT(*) FROM tasks;";
    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, 0);
    assert(rc == SQLITE_OK);
    rc = sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    assert(count == 2);
    sqlite3_finalize(stmt);

    mark_done(db, 1);
    const char *sql_done = "SELECT is_done FROM tasks WHERE id = 1;";
    rc = sqlite3_prepare_v2(db, sql_done, -1, &stmt, 0);
    assert(rc == SQLITE_OK);
    rc = sqlite3_step(stmt);
    int is_done = sqlite3_column_int(stmt, 0);
    assert(is_done == 1);
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    printf("All tests passed.\n");
    return 0;
}
