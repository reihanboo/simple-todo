#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/sqlite3.h"

#define DB_FILE "todo.db"


void create_table(sqlite3 * db) {
  char * err_msg = 0;
  const char * sql = "CREATE TABLE IF NOT EXISTS tasks ("
  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
  "description TEXT NOT NULL, "
  "is_done INTEGER DEFAULT 0"
  ");";
  int rc = sqlite3_exec(db, sql, 0, 0, & err_msg);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error (create table): %s\n", err_msg);
    sqlite3_free(err_msg);
    exit(1);
  }
}


void add_task(sqlite3 * db, const char * desc) {
  sqlite3_stmt * stmt;

  const char * sql = "INSERT INTO tasks (description) VALUES (?);";

  int rc = sqlite3_prepare_v2(db, sql, -1, & stmt, 0);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement (add): %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_text(stmt, 1, desc, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Execution failed (add): %s\n", sqlite3_errmsg(db));
  } else {
    printf("Task added: %s\n", desc);
  }
  sqlite3_finalize(stmt);
}


void list_tasks(sqlite3 * db) {
  sqlite3_stmt *stmt;

  const char *sql = "SELECT id, description, is_done FROM tasks WHERE is_done = 0;";

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement (list): %s\n", sqlite3_errmsg(db));
    return;
  }

  printf("Todo List:\n");

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char *desc = sqlite3_column_text(stmt, 1);
    int is_done = sqlite3_column_int(stmt, 2);
    printf("[%d] %s %s\n", id, desc, is_done ? "(done)" : "");
  }

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Error while fetching data: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}


void tasks_archive(sqlite3 * db) {
  sqlite3_stmt * stmt;

  const char * sql = "SELECT id, description, is_done FROM tasks;";
  int rc = sqlite3_prepare_v2(db, sql, -1, & stmt, 0);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement (list): %s\n", sqlite3_errmsg(db));
    return;
  }

  printf("Todo List:\n");

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char * desc = sqlite3_column_text(stmt, 1);
    int is_done = sqlite3_column_int(stmt, 2);
    printf("[%d] %s %s\n", id, desc, is_done ? "(done)" : "");
  }

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Error while fetching data: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}


void mark_done(sqlite3 * db, int id) {
  sqlite3_stmt * stmt;

  const char * sql = "UPDATE tasks SET is_done = 1 WHERE id = ?;";

  int rc = sqlite3_prepare_v2(db, sql, -1, & stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement (update): %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Execution failed (update): %s\n", sqlite3_errmsg(db));
  } else {
    printf("Task %d marked as done.\n", id);
  }

  sqlite3_finalize(stmt);
}


int main(int argc, char * argv[]) {
  sqlite3 * db;

  int rc = sqlite3_open(DB_FILE, & db);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    return 1;
  }

  create_table(db);

  if (argc < 2) {
    printf("Usage:\n");
    printf("  %s add \"task description\"\n", argv[0]);
    printf("  %s list\n", argv[0]);
    printf("  %s done task_id\n", argv[0]);
    printf("  %s archive\n", argv[0]);
    sqlite3_close(db);
    return 0;
  }

  if (strcmp(argv[1], "add") == 0) {

    if (argc < 3) {
      fprintf(stderr, "Please provide a task description.\n");
    } else {
      add_task(db, argv[2]);
    }

  } else if (strcmp(argv[1], "list") == 0) {

    list_tasks(db);

  } else if (strcmp(argv[1], "archive") == 0) {

    tasks_archive(db);

  } else if (strcmp(argv[1], "done") == 0) {

    if (argc < 3) {
      fprintf(stderr, "Please provide a task id to mark as done.\n");
    } else {
      int id = atoi(argv[2]);
      mark_done(db, id);
    }

  } else {
    fprintf(stderr, "Unknown command: %s\n", argv[1]);
  }

  sqlite3_close(db);
  return 0;
}