#include "pebble.h"
#include "MyBus.h"

static Window *window;
static Window *nostops_window;
static Window *refreshing_window;
static BitmapLayer *image_layer;
static GBitmap *image;
static DictionaryIterator *dict;
static MenuLayer* mainMenu;
MenuLayerCallbacks cbacks;

static BitmapLayer *nostops_image_layer;
static GBitmap *nostops_image;
static BitmapLayer *refreshing_image_layer;
static GBitmap *refreshing_image;

/**
 *	Message Functions
 */

static void send_cmd(void) {
  Tuplet value = TupletInteger(0, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

/**
 *  Refresh window
 */


static void refreshing_window_load() {

	refreshing_image = gbitmap_create_with_resource(RESOURCE_ID_REFRESHING);
	
	Layer *window_layer = window_get_root_layer(refreshing_window);
	GRect bounds = layer_get_frame(window_layer);

	// Image Layer
	refreshing_image_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_background_color(refreshing_image_layer, GColorBlack);
	bitmap_layer_set_bitmap(refreshing_image_layer, nostops_image);
	bitmap_layer_set_alignment(refreshing_image_layer, GAlignCenter);
	layer_add_child(window_layer, bitmap_layer_get_layer(refreshing_image_layer));
}

static void push_refreshing_window() {
	window_stack_push(refreshing_window, true);
}

static void refreshing_window_unload() {
	gbitmap_destroy(refreshing_image);
	bitmap_layer_destroy(refreshing_image_layer);
}

static void refreshing_window_init() {
	refreshing_window = window_create();
	window_set_window_handlers(refreshing_window, (WindowHandlers) {
	 .load = refreshing_window_load,
	 .unload = refreshing_window_unload,
	});
}


/**
 *  No stops found
 */

static void nostops_window_load() {

	nostops_image = gbitmap_create_with_resource(RESOURCE_ID_NOBUSSTOP);
	
	Layer *window_layer = window_get_root_layer(nostops_window);
	GRect bounds = layer_get_frame(window_layer);

	// Image Layer
	nostops_image_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_background_color(nostops_image_layer, GColorBlack);
	bitmap_layer_set_bitmap(nostops_image_layer, nostops_image);
	bitmap_layer_set_alignment(nostops_image_layer, GAlignCenter);
	layer_add_child(window_layer, bitmap_layer_get_layer(nostops_image_layer));
}

static void push_no_stops_window() {
	window_stack_push(nostops_window, true);
}

static void nostops_window_unload() {
	gbitmap_destroy(nostops_image);
	bitmap_layer_destroy(nostops_image_layer);
}

void refresh_bus_request(ClickRecognizerRef recognizer, void *context) {
	// APP_LOG(APP_LOG_LEVEL_DEBUG, "Calling refresh on javascript");
	send_cmd();
	window_stack_pop(true);
	push_refreshing_window();
}

void nostops_window_click_config_provider(void *callback_context) {
	window_single_click_subscribe(BUTTON_ID_DOWN, refresh_bus_request);
}

static void nostops_window_init() {
	nostops_window = window_create();
	window_set_window_handlers(nostops_window, (WindowHandlers) {
	 .load = nostops_window_load,
	 .unload = nostops_window_unload,
	});
	window_set_click_config_provider(nostops_window, nostops_window_click_config_provider);
}

/**
 *  Message section - 2
 */

static void in_received_handler(DictionaryIterator *iter, void *context) {

	if(window_is_loaded(refreshing_window)) {
		window_stack_pop(true);
	}

	dict = iter;

	Tuple *t = dict_find(dict,0);

	// APP_LOG(APP_LOG_LEVEL_DEBUG, "payload: %d", t->value->uint8);

	if(t && t->value->uint8 > 0) {
		menu_layer_reload_data(mainMenu);
		gbitmap_destroy(image);
		bitmap_layer_destroy(image_layer);
	}
	else {
		// nostops_window_init();
		push_no_stops_window();
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped! with reason %s", translate_message_error(reason));
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

static void app_message_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

// void animation_stopped(Animation *animation, bool finished, void *data) {

// 	gbitmap_destroy(image);
// 	bitmap_layer_destroy(image_layer);

// 	// Layer *l = bitmap_layer_get_layer(nostops_image_layer);
//    GRect from_frame = layer_get_frame(nostops_container);
//    GRect to_frame = GRect(from_frame.size.w * (-0.5), 0, from_frame.size.w, from_frame.size.h);

//    nostops_image_layer_animation = property_animation_create_layer_frame(nostops_container, &from_frame, &to_frame);

//    animation_schedule((Animation*) nostops_image_layer_animation);
// }

// static void nostops_layers_animation() {

// 	Layer *l = bitmap_layer_get_layer(nostops_image_layer);
//    GRect from_frame = layer_get_frame(l);
//    GRect to_frame = GRect(0, 0, from_frame.size.w, from_frame.size.h);

//    nostops_image_layer_animation = property_animation_create_layer_frame(l, &from_frame, &to_frame);

// 	animation_set_handlers((Animation*) nostops_image_layer_animation, (AnimationHandlers) {
// 		.stopped = (AnimationStoppedHandler) animation_stopped
// 	}, NULL);
//    animation_schedule((Animation*) nostops_image_layer_animation);

// }

/**
 *	Menu functions
 */

uint16_t mainMenu_get_num_sections(struct MenuLayer *menu_layer, void *callback_context)
{
	return 1;
}

uint16_t mainMenu_get_num_rows_in_section(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
	if(!dict)
	 return 0;
	
	Tuple *t = dict_find(dict,0);
	if(t) {
	 return t->value->uint8;
	}
	else {
	 return 0;
	}
}

int16_t mainMenu_get_cell_height(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
	return 40;
}

void mainMenu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{ 
	if(!dict)
	 return;

	char *s = dict_find(dict, (cell_index->row)+100)->value->cstring;
	char *dest = dict_find(dict, (cell_index->row)+200)->value->cstring;
	
	graphics_context_set_text_color(ctx, GColorBlack); // This is important.
	graphics_draw_text(ctx, s, fonts_get_system_font(FONT_KEY_GOTHIC_24), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h-16), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, dest, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(5,layer_get_frame(cell_layer).size.h-16,layer_get_frame(cell_layer).size.w-5,16), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

}

static void menu_init(void) {
	GRect frame = layer_get_bounds(window_get_root_layer(window));
	
	mainMenu = menu_layer_create(GRect(0,0,frame.size.w,frame.size.h));

	menu_layer_set_click_config_onto_window(mainMenu, window); // Sets the Window's button callbacks to the MenuLayer's default button callbacks.
	
	cbacks.get_num_sections = &mainMenu_get_num_sections; // Gets called at the beginning of drawing the table.
	cbacks.get_num_rows = &mainMenu_get_num_rows_in_section; // Gets called at the beginning of drawing each section.
	cbacks.get_cell_height = &mainMenu_get_cell_height; // Gets called at the beginning of drawing each normal cell.
	cbacks.draw_row = &mainMenu_draw_row; // Gets called to set the content of a normal cell.
	
	// cbacks.get_header_height = &mainMenu_get_header_height; // Gets called at the beginning of drawing each header cell.
	// cbacks.select_click = &mainMenu_select_click; // Gets called when select is pressed.
	// cbacks.draw_header = &mainMenu_draw_header; // Gets called to set the content of a header cell.
	// cbacks.selection_changed = &func(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) // I assume this would be called whenever an up or down button was pressed.
	// cbacks.select_long_click = &func(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) // I didn't use this.
	
	menu_layer_set_callbacks(mainMenu, NULL, cbacks); // I have no user data to supply to the callback functions, so 
	layer_add_child(window_get_root_layer(window), menu_layer_get_layer(mainMenu));
}

static void splashscreen_init() {
	Layer *window_layer = window_get_root_layer(window);

	image = gbitmap_create_with_resource(RESOURCE_ID_SPLASHSCREEN);
	image_layer = bitmap_layer_create(layer_get_frame(window_layer));

	bitmap_layer_set_bitmap(image_layer, image);
	bitmap_layer_set_alignment(image_layer, GAlignCenter);
	layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
}

static void window_load(Window *window) {

}

static void window_unload(Window *window) {
	menu_layer_destroy(mainMenu);
}

static void init(void) {

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
	 .load = window_load,
	 .unload = window_unload,
	});
	window_stack_push(window, true);

	// Layer *window_layer = window_get_root_layer(window);

	splashscreen_init();
	refreshing_window_init();
	nostops_window_init();
	app_message_init();
	menu_init();

	// nostops_window_init();
}

static void deinit(void) {
	window_destroy(window);
	window_destroy(nostops_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
