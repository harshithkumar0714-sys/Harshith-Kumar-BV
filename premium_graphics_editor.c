#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define ROWS 25
#define COLS 60
#define EMPTY '_'
#define DRAW  '*'
#define MAX_OBJECTS 100
#define INPUT_BUFFER 128

typedef enum {
    OBJ_NONE = 0,
    OBJ_CIRCLE,
    OBJ_RECTANGLE,
    OBJ_LINE,
    OBJ_TRIANGLE,
} ObjectType;

static const char *ObjectTypeName[] = {
    "None",
    "Circle",
    "Rectangle",
    "Line",
    "Triangle",
};

typedef struct {
    int x, y, p1, p2, p3, p4;
} ObjectParams;

typedef struct {
    int id;
    ObjectType type;
    bool active;
    ObjectParams params;
} Object;

static char canvas[ROWS][COLS];
static Object objects[MAX_OBJECTS];
static int next_id = 1;

static void canvas_clear(void) {
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            canvas[r][c] = EMPTY;
}

static void canvas_display(void) {
    printf("\n   ");
    for (int c = 0; c < COLS; ++c)
        putchar('0' + (c % 10));
    putchar('\n');

    for (int r = 0; r < ROWS; ++r) {
        printf("%2d ", r);
        for (int c = 0; c < COLS; ++c)
            putchar(canvas[r][c]);
        putchar('\n');
    }
    putchar('\n');
}

static void plot(int r, int c) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = DRAW;
}

static void draw_line_raw(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        plot(y0, x0);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void draw_circle(int cx, int cy, int radius) {
    if (radius < 1) return;
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    while (x <= y) {
        plot(cy + y, cx + x);
        plot(cy - y, cx + x);
        plot(cy + y, cx - x);
        plot(cy - y, cx - x);
        plot(cy + x, cx + y);
        plot(cy - x, cx + y);
        plot(cy + x, cx - y);
        plot(cy - x, cx - y);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y -= 1;
        }
        x += 1;
    }
}

static void draw_rectangle(int x, int y, int w, int h) {
    if (w < 1 || h < 1) return;
    for (int col = x; col < x + w; ++col) {
        plot(y, col);
        plot(y + h - 1, col);
    }
    for (int row = y; row < y + h; ++row) {
        plot(row, x);
        plot(row, x + w - 1);
    }
}

static void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line_raw(x1, y1, x2, y2);
    draw_line_raw(x2, y2, x3, y3);
    draw_line_raw(x3, y3, x1, y1);
}

static void redraw_all(void) {
    canvas_clear();
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!objects[i].active) continue;
        Object *obj = &objects[i];
        switch (obj->type) {
            case OBJ_CIRCLE:
                draw_circle(obj->params.x, obj->params.y, obj->params.p1);
                break;
            case OBJ_RECTANGLE:
                draw_rectangle(obj->params.x, obj->params.y, obj->params.p1, obj->params.p2);
                break;
            case OBJ_LINE:
                draw_line_raw(obj->params.x, obj->params.y, obj->params.p1, obj->params.p2);
                break;
            case OBJ_TRIANGLE:
                draw_triangle(obj->params.x, obj->params.y,
                              obj->params.p1, obj->params.p2,
                              obj->params.p3, obj->params.p4);
                break;
            default:
                break;
        }
    }
}

static bool parse_ints(const char *line, int count, int *out) {
    char buffer[INPUT_BUFFER];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *token = strtok(buffer, " \t,;");
    int parsed = 0;
    while (token && parsed < count) {
        char *end;
        long value = strtol(token, &end, 10);
        if (*end != '\0')
            return false;
        out[parsed++] = (int)value;
        token = strtok(NULL, " \t,;");
    }
    return parsed == count;
}

static bool read_numbers(const char *prompt, int count, int *out) {
    char line[INPUT_BUFFER];
    printf("%s", prompt);
    if (!fgets(line, sizeof(line), stdin))
        return false;
    if (!parse_ints(line, count, out)) {
        printf("Invalid input. Please enter %d integer%s separated by spaces.\n", count, count == 1 ? "" : "s");
        return false;
    }
    return true;
}

static Object *find_object(int id) {
    for (int i = 0; i < MAX_OBJECTS; ++i)
        if (objects[i].active && objects[i].id == id)
            return &objects[i];
    return NULL;
}

