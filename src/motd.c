#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_text_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;

static ScrollLayer *s_scroll_layer;

static char s_scroll_text[] = "For we labor diligently to write, to persuade our children, and also our brethren, to believe in Christ, and to be reconciled to God; for we know that it is by grace that we are saved, after all we can do. And we talk of Christ, we rejoice in Christ, we preach of Christ, we prophesy of Christ, and we write according to our prophecies, that our children may know to what source they may look for a remission of their sins.                                                                                                                         ";
//static char s_scroll_text[] = "What happens when the text doesn't need to scroll?";
static uint8_t pause_sec = 5;
static uint8_t cur_pause_sec = 0;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "++%%");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  // Needs to be static because it's used by the system later.
  static char s_time_text[] = "00:00:00";

  strftime(s_time_text, sizeof(s_time_text), "%T", tick_time);
  text_layer_set_text(s_time_layer, s_time_text);
  
  GSize  textSize    = text_layer_get_content_size(s_text_layer);       //positive integer that grows line by line as you scroll down the text
  GSize  scrollSize  = scroll_layer_get_content_size(s_scroll_layer);   //total height of the element that is to be scrolled
  GPoint scrollPoint = scroll_layer_get_content_offset(s_scroll_layer); //negative integer that lowers as you scroll down the text (0-start)
	
  if(cur_pause_sec > pause_sec) {
  scrollPoint.y -=10;
  scroll_layer_set_content_offset(s_scroll_layer,scrollPoint,true); 
  if((scrollSize.h-textSize.h) < 9) cur_pause_sec = pause_sec;
  } else {
    if((scrollSize.h-textSize.h) < 9) {
      if(cur_pause_sec != 0) {
        cur_pause_sec -= 1;
      } else {
        scroll_layer_set_content_offset(s_scroll_layer,GPointZero,false);
      }
    } else {
    cur_pause_sec += 1;
    }
  }
  
  //Debug scroll data
  
  static char scroll_point[] = "000000000";
  //snprintf(scroll_point, sizeof(scroll_point), "%d|%d", scrollPoint.y*-1, scrollSize.h-textSize.h);
  snprintf(scroll_point, sizeof(scroll_point), "%d|%d", cur_pause_sec, pause_sec);
  text_layer_set_text(s_connection_layer,scroll_point);
  
}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(s_connection_layer, connected ? "connected" : "disconnected");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 5000);

  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 34));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Initialize the scroll layer
  s_scroll_layer = scroll_layer_create(GRect(0, 34, bounds.size.w, bounds.size.h - 54));
  
  // Initialize the text layer
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(s_text_layer, s_scroll_text);
  
  // Trim text layer and scroll content to fit text box
  GSize max_size = text_layer_get_content_size(s_text_layer);
  max_size.w = bounds.size.w;
  if(max_size.h <= bounds.size.h - 58) max_size.h = bounds.size.h - 58;
  text_layer_set_size(s_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));
  
  // Add the layers for display
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  
  s_connection_layer = text_layer_create(GRect(0, 144, 101, 34));
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_background_color(s_connection_layer, GColorBlack);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentLeft);
  handle_bluetooth(bluetooth_connection_service_peek());

  s_battery_layer = text_layer_create(GRect(101, 144, 40, 34));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorBlack);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text(s_battery_layer, "100%");

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  battery_state_service_subscribe(handle_battery);
  bluetooth_connection_service_subscribe(handle_bluetooth);

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  //layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  
  handle_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
