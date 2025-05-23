#include <curses.h> 
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__unix__) || defined(__unix)
#include <sys/wait.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (status)
#endif

#define MAX_LINES 1024
#define MAX_LEN 256

typedef enum
{
    MODE_LIST,
    MODE_ARCHIVE
} Mode;

char *lines[MAX_LINES];
int line_count = 0;
int selected = 0;
int scroll_offset = 0;
Mode mode = MODE_LIST;
char error_message[MAX_LEN] = "";

#define CP_DEFAULT 1
#define CP_TITLE 2
#define CP_SELECTED 3
#define CP_ERROR 4
#define CP_HELP 5

static int has_term_colors = 0; 

static attr_t attr_default_text;
static attr_t attr_title_text;
static attr_t attr_selected_item_text;
static attr_t attr_error_text;
static attr_t attr_help_text;

int run_command(const char *cmd, char *output, size_t out_size)
{
    FILE *fp = popen(cmd, "r");
    if (!fp)
    {
        if (out_size > 0)
            output[0] = '\0';
        return -1;
    }

    if (out_size > 0)
        output[0] = '\0';

    size_t current_len = 0;
    char buffer[MAX_LEN];
    while (fgets(buffer, sizeof(buffer), fp))
    {
        size_t line_len = strlen(buffer);
        if (current_len + line_len < out_size)
        {
            strcat(output, buffer);
            current_len += line_len;
        }
        else
        {
            if (out_size > current_len + 1)
            {
                strncat(output, buffer, out_size - current_len - 1);
            }
            break;
        }
    }
    int status = pclose(fp);
    return WEXITSTATUS(status);
}

void load_items()
{
    error_message[0] = '\0';

    char cmd[MAX_LEN];
    snprintf(cmd, sizeof(cmd), "./td %s 2>&1",
             mode == MODE_LIST ? "list" : "archive");

    int old_selected_id = -1;
    if (selected >= 0 && selected < line_count && lines[selected] != NULL)
    {
        sscanf(lines[selected], "[%d]", &old_selected_id);
    }

    FILE *fp_cmd_output = popen(cmd, "r");
    if (!fp_cmd_output)
    {
        snprintf(error_message, sizeof(error_message), "Failed to execute: %s", cmd);
        return;
    }

    for (int i = 0; i < line_count; ++i)
    {
        free(lines[i]);
        lines[i] = NULL;
    }
    line_count = 0;

    char buf[MAX_LEN];
    while (fgets(buf, sizeof(buf), fp_cmd_output) && line_count < MAX_LINES)
    {
        buf[strcspn(buf, "\n\r")] = '\0';
        lines[line_count] = strdup(buf);
        if (!lines[line_count])
        {
            break;
        }
        line_count++;
    }
    int cmd_status = pclose(fp_cmd_output);

    if (WEXITSTATUS(cmd_status) != 0)
    {
        if (line_count > 0 && (strstr(lines[0], "Error") || strstr(lines[0], "error")))
        {
            snprintf(error_message, sizeof(error_message), "td: %s", lines[0]);
        }
        else if (line_count > 0 && strlen(lines[0]) < MAX_LEN / 2 && strlen(lines[0]) > 0)
        {
            snprintf(error_message, sizeof(error_message), "td msg: %s", lines[0]);
        }
        else
        {
            snprintf(error_message, sizeof(error_message), "Error listing (code: %d)", WEXITSTATUS(cmd_status));
        }
        for (int i = 0; i < line_count; ++i)
        {
            free(lines[i]);
            lines[i] = NULL;
        }
        line_count = 0;
    }

    if (old_selected_id != -1 && line_count > 0)
    {
        int new_selected_idx = -1;
        for (int i = 0; i < line_count; ++i)
        {
            int current_id;

            if (sscanf(lines[i], "[%d]", &current_id) == 1 && current_id == old_selected_id)
            {
                new_selected_idx = i;
                break;
            }
        }
        selected = (new_selected_idx != -1) ? new_selected_idx : (line_count > 0 ? 0 : 0);
    }
    else
    {
        if (selected >= line_count)
            selected = line_count > 0 ? line_count - 1 : 0;
        if (selected < 0 && line_count > 0)
            selected = 0;
        else if (line_count == 0)
            selected = 0;
    }

    int h_scr, w_scr;
    getmaxyx(stdscr, h_scr, w_scr);
    int container_h = h_scr - 5;
    if (line_count == 0)
    {
        scroll_offset = 0;
    }
    else
    {
        if (scroll_offset >= line_count && line_count > 0)
        {
            scroll_offset = line_count - 1;
        }
        if (container_h <= 0)
            container_h = 1; // avoid division by zero or negative height

        if (selected < scroll_offset)
        {
            scroll_offset = selected;
        }
        else if (selected >= scroll_offset + container_h)
        {
            scroll_offset = selected - container_h + 1;
        }
        if (scroll_offset < 0)
            scroll_offset = 0;
    }
}

