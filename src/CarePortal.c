#include "pebble.h"
#include "stddef.h"
#include "string.h"
//https://github.com/pebble-examples/feature-simple-menu-layer
#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 4
#define NUM_SECOND_MENU_ITEMS 1

#define KEY_DATA 5
#define KEY_VALUE 6
#define KEY_EVENT_TYPE 7
#define KEY_EVENTTYPE 8
#define ERROR 9
#define SUCCESS 10

#define UP 1
#define DOWN -1
#define INITIAL 0

static Window *s_main_window = NULL;
static Window *carbs_window = NULL;
static Window *insulin_window = NULL;
static Window *populate_window = NULL;
static Window *pumpsitechange_window = NULL;
static Window *tempbasal_window = NULL;
static Window *uploadresult_window = NULL;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
TextLayer *graph_text_layer_carbs = NULL;
TextLayer *graph_text_layer_insulin = NULL;
TextLayer *graph_text_layer_populate = NULL;
TextLayer *graph_text_layer_pumpsitechange = NULL;
TextLayer *graph_text_layer_TempBasal = NULL;
TextLayer *graph_text_layer_uploadresult = NULL;


char messageresultwindow[100];

char outputtext[100];
char fractionaText[10];

char keyname[20];
char resultvalue[40];
char eventtype[40];

char pumpsitechange[50];
int pumpsiteindex = 0;
static char *pumpsitelocations[9] = { "RHS Stomach", "LHS Stomach", "RHS Bottom", "LHS Bottom","RHS Arm", "LHS Arm", "RHS Leg", "LHS Leg", "Other" };

char TempBasal[50];
int TempBasalindex = 0;
static char *TempBasallist[7]={ "Plus 10% for 1hr", "Plus 20% for 1hr", "Plus 30% for 1hr", "Plus 40% for 1 hr", "Basal off for 30 mins", "Basal off for 1hr", "other" };


char* GetTempBasalLocation(int change)
{
      TempBasalindex += change;
      
      int count = sizeof(TempBasallist)/sizeof(*TempBasallist);
      app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetTempBasalLocation: count: %d ###", count);
      if(TempBasalindex >= count)
      {
          TempBasalindex = 0;
      }
      else if(TempBasalindex < 0)
      {
          TempBasalindex = 0;      
      } 
        
      snprintf(TempBasal, sizeof(TempBasal), "%s", TempBasallist[TempBasalindex]);
      return TempBasal;
}

/////////////////////////////////////// ERROR HANDLING ///////////////



static void click_config_provider_uploadresult(void *context) {
  // Register the ClickHandlers
 // window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_populate);
}
void uploadresult_load_window(Window * window)
{
    APP_LOG(APP_LOG_LEVEL_INFO, "error_load_window called!");
    Layer *window_layer_graph = NULL;
    
    window_layer_graph = window_get_root_layer(uploadresult_window);
    graph_text_layer_uploadresult = text_layer_create(GRect(0, 0, 144, 144));
    text_layer_set_text(graph_text_layer_uploadresult, messageresultwindow);
    text_layer_set_text_color(graph_text_layer_uploadresult, GColorBlack);
    text_layer_set_background_color(graph_text_layer_uploadresult, GColorWhite);
    text_layer_set_font(graph_text_layer_uploadresult, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_uploadresult, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_uploadresult));
    
    window_set_click_config_provider(uploadresult_window,(ClickConfigProvider)click_config_provider_uploadresult);
}


void uploadresult_unload_window(Window *window)
{
   if(graph_text_layer_uploadresult)
   {
     text_layer_destroy(graph_text_layer_uploadresult);
   }
  window_destroy(uploadresult_window);
  
}