static int add_object(ObjectType type, const ObjectParams *params) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!objects[i].active && objects[i].type == OBJ_NONE) {
            objects[i].id = next_id++;
            objects[i].type = type;
            objects[i].active = true;
            objects[i].params = *params;
            redraw_all();
            return objects[i].id;
        }
    }
    printf("Error: capacity is full.\n");
    return -1;
}

static bool remove_object(int id) {
    Object *obj = find_object(id);
    if (!obj) return false;
    obj->active = false;
    obj->type = OBJ_NONE;
    redraw_all();
    return true;
}

static bool modify_object(int id, const ObjectParams *params) {
    Object *obj = find_object(id);
    if (!obj) return false;
    obj->params = *params;
    redraw_all();
    return true;
}

static void list_objects(void) {
    printf("\n%-5s %-10s %s\n", "ID", "Type", "Parameters");
    printf("----------------------------------------\n");
    bool found = false;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!objects[i].active)
            continue;
        found = true;
        const Object *obj = &objects[i];
        printf("%-5d %-10s ", obj->id, ObjectTypeName[obj->type]);
        switch (obj->type) {
            case OBJ_CIRCLE:
                printf("center=(%d,%d) radius=%d\n", obj->params.x, obj->params.y, obj->params.p1);
                break;
            case OBJ_RECTANGLE:
                printf("top-left=(%d,%d) w=%d h=%d\n", obj->params.x, obj->params.y, obj->params.p1, obj->params.p2);
                break;
            case OBJ_LINE:
                printf("(%d,%d)->(%d,%d)\n", obj->params.x, obj->params.y, obj->params.p1, obj->params.p2);
                break;
            case OBJ_TRIANGLE:
                printf("(%d,%d) (%d,%d) (%d,%d)\n",
                       obj->params.x, obj->params.y,
                       obj->params.p1, obj->params.p2,
                       obj->params.p3, obj->params.p4);
                break;
            default:
                printf("unknown\n");
                break;
        }
    }
    if (!found)
        printf("  (no objects)\n");
    printf("\n");
}

static bool save_scene(const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Unable to open file");
        return false;
    }
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!objects[i].active) continue;
        fprintf(out, "%d %d %d %d %d %d %d %d\n",
                objects[i].id,
                objects[i].type,
                objects[i].params.x,
                objects[i].params.y,
                objects[i].params.p1,
                objects[i].params.p2,
                objects[i].params.p3,
                objects[i].params.p4);
    }
    fclose(out);
    return true;
}

static bool load_scene(const char *filename) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        perror("Unable to open file");
        return false;
    }

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        objects[i].active = false;
        objects[i].type = OBJ_NONE;
    }

    int id, type, x, y, p1, p2, p3, p4;
    int max_id = 0;
    while (fscanf(in, "%d %d %d %d %d %d %d %d", &id, &type, &x, &y, &p1, &p2, &p3, &p4) == 8) {
        for (int i = 0; i < MAX_OBJECTS; ++i) {
            if (!objects[i].active) {
                objects[i].id = id;
                objects[i].type = (ObjectType)type;
                objects[i].active = true;
                objects[i].params.x = x;
                objects[i].params.y = y;
                objects[i].params.p1 = p1;
                objects[i].params.p2 = p2;
                objects[i].params.p3 = p3;
                objects[i].params.p4 = p4;
                if (id > max_id)
                    max_id = id;
                break;
            }
        }
    }
    fclose(in);
    next_id = max_id + 1;
    redraw_all();
    return true;
}

static void print_menu(void) {
    printf("\n=== PREMIUM 2D ASCII GRAPHICS EDITOR ===\n");
    printf("1) Add circle\n");
    printf("2) Add rectangle\n");
    printf("3) Add line\n");
    printf("4) Add triangle\n");
    printf("5) Delete object\n");
    printf("6) Modify object\n");
    printf("7) List objects\n");
    printf("8) Display canvas\n");
    printf("9) Save scene\n");
    printf("10) Load scene\n");
    printf("0) Exit\n");
    printf("Choice: ");
}

