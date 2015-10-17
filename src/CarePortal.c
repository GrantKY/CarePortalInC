#include "pebble.h"
#include "stddef.h"
#include "string.h"
//https://github.com/pebble-examples/feature-simple-menu-layer
#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 2
#define NUM_SECOND_MENU_ITEMS 1

#define KEY_DATA 5
#define KEY_VALUE 6

static Window *s_main_window = NULL;
static Window *carbs_window = NULL;
static Window *insulin_window = NULL;
static Window *populate_window = NULL;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
TextLayer *graph_text_layer_carbs = NULL;
TextLayer *graph_text_layer_insulin = NULL;
TextLayer *graph_text_layer_populate = NULL;

char outputtext[100];
char fractionaText[10];

char keyname[20];
char resultvalue[40];

int integerpart = 0;
int fractionalpart=0;
bool bIntegerPart_set = false;

static GBitmap *s_menu_icon_image;

int Set_IntegerPart(int increment)
{
    integerpart+= increment;  
    if(integerpart < 0)
    {
      integerpart = 0;
    }
  
    return integerpart;
}

int Set_FractionPart(int increment)
{
  fractionalpart += increment;
  if(fractionalpart< 0)
  {
      fractionalpart = 0; 
  }
  else if(fractionalpart >=100)
  {
      fractionalpart = 0;
  }
   return fractionalpart;
}

char* GetFractionaPartAsChar() 
{
  if(fractionalpart<=5)
  {
      snprintf(fractionaText, 10, "0%d", fractionalpart);
  }
  else
  {
      snprintf(fractionaText, 10, "%d", fractionalpart);
  }
  
  return fractionaText; 
}

void Set_Part(bool currentPartSet, int increment)
{
    if(!currentPartSet)
    {      
       Set_IntegerPart(increment);
    }
    else
    {
      if(increment > 0)
      {
         Set_FractionPart(5);
      }
      else
      {  
         Set_FractionPart(-5);
      }
    }
}

void ResetToDefaults()
{
  integerpart = 0;
  fractionalpart = 0;
  bIntegerPart_set = false;
}

//////////////////////// POPULATE WINDOW ///////////////////////////////////////
void select_click_handler_populate(ClickRecognizerRef recognizer, void *context) {
   DictionaryIterator *iter;
   AppMessageResult result = app_message_outbox_begin(&iter);
  
   if(result == APP_MSG_OK)
    { 
      char * const_key;
      const_key = keyname;
  
      char * const_result;
      const_result = resultvalue;
      dict_write_cstring(iter, KEY_DATA, const_key);
      dict_write_cstring(iter, KEY_VALUE, const_result);
 
      app_message_outbox_send();
    
      window_stack_pop_all(true);
      ResetToDefaults();
      window_stack_push(s_main_window, true);
  }

  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: Exiting###");
}


static void click_config_provider_populate(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_populate);
}

void populate_load_window(Window * window)
{
    Layer *window_layer_graph = NULL;
    
    window_layer_graph = window_get_root_layer(populate_window);
    graph_text_layer_populate = text_layer_create(GRect(0, 0, 144, 144));
    text_layer_set_text(graph_text_layer_populate, outputtext);
    text_layer_set_text_color(graph_text_layer_populate, GColorBlack);
    text_layer_set_background_color(graph_text_layer_populate, GColorWhite);
    text_layer_set_font(graph_text_layer_populate, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_populate, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_populate));
    
    window_set_click_config_provider(populate_window,(ClickConfigProvider)click_config_provider_populate);
}


void populate_unload_window(Window *window)
{
   if(graph_text_layer_populate)
   {
     text_layer_destroy(graph_text_layer_populate);
   }
  window_destroy(populate_window);
  
}

void create_populate_window()
{
   populate_window = window_create();
    	  window_set_window_handlers(populate_window, 
    							   (WindowHandlers){
    									                .load   = populate_load_window,
    								                  .unload = populate_unload_window,
                                     }
    							   );  
    	
  
    	  window_stack_push(populate_window, true);
}

///////////////////// INSULIN //////////////////////
void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {

  // Dummy Function does not do anthing
}



void Set_GraphText_layer_Insulin(TextLayer* currentlayer, bool currentPartSet, int increment)
{
  Set_Part(bIntegerPart_set, increment);
  int temp_integerpart = integerpart;
  static char s_packet_id_text[30];
  
  snprintf(s_packet_id_text, 30, "Insulin: %d.%s units", temp_integerpart, GetFractionaPartAsChar());
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void select_click_handler_insulin(ClickRecognizerRef recognizer, void *context) {
    if(!bIntegerPart_set)
    {
      bIntegerPart_set = true;  
    }
    else
    {
      snprintf(outputtext, 100, "You are adding 'Insulin: %d.%s units'  to Care Portal.", integerpart, GetFractionaPartAsChar());

      snprintf(keyname, 20, "insulin");
      snprintf(resultvalue, 40, "%d.%s", integerpart, GetFractionaPartAsChar());
      create_populate_window();
      bIntegerPart_set = false;
    }
}

static void up_click_handler_insulin(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
	 
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, 1);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_insulin: Exiting###");
}


