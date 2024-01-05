'use strict';

let application = document.getElementById("application");
let context = application.getContext("2d");
let wasm = null;

function string_length(memory, start_address)
{
    let length = 0;
    let address = start_address;

    while (memory[address] != 0)
    {
	++length;
	++address;
    }

    return length;
}

function color_to_hex(color)
{
    const r = (color >> 24) & 0xFF;
    const g = (color >> 16) & 0xFF;
    const b = (color >> 8) & 0xFF;
    const a = (color >> 0) & 0xFF;

    return "#" +
	r.toString(16).padStart(2, '0') +
	g.toString(16).padStart(2, '0') +
	b.toString(16).padStart(2, '0') +
	a.toString(16).padStart(2, '0');
}

function convert_to_string(buffer, string_address)
{
    const memory = new Uint8Array(buffer);
    const length = string_length(memory, string_address);
    const bytes = new Uint8Array(buffer, string_address, length);

    return new TextDecoder().decode(bytes);
}

function platform_throw_error(string_address)
{
    const buffer = wasm.instance.exports.memory.buffer;
    const string = convert_to_string(buffer, string_address);
    throw new Error(string);
}

function platform_print_text(string_address)
{
    const buffer = wasm.instance.exports.memory.buffer;
    const string = convert_to_string(buffer, string_address)

    console.log(string);
}

function platform_draw_text(string_address, x, y, size, color, fill, alignment)
{
    const buffer = wasm.instance.exports.memory.buffer;
    const string = convert_to_string(buffer, string_address)
    const align = convert_to_string(buffer, alignment)

    context.font = size + "px Iosevka Fixed Web";
    context.textAlign = align;

    if (fill)
    {
	context.fillStyle = color_to_hex(color);
	context.fillText(string, x, y);
    }
    else
    {
	context.strokeStyle = color_to_hex(color);
	context.strokeText(string, x, y);
    }
}

function platform_draw_number(number, x, y, size, color, fill, alignment)
{
    const buffer = wasm.instance.exports.memory.buffer;
    const align = convert_to_string(buffer, alignment)

    context.font = size + "px Iosevka Fixed Web";
    context.textAlign = align;

    if (fill)
    {
	context.fillStyle = color_to_hex(color);
	context.fillText(number.toString(10), x, y);
    }
    else
    {
        context.strokeStyle = color_to_hex(color);
	context.strokeText(number.toString(10), x, y);
    }
}

function platform_draw_rectangle(x, y, width, height, color, fill)
{
    if (fill)
    {
	context.fillStyle = color_to_hex(color);
	context.fillRect(x, y, width, height);
    }
    else
    {
	context.strokeStyle = color_to_hex(color);
	context.strokeRect(x, y, width, height);
    }
}

let previous_timestamp = null;

function game_loop(timestamp)
{
    if (previous_timestamp != null)
    {
	let delta_time = (timestamp - previous_timestamp) / 1000;
	wasm.instance.exports.game_update(delta_time);
	wasm.instance.exports.game_render();
    }

    previous_timestamp = timestamp;
    window.requestAnimationFrame(game_loop);
}

WebAssembly.instantiateStreaming(
    fetch("./build/snake.wasm"),
    {
	env:
	{
	    platform_throw_error,
	    platform_print_text,
	    platform_draw_text,
	    platform_draw_number,
	    platform_draw_rectangle
	}
    }
).then((w) =>
    {
	wasm = w;

	context.font = "14px Iosevka Fixed Web";
        context.textAlign = "center";
	context.fillStyle = "#FFFFFF"
	context.fillText("", 0, 0);

	wasm.instance.exports.game_init(application.width, application.height);

	document.addEventListener(
	    "keydown",
	    function(event)
	    {
		let key = 0;
		
		if (event.keyCode === 37 || event.keyCode === 38 ||
		    event.keyCode === 39 || event.keyCode === 40)
                {
		    key = event.keyCode;
                }
		else
		{
		    key = event.key.charCodeAt();
		}

		wasm.instance.exports.game_key_down(key);
	    },
	);

	window.requestAnimationFrame(game_loop);
    }
);
