# Harshith-Kumar-BV
/*
 * ============================================================
 *  2D Graphics Editor — C Implementation
 *  Canvas: 2D char array filled with '_', objects drawn with '*'
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Canvas dimensions ─────────────────────────────────── */
#define ROWS 25
#define COLS 60
#define EMPTY '_'
#define DRAW  '*'
#define MAX_OBJECTS 50

/* ── Object types ───────────────────────────────────────── */
typedef enum { OBJ_CIRCLE = 1, OBJ_RECTANGLE, OBJ_LINE, OBJ_TRIANGLE } ObjType;

/* ── Object descriptor ──────────────────────────────────── */
typedef struct {
    int      id;
    ObjType  type;
    /* circle  : x,y = centre,  p1=radius                  */
    /* rect    : x,y = top-left, p1=width, p2=height        */
    /* line    : x,y = start,   p1=endX,  p2=endY           */
    /* triangle: x,y = v1,      p1=v2x,p2=v2y, p3=v3x,p4=v3y */
    int x, y, p1, p2, p3, p4;
    int active; /* 0 = deleted */
} Object;

/* ── Global state ───────────────────────────────────────── */
static char   canvas[ROWS][COLS];
static Object objects[MAX_OBJECTS];
static int    obj_count  = 0;
static int    next_id    = 1;

/* ═══════════════════════════════════════════════════════════
 *  Canvas helpers
 * ═══════════════════════════════════════════════════════════ */
void canvas_clear(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = EMPTY;
}

void canvas_display(void) {
    printf("\n  ");
    for (int c = 0; c < COLS; c++) printf("%d", c % 10);
    printf("\n");
    for (int r = 0; r < ROWS; r++) {
        printf("%2d ", r);
        for (int c = 0; c < COLS; c++)
            putchar(canvas[r][c]);
        putchar('\n');
    }
    printf("\n");
}

/* Safe pixel setter */
static void plot(int r, int c) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = DRAW;
}

/* ═══════════════════════════════════════════════════════════
 *  Bresenham line
 * ═══════════════════════════════════════════════════════════ */