void draw_ui()
{
    erase();
    int h, w;
    getmaxyx(stdscr, h, w);

    const char *title_str = mode == MODE_LIST ? "Todos" : "Archive";

    attrset(attr_title_text);
    mvprintw(0, (w - (int)strlen(title_str)) / 2, "%s", title_str);

    int list_display_start_y = 1; 
    int list_display_end_y = h - 3; 
    int container_height = list_display_end_y - list_display_start_y + 1;
    if (container_height <= 0)
        container_height = 1;

    if (line_count > 0)
    {
        if (selected < scroll_offset)
        {
            scroll_offset = selected;
        }
        else if (selected >= scroll_offset + container_height)
        {
            scroll_offset = selected - container_height + 1;
        }

        if (scroll_offset > line_count - container_height && line_count > container_height)
        {
            scroll_offset = line_count - container_height;
        }
        if (scroll_offset < 0)
            scroll_offset = 0;
    }
    else
    {
        scroll_offset = 0;
    }

    int start_idx = scroll_offset;
    int end_idx = scroll_offset + container_height;
    if (end_idx > line_count)
        end_idx = line_count;

    for (int i = start_idx; i < end_idx; ++i)
    {
        int screen_row = list_display_start_y + (i - start_idx);
        if (screen_row >= list_display_start_y && screen_row <= list_display_end_y)
        {
            if (i == selected)
            {
                attrset(attr_selected_item_text);
            }
            else
            {
                attrset(attr_default_text);
            }
            char display_buf[MAX_LEN + 1];
            snprintf(display_buf, sizeof(display_buf), "%-*s", w - 2, lines[i] ? lines[i] : "");
            mvprintw(screen_row, 1, "%s", display_buf);
        }
    }

    if (error_message[0])
    {
        attrset(attr_error_text);
        mvhline(h - 2, 0, ' ', w);
        mvprintw(h - 2, 1, "Error: %s", error_message);
    }
    else
    {
        attrset(attr_default_text); 
        mvhline(h - 2, 0, ' ', w);
    }

    attrset(attr_help_text);
    mvhline(h - 1, 0, ' ', w);
    mvprintw(h - 1, 1, "[a]Add [d]Done [m]Mode [g/G]Top/Bottom [PgUp/PgDn]Scroll [q]Quit");

    attrset(attr_default_text);
    refresh();
}

void add_item()
{
    error_message[0] = '\0';
    int h_main, w_main;
    getmaxyx(stdscr, h_main, w_main);

    int input_win_h = 3;
    int input_win_w = w_main > 60 ? w_main / 2 : (w_main > 20 ? w_main - 10 : (w_main > 5 ? w_main - 2 : 3));
    if (input_win_w < 10)
        input_win_w = (w_main > 2 ? w_main - 2 : 1); 
    int input_win_y = (h_main - input_win_h) / 2;
    int input_win_x = (w_main - input_win_w) / 2;

    WINDOW *win = newwin(input_win_h, input_win_w, input_win_y, input_win_x);
    keypad(win, TRUE);

    if (has_term_colors)
    {
        wbkgd(win, COLOR_PAIR(CP_SELECTED));
    }
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "New: ");
    wrefresh(win);

    echo();
    curs_set(1);
    char buf[MAX_LEN - 20];
    memset(buf, 0, sizeof(buf));
    if (input_win_w > 8)
    {
        mvwgetnstr(win, 1, 7, buf, sizeof(buf) - 1 < (size_t)(input_win_w - 8) ? sizeof(buf) - 1 : (input_win_w - 8));
    }
    noecho();
    curs_set(0);
    delwin(win);

    if (strlen(buf) > 0)
    {
        char cmd[MAX_LEN * 2];
        char escaped_buf[MAX_LEN * 2] = "";
        int k = 0;
        for (size_t i = 0; buf[i] != '\0' && k < sizeof(escaped_buf) - 1; ++i)
        {
            if (buf[i] == '"' || buf[i] == '\\' || buf[i] == '`' || buf[i] == '$' || buf[i] == '!')
            {
                if (k < sizeof(escaped_buf) - 2)
                {
                    escaped_buf[k++] = '\\';
                }
                else
                    break;
            }
            escaped_buf[k++] = buf[i];
        }
        escaped_buf[k] = '\0';

        snprintf(cmd, sizeof(cmd), "./td add \"%s\" 2>&1", escaped_buf);
        char output[MAX_LEN] = "";
        int code = run_command(cmd, output, sizeof(output));
        if (code != 0)
        {
            output[strcspn(output, "\n\r")] = '\0';
            snprintf(error_message, sizeof(error_message), "Add failed: %s", output);
        }
        load_items();
    }
}

