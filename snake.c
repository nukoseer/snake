#include "utils.h"
#include "memory_utils.h"
#include "snake.h"

#define CELL_SIZE      (50)
#define CELL_CENTER(n) ((n) * CELL_SIZE + (CELL_SIZE / 2))
#define CELL_MAX_COUNT (100)

#define STEP_INTERVAL_DEFAULT (60.0f / 1000.0f)
#define SNAKE_DEFAULT_LENGTH  (8)

#define ARROW_LEFT_SYMBOL  ((u8*)"\xCB\x82")
#define ARROW_RIGHT_SYMBOL ((u8*)"\xCB\x83")
#define ARROW_DOWN_SYMBOL  ((u8*)"\xCB\x84")
#define ARROW_UP_SYMBOL    ((u8*)"\xCB\x85")

#define PRIMARY_ACENT_COLOR     (0xDA1A21FF) // NOTE: Maximum Red
#define PRIMARY_BASE_COLOR      (0xECE0D3FF) // NOTE: White Coffe
#define SECONDARY_ACENT_COLOR   (0x212120FF) // NOTE: Raisin Black
#define SECONDARY_BASE_COLOR    (0xF5FBFCFF) // NOTE: Mint Cream

#define BACKGROUND_COLOR   PRIMARY_BASE_COLOR

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

typedef enum RunState
{
    RUN_STATE_START_GAME,
    RUN_STATE_PLAY_GAME,
    RUN_STATE_END_GAME,
} RunState;

typedef enum Alignment
{
    ALIGNMENT_LEFT,
    ALIGNMENT_CENTER,
    ALIGNMENT_RIGHT
} Alignment;

typedef struct GameState
{
    MemoryArena* arena;

    u32 width;
    u32 height;
    f32 aspect_ratio;

    u32 row_count;
    u32 column_count;

    Snake snake;
    u32 score;
    u8 score_string[16];

    f32 step_interval;
    f32 delta_time;

    u32 key_pressed;

    RunState run_state;
} GameState;

extern u8 __heap_base;
extern u8 __data_end;

static GameState* game_state;
static const char* alignments[] =
{
    "left", "center", "right"
};

#define GET_ALIGNMENT(alignment) (u8*)alignments[alignment]

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
static u8* get_arrow_symbol(CellNode* node)
{
    Snake* snake = &game_state->snake;
    u8* symbol = 0;
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
        symbol = ARROW_LEFT_SYMBOL;
    }
    else if (direction->x == 1)
    {
        symbol = ARROW_RIGHT_SYMBOL;
    }
    else if (direction->y == -1)
    {
        symbol = ARROW_DOWN_SYMBOL;
    }
    else if (direction->y == 1)
    {
        symbol = ARROW_UP_SYMBOL;
    }

    return symbol;
}
#endif

static void print_text(const u8* text)
{
    platform_print_text(text);
}

static void draw_text(const u8* text, u32 x, u32 y, u32 size, u32 color, u32 fill, u8* alignment)
{
    platform_draw_text(text, x, y, size, color, fill, alignment);
}

static void draw_number(u32 number, u32 x, u32 y, u32 size, u32 color, u32 fill, u8* alignment)
{
    platform_draw_number(number, x, y, size, color, fill, alignment);
}

static void draw_rectangle(u32 x, u32 y, u32 width, u32 height, u32 color, u32 fill)
{
    platform_draw_rectangle(x, y, width, height, color, fill);
}

static void clear_screen(GameState* state)
{
    draw_rectangle(0, 0, state->width, state->height, BACKGROUND_COLOR, 1);
    draw_rectangle(0, 0, state->width, state->height, PRIMARY_ACENT_COLOR, 0);
}

static void draw_snake(Snake* snake)
{
    CellNode* node = snake->head.next;
    Cell cell = node->cell;

    draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, (SECONDARY_ACENT_COLOR & 0xFFFFFF00) | 0xCE, 1);
    // draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, PRIMARY_BASE_COLOR, 0);

    for (node = node->next; node != &snake->head; node = node->next)
    {
        cell = node->cell;

        draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, PRIMARY_ACENT_COLOR, 1);
        draw_rectangle(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, PRIMARY_BASE_COLOR, 0);

#ifdef DEBUG
        u8* arrow_symbol = get_arrow_symbol(node);

        if (arrow_symbol)
        {
            draw_text(arrow_symbol, CELL_CENTER(cell.x), CELL_CENTER(cell.y) + 6, 24, 0x000000FF, 1, alignments[ALIGNMENT_CENTER]);
        }
