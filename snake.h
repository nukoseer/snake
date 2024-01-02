#ifndef H_SNAKE_H

void platform_print_text(const u8* text);
void platform_draw_text(const u8* text, u32 x, u32 y, u32 size, u32 color, u32 fill, u8* alignment);
void platform_draw_number(u32 number, u32 x, u32 y, u32 size, u32 color, u32 fill, u8* alignment);
void platform_draw_rectangle(u32 x, u32 y, u32 width, u32 height, u32 color, u32 fill);

void game_key_down(u32 key);
void game_update(f32 delta_time);
void game_render(void);
void game_init(u32 width, u32 height);

#define H_SNAKE_H
#endif