int main(void) {
    canvas_clear();

    while (true) {
        char line[INPUT_BUFFER];
        print_menu();
        if (!fgets(line, sizeof(line), stdin))
            break;

        int choice;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Please enter a valid menu number.\n");
            continue;
        }

        ObjectParams params = {0};
        int id;

        switch (choice) {
            case 0:
                printf("Goodbye!\n");
                return 0;

            case 1: {
                int values[3];
                if (!read_numbers("Enter centre (col row) and radius: ", 3, values))
                    break;
                params.x = values[0];
                params.y = values[1];
                params.p1 = values[2];
                add_object(OBJ_CIRCLE, &params);
                redraw_all();
                canvas_display();
                break;
            }

            case 2: {
                int values[4];
                if (!read_numbers("Enter top-left (col row) and width height: ", 4, values))
                    break;
                params.x = values[0];
                params.y = values[1];
                params.p1 = values[2];
                params.p2 = values[3];
                add_object(OBJ_RECTANGLE, &params);
                redraw_all();
                canvas_display();
                break;
            }

            case 3: {
                int values[4];
                if (!read_numbers("Enter start (col row) and end (col row): ", 4, values))
                    break;
                params.x = values[0];
                params.y = values[1];
                params.p1 = values[2];
                params.p2 = values[3];
                add_object(OBJ_LINE, &params);
                redraw_all();
                canvas_display();
                break;
            }

            case 4: {
                int values[6];
                if (!read_numbers("Enter vertex1, vertex2 and vertex3 (col row ...): ", 6, values))
                    break;
                params.x = values[0];
                params.y = values[1];
                params.p1 = values[2];
                params.p2 = values[3];
                params.p3 = values[4];
                params.p4 = values[5];
                add_object(OBJ_TRIANGLE, &params);
                redraw_all();
                canvas_display();
                break;
            }

            case 5:
                if (!read_numbers("Enter object ID to delete: ", 1, &id))
                    break;
                if (remove_object(id))
                    printf("Deleted object #%d.\n", id);
                else
                    printf("Object #%d not found.\n", id);
                canvas_display();
                break;

            case 6:
                if (!read_numbers("Enter object ID to modify: ", 1, &id))
                    break;
                {
                    Object *obj = find_object(id);
                    if (!obj) {
                        printf("Object #%d not found.\n", id);
                        break;
                    }
                    switch (obj->type) {
                        case OBJ_CIRCLE: {
                            int values[3];
                            if (!read_numbers("New centre (col row) and radius: ", 3, values)) break;
                            params.x = values[0];
                            params.y = values[1];
                            params.p1 = values[2];
                            break;
                        }
                        case OBJ_RECTANGLE: {
                            int values[4];
                            if (!read_numbers("New top-left (col row) and width height: ", 4, values)) break;
                            params.x = values[0];
                            params.y = values[1];
                            params.p1 = values[2];
                            params.p2 = values[3];
                            break;
                        }
                        case OBJ_LINE: {
                            int values[4];
                            if (!read_numbers("New start (col row) and end (col row): ", 4, values)) break;
                            params.x = values[0];
                            params.y = values[1];
                            params.p1 = values[2];
                            params.p2 = values[3];
                            break;
                        }
                        case OBJ_TRIANGLE: {
                            int values[6];
                            if (!read_numbers("New vertices (col row ...): ", 6, values)) break;
                            params.x = values[0];
                            params.y = values[1];
                            params.p1 = values[2];
                            params.p2 = values[3];
                            params.p3 = values[4];
                            params.p4 = values[5];
                            break;
                        }
                        default:
                            printf("Unsupported object type.\n");
                            continue;
                    }
                    modify_object(id, &params);
                    printf("Modified object #%d.\n", id);
                    canvas_display();
                }
                break;

            case 7:
                list_objects();
                break;

            case 8:
                canvas_display();
                break;

            case 9:
                printf("Enter filename to save: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                line[strcspn(line, "\r\n")] = '\0';
                if (save_scene(line))
                    printf("Scene saved to %s\n", line);
                break;

            case 10:
                printf("Enter filename to load: ");
                if (!fgets(line, sizeof(line), stdin)) break;
                line[strcspn(line, "\r\n")] = '\0';
                if (load_scene(line))
                    printf("Scene loaded from %s\n", line);
                break;

            default:
                printf("Invalid choice. Please select a menu item.\n");
                break;
        }
    }
    return 0;
}