#endif
    }    
}

static void draw_statistics(GameState* state)
{
    draw_rectangle(0, 0, state->width, state->height, PRIMARY_ACENT_COLOR, 0);
    // draw_number(0.001f / (state->delta_time * 0.001f), 24, 32, 24, SECONDARY_ACENT_COLOR, 1);
}

static void draw_score(GameState* state, u32 x, u32 y, u32 size, u8* alignment)
{
    u32_to_string(state->score_string + 7, state->score);
    draw_text(state->score_string, x, y, size, PRIMARY_ACENT_COLOR, 1, alignment);
}

static void draw_coord(GameState* state)
{
    // for (u32 y = 0; y < state->row_count; ++y)
    // {
    //     for (u32 x = 0; x < state->column_count; ++x)
    //     {
    //         draw_rectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, SECONDARY_BASE_COLOR, 0);
    //     }   
    // }
    
    draw_rectangle(state->width / 2, 0,
                   state->width / 2, state->height, 
                   SECONDARY_ACENT_COLOR, 0);
    draw_rectangle(0,            state->height / 2,
                   state->width, state->height / 2, 
                   SECONDARY_ACENT_COLOR, 0);
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
    if (key == 32)
    {
        if (game_state->run_state == RUN_STATE_PLAY_GAME)
        {
            game_state->run_state = RUN_STATE_START_GAME;
        }
        else if (game_state->run_state == RUN_STATE_START_GAME)
        {
            game_state->run_state = RUN_STATE_PLAY_GAME;
        }
    }
    else if (game_state->run_state == RUN_STATE_PLAY_GAME)
    {
        game_state->key_pressed = key;
    }
}

void game_update(f32 delta_time)
{
    if (game_state->run_state == RUN_STATE_PLAY_GAME)
    {
        game_state->delta_time = delta_time;
        game_state->step_interval -= delta_time;

        if (game_state->step_interval <= 0.0f)
        {
            Snake* snake = &game_state->snake;

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

            CellNode* sentinel = snake->head.next;
            Cell prev_cell = sentinel->cell;
            Direction prev_direction = snake->direction;

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
}

void game_render(void)
{
    if (game_state->run_state == RUN_STATE_PLAY_GAME)
    {
        if (game_state->step_interval == STEP_INTERVAL_DEFAULT)
        {
            clear_screen(game_state);
            draw_snake(&game_state->snake);
            draw_statistics(game_state);
            draw_coord(game_state);
            u32 score_length = string_length(game_state->score_string);
            draw_number(score_length, game_state->width / 2, game_state->height / 2, 32, 0x000000FF, 1, GET_ALIGNMENT(ALIGNMENT_CENTER));
            u32 size = 36;
            u32 y = 0;
            u32 x = (f32)y * game_state->aspect_ratio;
            draw_score(game_state, x + size / 4, y + size, size, GET_ALIGNMENT(ALIGNMENT_LEFT));
        }
    }
    else if (game_state->run_state == RUN_STATE_START_GAME)
    {
        clear_screen(game_state);
        draw_text((u8*)"SNAKE", game_state->width / 2, game_state->height / 2, 112, PRIMARY_ACENT_COLOR, 1, GET_ALIGNMENT(ALIGNMENT_CENTER));
        draw_coord(game_state);
        draw_score(game_state, game_state->width / 2, game_state->height / 2 + 112, 84, GET_ALIGNMENT(ALIGNMENT_CENTER));
    }
}

void game_init(u32 width, u32 height)
{
    MemoryArena* arena = get_memory_arena(&__heap_base, &__heap_base - &__data_end);

    game_state = PUSH_STRUCT(arena, GameState);
    game_state->arena = arena;
    game_state->width = width;
    game_state->height = height;
    game_state->aspect_ratio = (f32)width / (f32)height;
    game_state->row_count = height / CELL_SIZE;
    game_state->column_count = width / CELL_SIZE;
    game_state->step_interval = STEP_INTERVAL_DEFAULT;
    game_state->run_state = RUN_STATE_START_GAME;

    game_state->score = 0;
    memory_set(game_state->score_string, ' ', ARRAY_COUNT(game_state->score_string));
    memory_copy(game_state->score_string, "SCORE:", 6);

    snake_init(game_state);
}