void create_uploadresult_window()
{
    uploadresult_window = window_create();
    window_set_window_handlers(uploadresult_window, 
                               (WindowHandlers){
                                 .load   = uploadresult_load_window,
                                 .unload = uploadresult_unload_window,
                               }
                              );  
  
  
    window_stack_push(uploadresult_window, true);
  
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback called!");

  // Get the first pair
  Tuple *new_tuple = dict_read_first(iterator);


  // Process all pairs present
  while(new_tuple != NULL)
  {
       switch (new_tuple->key) {
  
        case ERROR:
        {
            APP_LOG(APP_LOG_LEVEL_INFO, "Error Message: %s", new_tuple->value->cstring);
             
            snprintf(messageresultwindow, sizeof(messageresultwindow), "Error uploading. Please check connection.");
            create_uploadresult_window();
         }
         break;
        case SUCCESS:
         {
           
            APP_LOG(APP_LOG_LEVEL_INFO, "SUCCESSFUL: %s", new_tuple->value->cstring);
            snprintf(messageresultwindow, sizeof(messageresultwindow), "Success uploading to website.");
            create_uploadresult_window();
         }
        break;
    }

    // Get next pair, if any
    new_tuple = dict_read_next(iterator);
  }
 }

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "inbox_dropped_callback called!");  
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
   APP_LOG(APP_LOG_LEVEL_INFO, "outbox_failed_callback called!"); 
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback called!");
}

/////////////////////////////////////////////////////////////////////////////////////








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
      snprintf(fractionaText, sizeof(fractionaText), "0%d", fractionalpart);
  }
  else
  {
      snprintf(fractionaText, sizeof(fractionaText), "%d", fractionalpart);
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
 char* GetPumpSiteChangeLocation(int change)
{
   pumpsiteindex += change;
  
   int count = sizeof(pumpsitelocations)/sizeof(*pumpsitelocations);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetPumpSiteChangeLocation: count: %d ###", count);
   if(pumpsiteindex >= count)
   {
     pumpsiteindex = 0;
   }
   else if(pumpsiteindex < 0)
   {
     pumpsiteindex = 0;      
   } 
   snprintf(pumpsitechange, sizeof(pumpsitechange), "%s", pumpsitelocations[pumpsiteindex]);
   return pumpsitechange;
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
      
      char *const_eventtype;
      const_eventtype = eventtype;
     
     
      app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: const_key: %s ###", const_key);
      app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: const_result: %s ###", const_result);
      app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: const_eventtype: %s ###", const_eventtype);
   
     
      dict_write_cstring(iter, KEY_DATA, const_key);
      dict_write_cstring(iter, KEY_VALUE, const_result);
      dict_write_cstring(iter, KEY_EVENTTYPE, const_eventtype);
      
 
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
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin: %d.%s units", temp_integerpart, GetFractionaPartAsChar());
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

      snprintf(keyname, sizeof(keyname), "insulin");
      snprintf(resultvalue, sizeof(resultvalue), "%d.%s", integerpart, GetFractionaPartAsChar());
      snprintf(eventtype,sizeof(eventtype), "Note");
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
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Carbs: %d g", integerpart);
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
  snprintf(eventtype,sizeof(eventtype), "Note");

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

//////// PUMP SITE CHANGE ////////////////////////////////////////////////////////////////
void Set_GraphText_layer_pumpsitechange(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * sitechange = GetPumpSiteChangeLocation(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Pump Site Location: %s", sitechange);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "Pump Site Location: %s", sitechange);
  text_layer_set_text(currentlayer, s_packet_id_text);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_pumpsitechange: Exiting###");
}

static void up_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_pumpsitechange: Exiting###");
}

static void select_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) {
  
    char * sitechange = GetPumpSiteChangeLocation(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_pumpsitechange - Pump Site Location: %s", sitechange);
    snprintf(outputtext, 100, "You are adding 'Pump Site Location: %s'  to Care Portal.", sitechange);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "%s", sitechange);
    snprintf(eventtype,sizeof(eventtype), "Site Change");
  
    create_populate_window();
}

static void down_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_pumpsitechange: Exiting###");
}


static void click_config_provider_pumpsitechange(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_pumpsitechange);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_pumpsitechange);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_pumpsitechange);
}