static void down_click_handler_insulin(ClickRecognizerRef recognizer, void *context) {
   
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, -1);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_insulin: Exiting###");
}

void select_long_click_up_handler_insulin(ClickRecognizerRef recognizer, void *context) {
   
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, 10);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_long_click_up_handler_insulin: Exiting###");
}

void select_long_click_down_handler_insulin(ClickRecognizerRef recognizer, void *context) {
   
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, 0);
  
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_long_click_down_handler_insulin: Exiting###");
}

static void click_config_provider_insulin(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_insulin);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_insulin);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_insulin);
  window_long_click_subscribe(BUTTON_ID_UP, 700, select_long_click_up_handler_insulin, select_long_click_release_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, select_long_click_down_handler_insulin, select_long_click_release_handler);
}

////////////////////// CARBS WINDOW///////////////////////////////////////////////////////////

void Set_GraphText_layer_carbs(TextLayer* currentlayer, int increment)
{
  Set_IntegerPart(increment);
  static char s_packet_id_text[20];
  snprintf(s_packet_id_text, 20, "Carbs: %d g", integerpart);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_carbs(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
  Set_GraphText_layer_carbs(graph_text_layer_carbs, 1);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_carbs: Exiting###");
}

static void select_click_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  snprintf(outputtext, 100, "You are adding 'Carbs: %d g'  to Care Portal.", integerpart);
  snprintf(keyname, 20, "carbs");
  snprintf(resultvalue, 40, "%d", integerpart);
  
  create_populate_window();
}

static void down_click_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_carbs(graph_text_layer_carbs, -1);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_carbs: Exiting###");
}

void select_long_click_up_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_carbs(graph_text_layer_carbs, 10);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_long_click_up_handler_carbs: Exiting###");
}

void select_long_click_down_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_carbs(graph_text_layer_carbs, -10);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_long_click_down_handler_carbs: Exiting###");
}

static void click_config_provider_carbs(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_carbs);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_carbs);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_carbs);
  window_long_click_subscribe(BUTTON_ID_UP, 700, select_long_click_up_handler_carbs, select_long_click_release_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 700, select_long_click_down_handler_carbs, select_long_click_release_handler);
}

void carbs_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(carbs_window);
 
  graph_text_layer_carbs = text_layer_create(GRect(0, 0, 144, 27));
  text_layer_set_text(graph_text_layer_carbs, "Carbs: 0 g");
  text_layer_set_text_color(graph_text_layer_carbs, GColorBlack);
  text_layer_set_background_color(graph_text_layer_carbs, GColorWhite);
  text_layer_set_font(graph_text_layer_carbs, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_carbs, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_carbs));
  
  window_set_click_config_provider(carbs_window,(ClickConfigProvider)click_config_provider_carbs);
}

void carbs_unload_graph(Window *window) {
  
   if(graph_text_layer_carbs)
   {
     text_layer_destroy(graph_text_layer_carbs);
   }
   window_destroy(carbs_window);
}

void insulin_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(insulin_window);

  graph_text_layer_insulin = text_layer_create(GRect(0, 0, 144, 27));
  text_layer_set_text(graph_text_layer_insulin, "Insulin: 0.00 units");
  text_layer_set_text_color(graph_text_layer_insulin, GColorBlack);
  text_layer_set_background_color(graph_text_layer_insulin, GColorWhite);
  text_layer_set_font(graph_text_layer_insulin, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_insulin, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_insulin));
  
  window_set_click_config_provider(insulin_window,(ClickConfigProvider)click_config_provider_insulin);
}

void insulin_unload_graph(Window *window) {
   if(graph_text_layer_insulin)
   {
     text_layer_destroy(graph_text_layer_insulin);
   }
   window_destroy(insulin_window);
}

static void menu_select_callback(int index, void *ctx) {
  
  if(index == 0)
  {
      carbs_window = window_create();
  	  window_set_window_handlers(carbs_window, 
  							   (WindowHandlers){
  									                .load   = carbs_load_graph,
  								                  .unload = carbs_unload_graph,
                                   }
  							   );  
  							  
  	  window_stack_push(carbs_window, true);
  }
  else if(index == 1)
  {
        insulin_window = window_create();
    	  window_set_window_handlers(insulin_window, 
    							   (WindowHandlers){
    									                .load   = insulin_load_graph,
    								                  .unload = insulin_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(insulin_window, true);
  }
}

static void main_window_load(Window *window) {
  int num_a_items = 0;

  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Carbs",
    .callback = menu_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Insulin",
    .callback = menu_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

void main_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image);
}

static void init() 
{
  s_main_window = window_create();
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
   
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "(free: %d, used: %d)",  heap_bytes_free(), heap_bytes_used());
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