int get_current_id()
{
    if (line_count == 0 || selected < 0 || selected >= line_count || lines[selected] == NULL)
    {
        return -1;
    }
    int id = -1;
    if (sscanf(lines[selected], "[%d]", &id) == 1 && id > 0)
    {
        return id;
    }
    return -1;
}

void mark_done()
{
    error_message[0] = '\0';
    if (line_count == 0)
    {
        snprintf(error_message, sizeof(error_message), "No tasks to mark.");
        return;
    }

    int id = get_current_id();
    if (id > 0)
    {
        char cmd[MAX_LEN];
        snprintf(cmd, sizeof(cmd), "./td done %d 2>&1", id);
        char output[MAX_LEN] = "";
        int code = run_command(cmd, output, sizeof(output));
        if (code != 0)
        {
            output[strcspn(output, "\n\r")] = '\0';
            snprintf(error_message, sizeof(error_message), "Done failed: %s", output);
        }
        load_items();
        int old_idx = selected;
        if (line_count > 0)
        {
            selected = (old_idx < line_count ? old_idx : line_count - 1);
        }
    }
    else
    {
        snprintf(error_message, sizeof(error_message), "Invalid task: %s", lines[selected] ? lines[selected] : "NULL");
    }
}

int main()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors())
    {
        start_color();
        use_default_colors();

        init_pair(CP_DEFAULT, -1, -1);
        init_pair(CP_TITLE, COLOR_CYAN, -1);
        init_pair(CP_SELECTED, COLOR_BLACK, COLOR_WHITE);
        init_pair(CP_ERROR, COLOR_RED, -1);
        init_pair(CP_HELP, COLOR_GREEN, -1);

        has_term_colors = 1;
    }

    if (has_term_colors)
    {
        attr_default_text = COLOR_PAIR(CP_DEFAULT);
        attr_title_text = COLOR_PAIR(CP_TITLE) | A_BOLD;
        attr_selected_item_text = COLOR_PAIR(CP_SELECTED);
        attr_error_text = COLOR_PAIR(CP_ERROR) | A_BOLD;
        attr_help_text = COLOR_PAIR(CP_HELP) | A_BOLD;
        bkgd(COLOR_PAIR(CP_DEFAULT));
    }
    else
    {
        attr_default_text = A_NORMAL;
        attr_title_text = A_BOLD;
        attr_selected_item_text = A_REVERSE;
        attr_error_text = A_STANDOUT | A_BOLD;
        attr_help_text = A_BOLD;
    }

    load_items();
    draw_ui();

    int ch;
    while ((ch = getch()) != 'q')
    {
        error_message[0] = '\0';
        int old_selected = selected;

        switch (ch)
        {
        case KEY_UP:
        case 'k':
            if (line_count > 0)
            {
                selected = (selected > 0) ? selected - 1 : line_count - 1;
            }
            break;
        case KEY_DOWN:
        case 'j':
            if (line_count > 0)
            {
                selected = (selected < line_count - 1) ? selected + 1 : 0;
            }
            break;
        case KEY_PPAGE:
        {
            int view_height = (getmaxy(stdscr) - 3) - 1;
            if (view_height < 1)
                view_height = 1;
            selected -= view_height;
            if (selected < 0)
                selected = 0;
        }
        break;
        case KEY_NPAGE:
        {
            int view_height = (getmaxy(stdscr) - 3) - 1;
            if (view_height < 1)
                view_height = 1;
            selected += view_height;
            if (selected >= line_count)
                selected = line_count > 0 ? line_count - 1 : 0;
        }
        break;
        case 'g':
            if (line_count > 0)
                selected = 0;
            break;
        case 'G':
            if (line_count > 0)
                selected = line_count - 1;
            break;
        case 'a':
            add_item();
            break;
        case 'd':
            if (mode == MODE_LIST)
            {
                mark_done();
            }
            else
            {
                snprintf(error_message, sizeof(error_message), "'Done' not in Archive mode.");
            }
            selected = old_selected;
            break;
        case 'm':
            mode = (mode == MODE_LIST) ? MODE_ARCHIVE : MODE_LIST;
            selected = 0;
            scroll_offset = 0;
            load_items();
            break;
        default:
            break;
        }
        draw_ui();
    }

    endwin();
    for (int i = 0; i < line_count; ++i)
    {
        if (lines[i])
            free(lines[i]);
    }
    return 0;
}