static void draw_line_raw(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy, e2;
    while (1) {
        plot(y0, x0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/* ═══════════════════════════════════════════════════════════
 *  Draw primitives (write directly onto canvas)
 * ═══════════════════════════════════════════════════════════ */
void draw_circle(int cx, int cy, int r) {
    /* Midpoint circle algorithm */
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        plot(cy + y, cx + x); plot(cy - y, cx + x);
        plot(cy + y, cx - x); plot(cy - y, cx - x);
        plot(cy + x, cx + y); plot(cy - x, cx + y);
        plot(cy + x, cx - y); plot(cy - x, cx - y);
        if (d < 0) d += 2 * x + 3;
        else      { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

void draw_rectangle(int x, int y, int w, int h) {
    for (int c = x; c < x + w; c++) { plot(y,       c); plot(y + h - 1, c); }
    for (int r = y; r < y + h; r++) { plot(r,       x); plot(r,         x + w - 1); }
}

void draw_line(int x0, int y0, int x1, int y1) {
    draw_line_raw(x0, y0, x1, y1);
}

void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line_raw(x1, y1, x2, y2);
    draw_line_raw(x2, y2, x3, y3);
    draw_line_raw(x3, y3, x1, y1);
}

/* ═══════════════════════════════════════════════════════════
 *  Redraw all active objects onto a fresh canvas
 * ═══════════════════════════════════════════════════════════ */
static void redraw_all(void) {
    canvas_clear();
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        Object *o = &objects[i];
        switch (o->type) {
            case OBJ_CIRCLE:    draw_circle   (o->x, o->y, o->p1); break;
            case OBJ_RECTANGLE: draw_rectangle(o->x, o->y, o->p1, o->p2); break;
            case OBJ_LINE:      draw_line     (o->x, o->y, o->p1, o->p2); break;
            case OBJ_TRIANGLE:  draw_triangle (o->x, o->y, o->p1, o->p2, o->p3, o->p4); break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 *  Add object
 * ═══════════════════════════════════════════════════════════ */
static int add_object(ObjType t, int x, int y,
                      int p1, int p2, int p3, int p4) {
    if (obj_count >= MAX_OBJECTS) {
        printf("Error: object limit reached.\n");
        return -1;
    }
    Object *o = &objects[obj_count++];
    o->id     = next_id++;
    o->type   = t;
    o->active = 1;
    o->x = x; o->y = y;
    o->p1 = p1; o->p2 = p2; o->p3 = p3; o->p4 = p4;
    redraw_all();
    printf("Added object #%d.\n", o->id);
    return o->id;
}

/* ═══════════════════════════════════════════════════════════
 *  Delete object by ID
 * ═══════════════════════════════════════════════════════════ */
void delete_object(int id) {
    for (int i = 0; i < obj_count; i++) {
        if (objects[i].id == id) {
            if (!objects[i].active) { printf("Object #%d already deleted.\n", id); return; }
            objects[i].active = 0;
            redraw_all();
            printf("Deleted object #%d.\n", id);
            return;
        }
    }
    printf("Object #%d not found.\n", id);
}

/* ═══════════════════════════════════════════════════════════
 *  Modify object by ID
 * ═══════════════════════════════════════════════════════════ */
void modify_object(int id, int x, int y, int p1, int p2, int p3, int p4) {
    for (int i = 0; i < obj_count; i++) {
        if (objects[i].id == id) {
            if (!objects[i].active) { printf("Object #%d is deleted.\n", id); return; }
            objects[i].x  = x;  objects[i].y  = y;
            objects[i].p1 = p1; objects[i].p2 = p2;
            objects[i].p3 = p3; objects[i].p4 = p4;
            redraw_all();
            printf("Modified object #%d.\n", id);
            return;
        }
    }
    printf("Object #%d not found.\n", id);
}

/* ═══════════════════════════════════════════════════════════
 *  List objects
 * ═══════════════════════════════════════════════════════════ */
void list_objects(void) {
    const char *names[] = {"", "Circle", "Rectangle", "Line", "Triangle"};
    printf("\n%-5s %-12s %s\n", "ID", "Type", "Parameters");
    printf("----------------------------------------------\n");
    int found = 0;
    for (int i = 0; i < obj_count; i++) {
        Object *o = &objects[i];
        if (!o->active) continue;
        found = 1;
        printf("%-5d %-12s ", o->id, names[o->type]);
        switch (o->type) {
            case OBJ_CIRCLE:
                printf("centre=(%d,%d) radius=%d\n", o->x, o->y, o->p1); break;
            case OBJ_RECTANGLE:
                printf("topleft=(%d,%d) w=%d h=%d\n", o->x, o->y, o->p1, o->p2); break;
            case OBJ_LINE:
                printf("(%d,%d)->(%d,%d)\n", o->x, o->y, o->p1, o->p2); break;
            case OBJ_TRIANGLE:
                printf("(%d,%d) (%d,%d) (%d,%d)\n",
                       o->x, o->y, o->p1, o->p2, o->p3, o->p4); break;
        }
    }
    if (!found) printf("  (no objects)\n");
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════
 *  Menu-driven main
 * ═══════════════════════════════════════════════════════════ */
static void print_menu(void) {
    printf("┌─────────────────────────────────────┐\n");
    printf("│       2D GRAPHICS EDITOR            │\n");
    printf("├─────────────────────────────────────┤\n");
    printf("│  1. Add Circle                      │\n");
    printf("│  2. Add Rectangle                   │\n");
    printf("│  3. Add Line                        │\n");
    printf("│  4. Add Triangle                    │\n");
    printf("│  5. Delete Object                   │\n");
    printf("│  6. Modify Object                   │\n");
    printf("│  7. Display Canvas                  │\n");
    printf("│  8. List Objects                    │\n");
    printf("│  0. Exit                            │\n");
    printf("└─────────────────────────────────────┘\n");
    printf("Choice: ");
}

int main(void) {
    canvas_clear();
    int choice;

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) break;

        if (choice == 0) { printf("Goodbye!\n"); break; }

        int id, x, y, p1, p2, p3, p4;

        switch (choice) {
            case 1: /* Circle */
                printf("Centre (col row): "); scanf("%d %d", &x, &y);
                printf("Radius: ");           scanf("%d", &p1);
                add_object(OBJ_CIRCLE, x, y, p1, 0, 0, 0);
                canvas_display();
                break;

            case 2: /* Rectangle */
                printf("Top-left (col row): "); scanf("%d %d", &x, &y);
                printf("Width Height: ");       scanf("%d %d", &p1, &p2);
                add_object(OBJ_RECTANGLE, x, y, p1, p2, 0, 0);
                canvas_display();
                break;

            case 3: /* Line */
                printf("Start (col row): "); scanf("%d %d", &x,  &y);
                printf("End   (col row): "); scanf("%d %d", &p1, &p2);
                add_object(OBJ_LINE, x, y, p1, p2, 0, 0);
                canvas_display();
                break;

            case 4: /* Triangle */
                printf("Vertex 1 (col row): "); scanf("%d %d", &x,  &y);
                printf("Vertex 2 (col row): "); scanf("%d %d", &p1, &p2);
                printf("Vertex 3 (col row): "); scanf("%d %d", &p3, &p4);
                add_object(OBJ_TRIANGLE, x, y, p1, p2, p3, p4);
                canvas_display();
                break;

            case 5: /* Delete */
                list_objects();
                printf("Object ID to delete: "); scanf("%d", &id);
                delete_object(id);
                canvas_display();
                break;

            case 6: /* Modify */
                list_objects();
                printf("Object ID to modify: "); scanf("%d", &id);
                /* Find type first */
                {
                    int found = 0;
                    for (int i = 0; i < obj_count; i++) {
                        if (objects[i].id == id && objects[i].active) {
                            found = 1;
                            switch (objects[i].type) {
                                case OBJ_CIRCLE:
                                    printf("New centre (col row): "); scanf("%d %d", &x, &y);
                                    printf("New radius: ");            scanf("%d", &p1);
                                    modify_object(id, x, y, p1, 0, 0, 0);
                                    break;
                                case OBJ_RECTANGLE:
                                    printf("New top-left (col row): "); scanf("%d %d", &x, &y);
                                    printf("New width height: ");        scanf("%d %d", &p1, &p2);
                                    modify_object(id, x, y, p1, p2, 0, 0);
                                    break;
                                case OBJ_LINE:
                                    printf("New start (col row): "); scanf("%d %d", &x,  &y);
                                    printf("New end   (col row): "); scanf("%d %d", &p1, &p2);
                                    modify_object(id, x, y, p1, p2, 0, 0);
                                    break;
                                case OBJ_TRIANGLE:
                                    printf("New v1 (col row): "); scanf("%d %d", &x,  &y);
                                    printf("New v2 (col row): "); scanf("%d %d", &p1, &p2);
                                    printf("New v3 (col row): "); scanf("%d %d", &p3, &p4);
                                    modify_object(id, x, y, p1, p2, p3, p4);
                                    break;
                            }
                            break;
                        }
                    }
                    if (!found) printf("Object #%d not found or deleted.\n", id);
                }
                canvas_display();
                break;

            case 7: /* Display */
                canvas_display();
                break;

            case 8: /* List */
                list_objects();
                break;

            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}