void pumpsitechange_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(pumpsitechange_window);

  graph_text_layer_pumpsitechange = text_layer_create(GRect(0, 0, 144, 170));
 
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, INITIAL);
   
  //snprintf(pumpsitechange, 50, "Pump Site Location: %s", GetPumpSiteChangeLocation());

 // Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange);
  //text_layer_set_text(graph_text_layer_pumpsitechange,  "Pump Site Location: RHS tummy");
  text_layer_set_text_color(graph_text_layer_pumpsitechange, GColorBlack);
  text_layer_set_background_color(graph_text_layer_pumpsitechange, GColorWhite);
  text_layer_set_font(graph_text_layer_pumpsitechange, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_pumpsitechange, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_pumpsitechange));
  
  window_set_click_config_provider(pumpsitechange_window,(ClickConfigProvider)click_config_provider_pumpsitechange);
}

void pumpsitechange_unload_graph(Window *window) {
   if(graph_text_layer_pumpsitechange)
   {
     text_layer_destroy(graph_text_layer_pumpsitechange);
   }
   pumpsiteindex =0;
   window_destroy(pumpsitechange_window);
}
/////////////////////// END OF PUMP SITE CHANGE ///////////////////////////////
//////// TEMP BASAL ////////////////////////////////////////////////////////////////
void Set_GraphText_layer_TempBasal(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * basalchange = GetTempBasalLocation(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "TempBasal: %s", basalchange);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "TempBasal: %s", basalchange);
  text_layer_set_text(currentlayer, s_packet_id_text);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_TempBasal: Exiting###");
}

static void up_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_TempBasal: Exiting###");
}

static void select_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
  
    char * basalchange = GetTempBasalLocation(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_TempBasal - TempBasal: %s", basalchange);
    snprintf(outputtext, 100, "You are adding 'TempBasal: %s'  to Care Portal.", basalchange);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "%s", basalchange);
    snprintf(eventtype,sizeof(eventtype), "Temp Basal");
  
    create_populate_window();
}

static void down_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_TempBasal: Exiting###");
}


static void click_config_provider_TempBasal(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_TempBasal);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_TempBasal);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_TempBasal);
}

void tempbasal_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(tempbasal_window);

  graph_text_layer_TempBasal = text_layer_create(GRect(0, 0, 144, 170));
 
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, INITIAL);
   
  //snprintf(pumpsitechange, 50, "Pump Site Location: %s", GetPumpSiteChangeLocation());

 // Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal);
  //text_layer_set_text(graph_text_layer_TempBasal,  "Pump Site Location: RHS tummy");
  text_layer_set_text_color(graph_text_layer_TempBasal, GColorBlack);
  text_layer_set_background_color(graph_text_layer_TempBasal, GColorWhite);
  text_layer_set_font(graph_text_layer_TempBasal, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_TempBasal, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_TempBasal));
  
  window_set_click_config_provider(tempbasal_window,(ClickConfigProvider)click_config_provider_TempBasal);
}

void tempbasal_unload_graph(Window *window) {
   if(graph_text_layer_TempBasal)
   {
     text_layer_destroy(graph_text_layer_TempBasal);
   }
   TempBasalindex = 0;
   window_destroy(tempbasal_window);
}
/////////////////////// END OF Temp Basal CHANGE ///////////////////////////////
///////////////////////////// MAIN WINDOW /////////////////////////////////
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
  else if(index == 2)
  {
        tempbasal_window = window_create();
    	  window_set_window_handlers(tempbasal_window, 
    							   (WindowHandlers){
    									                .load   = tempbasal_load_graph,
    								                  .unload = tempbasal_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(tempbasal_window, true);
  } 
  else if(index == 3)
  {
        pumpsitechange_window = window_create();
    	  window_set_window_handlers(pumpsitechange_window, 
    							   (WindowHandlers){
    									                .load   = pumpsitechange_load_graph,
    								                  .unload = pumpsitechange_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(pumpsitechange_window, true);
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
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Temp Basal",
    .callback = menu_select_callback,
  };
   s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Pump Site Change",
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
////////////////////////////// END OF MAIN WINDOW //////////////////////////////////////////
static void init() 
{
  s_main_window = window_create();
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
   
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  
  // Registering callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
  
  
  
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
