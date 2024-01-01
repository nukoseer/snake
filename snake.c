#include "utils.h"
#include "memory_utils.h"
#include "snake.h"

#define CELL_SIZE      (50)
#define CELL_CENTER(n) ((n) * CELL_SIZE + (CELL_SIZE / 2))
#define CELL_MAX_COUNT (100)

#define STEP_INTERVAL_DEFAULT (90.0f / 1000.0f)
#define SNAKE_DEFAULT_LENGTH  (2)

#define ARROW_LEFT  ((u8*)"\xCB\x82")
#define ARROW_RIGHT ((u8*)"\xCB\x83")
#define ARROW_DOWN  ((u8*)"\xCB\x84")
#define ARROW_UP    ((u8*)"\xCB\x85")

typedef struct Cell
{
    u32 x, y;
} Cell;

typedef struct Direction
{
    i32 x, y;
} Direction;

typedef struct _CellNode
{
    struct _CellNode* prev;
    struct _CellNode* next;
    Cell cell;
    Direction direction;
} CellNode;

typedef struct Snake
{
    CellNode head;
    u32 length;
    Direction direction;
} Snake;

typedef struct GameState
{
    MemoryArena* arena;

    u32 width;
    u32 height;

    u32 row_count;
    u32 column_count;

    Snake snake;
    f32 step_interval;
    f32 delta_time;

    u32 key_pressed;
} GameState;

extern u8 __heap_base;
extern u8 __data_end;

static GameState* game_state;

static CellNode* cell_node_create(MemoryArena* arena, u32 x, u32 y)
{
    CellNode* cell_node = PUSH_STRUCT(arena, CellNode);

    cell_node->cell = (Cell){ x, y };

    return cell_node;
}

static CellNode* cell_node_create_and_insert(MemoryArena* arena, CellNode* sentinel, u32 x, u32 y)
{
    CellNode* cell_node = cell_node_create(arena, x, y);

    DOUBLY_LINKED_LIST_INSERT(sentinel, cell_node);

    return cell_node;
}

#ifdef DEBUG
static u8* get_direction_arrow(CellNode* node)
{
    Snake* snake = &game_state->snake;
    u8* sign = 0;
    Direction* direction = 0;

    if (node->prev != &snake->head)
    {
        direction = &node->direction;
    }
    else
    {
        direction = &snake->direction;
    }

    if (direction->x == -1)
    {
        sign = ARROW_LEFT;
    }
    else if (direction->x == 1)
    {
        sign = ARROW_RIGHT;
    }
    else if (direction->y == -1)
    {
        sign = ARROW_DOWN;
    }
    else if (direction->y == 1)
    {
        sign = ARROW_UP;
    }

    return sign;
}
#endif

static void print_text(const u8* text)
{
    platform_print_text(text);
}

static void draw_text(const u8* text, u32 x, u32 y, u32 size, u32 color)
{
    platform_draw_text(text, x, y, size, color);
}

static void draw_number(u32 number, u32 x, u32 y, u32 size, u32 color)
{
    platform_draw_number(number, x, y, size, color);
}

static void draw_rectangle(u32 x, u32 y, u32 width, u32 height, u32 color, u32 fill)
{
    platform_draw_rectangle(x, y, width, height, color, fill);
}

static void snake_init(GameState* state)
{
    u32 x = state->column_count / 2;
    u32 y = state->row_count / 2;
    Snake* snake = &state->snake;

    DOUBLY_LINKED_LIST_INIT(&snake->head);

    snake->length = SNAKE_DEFAULT_LENGTH;

    CellNode* cell_node = &snake->head;

    for (u32 i = 0; i < snake->length; ++i)
    {
        cell_node = cell_node_create_and_insert(state->arena, cell_node, x, y);
    }
}

u32 get_arena_size(void)
{
    return game_state->arena->size;
}

u32 get_arena_used(void)
{
    return game_state->arena->used;
}

void game_key_down(u32 key)
{
    game_state->key_pressed = key;
}

void game_update(f32 delta_time)
{
    game_state->delta_time = delta_time;
    game_state->step_interval -= delta_time;

    if (game_state->step_interval <= 0.0f)
    {
        Snake* snake = &game_state->snake;
        CellNode* sentinel = snake->head.next;
        Cell prev_cell = sentinel->cell;
        Direction prev_direction = snake->direction;
        
        switch (game_state->key_pressed)
        {
            case 'a':
            {
                if (snake->direction.x != 1)
                {
                    snake->direction.x = -1;
                    snake->direction.y = 0;
                }
            }
            break;
            case 'd':
            {
                if (snake->direction.x != -1)
                {
                    snake->direction.x = 1;
                    snake->direction.y = 0;
                }
            }
            break;
            case 'w':
            {
                if (snake->direction.y != 1)
                {
                    snake->direction.x = 0;
                    snake->direction.y = -1;
                }
            }
            break;
            case 's':
            {
                if (snake->direction.y != -1)
                {
                    snake->direction.x = 0;
                    snake->direction.y = 1;
                }
            }
            break;
        }

        sentinel->cell.x = modulo(sentinel->cell.x + snake->direction.x, game_state->column_count);
        sentinel->cell.y = modulo(sentinel->cell.y + snake->direction.y, game_state->row_count);

        // NOTE: This if check is for initial snake state.
        // Maybe it is not that important.
        if (!(snake->direction.x == 0 && snake->direction.y == 0))
        {
            for (CellNode* node = sentinel->next; node != &snake->head; node = node->next)
            {
                SWAP(prev_cell, node->cell, Cell);
                SWAP(prev_direction, node->direction, Direction);
            }
        }

        game_state->step_interval = STEP_INTERVAL_DEFAULT;
    }
}

void game_render(void)
{
    if (game_state->step_interval == STEP_INTERVAL_DEFAULT)
    {
        Snake* snake = &game_state->snake;

        draw_rectangle(0, 0, game_state->width, game_state->height, 0xFFFFFFFF, 1);
        draw_number(0.001f / (game_state->delta_time * 0.001f), CELL_SIZE * 0.5f, CELL_SIZE * 0.5f + 6, 24, 0x000000FF);

        for (u32 y = 0; y < game_state->row_count; ++y)
        {
            for (u32 x = 0; x < game_state->column_count; ++x)
            {
                draw_rectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, 0x0000FF20, 0);
            }
        }

        for (CellNode* node = snake->head.next; node != &snake->head; node = node->next)
        {
            Cell cell = node->cell;

            draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, 0xFFFF0080, 1);
            draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, 0x00000020, 0);

#ifdef DEBUG
            u8* arrow = get_direction_arrow(node);

            if (arrow)
            {
                draw_text(arrow, CELL_CENTER(cell.x), CELL_CENTER(cell.y) + 6, 24, 0x000000FF);
            }
#endif
       }
    }
}

void game_init(u32 width, u32 height)
{
    MemoryArena* arena = get_memory_arena(&__heap_base, &__heap_base - &__data_end);

    game_state = PUSH_STRUCT(arena, GameState);
    game_state->arena = arena;
    game_state->width = width;
    game_state->height = height;
    game_state->row_count = height / CELL_SIZE;
    game_state->column_count = width / CELL_SIZE;
    game_state->step_interval = STEP_INTERVAL_DEFAULT;

    snake_init(game_state);
}
