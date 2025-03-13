#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"

#define DB_FILE "todo.db"

static void print_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s add \"task description\"\n", progname);
    printf("  %s list\n", progname);
    printf("  %s done task_id\n", progname);
    printf("  %s archive\n", progname);
}

static void process_command(sqlite3 *db, int argc, char *argv[]) {
    if (strcmp(argv[1], "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a task description.\n");
        } else {
            add_task(db, argv[2]);
        }
    } else if (strcmp(argv[1], "list") == 0) {
        list_tasks(db);
    } else if (strcmp(argv[1], "done") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Please provide a task id to mark as done.\n");
        } else {
            int id = atoi(argv[2]);
            mark_done(db, id);
        }
    } else if (strcmp(argv[1], "archive") == 0) {
        tasks_archive(db);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
    }
}

int main(int argc, char *argv[]) {
    sqlite3 *db;

    if (open_database(&db, DB_FILE) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database.\n");
        return EXIT_FAILURE;
    }

    create_table(db);

    if (argc < 2) {
        print_usage(argv[0]);
        close_database(db);
        return EXIT_SUCCESS;
    }

    process_command(db, argc, argv);

    close_database(db);
    return EXIT_SUCCESS;
}
