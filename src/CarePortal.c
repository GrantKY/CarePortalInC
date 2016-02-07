#include "pebble.h"
#include "stddef.h"
#include "string.h"
#include "pebble_process_info.h"
extern const PebbleProcessInfo __pbl_app_info;
char version[20];

//https://github.com/pebble-examples/feature-simple-menu-layer
#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 7
#define NUM_SECOND_MENU_ITEMS 1

#define KEY_DATA 5
#define KEY_VALUE 6
#define KEY_EVENT_TYPE 7
#define KEY_EVENTTYPE 8
#define ERROR 9
#define SUCCESS 10
#define DURATION 11
#define PERCENT 12
#define GLUCOSE 13
#define  BG_UNITS 14

#define MMOL_INTEGER_DEFAULT 5
#define MMOL_FRACTIONAL_DEFAULT 6
#define MGDL_DEFAULT 150
#define CARBS_DEFAULT 20

#define COL_DARK PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack)
#define COL_LIGHT PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)

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
static Window *bg_window = NULL;
static Window *combobolus_window = NULL;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
TextLayer *graph_text_layer_carbs = NULL;
TextLayer *graph_text_layer_insulin = NULL;
TextLayer *graph_text_layer_populate = NULL;
TextLayer *graph_text_layer_pumpsitechange = NULL;
TextLayer *graph_text_layer_TempBasal = NULL;
TextLayer *graph_text_layer_uploadresult = NULL;
TextLayer *graph_text_layer_bg = NULL;
TextLayer *graph_text_layer_combobolus = NULL;
char messageresultwindow[100];

char outputtext[150];
char fractionaText[10];

char keyname[20];
char resultvalue[40];
char eventtype[40];
char duration[10];
char percent[10];
char unitsused[10];
char bgresult[10];

char pumpsitechange[50];
int pumpsiteindex = 0;
static char *pumpsitelocations[9] = { "RHS Stomach", "LHS Stomach", "RHS Bottom", "LHS Bottom","RHS Arm", "LHS Arm", "RHS Leg", "LHS Leg", "Other" };

bool bBasalSet = false;
char TempBasal[150];
//int TempBasalindex = 0;
//static char *TempBasallist[7]={ "Plus 10% for 1hr", "Plus 20% for 1hr", "Plus 30% for 1hr", "Plus 40% for 1 hr", "Basal off for 30 mins", "Basal off for 1hr", "other" };
int iTempBasalPercentage = 0;
int iTempBasalMinutes = 0;

// for setting insulin numbers - maybe add _insulin to the end.
int integerpart = 0;
int fractionalpart = 0;
bool bIntegerPart_set = false;  
int insulin_increment = 5;

// BG variables
int integerpart_bg = 0;
int fractionalpart_bg = 0;
bool mmolsunits = true;
// bool bIntegerPart_bg_set = false; - not needed anymore since we're setting from the decimal in mmol

static GBitmap *s_menu_icon_image;


// Combo Bolis
int combo_bolus_insulin_integerpart = 0;
int combo_bolus_insulin_fractionalpart = 0;
int combo_bolus_combo_per = 100;
int combo_bolus_minutes = 0;
int combo_bolus_currentstep = 0; //0 - integer part, 1 - fractional part, 2 - combo bolus percentage, 3 - minutes.

/////////////////////////////////////// ERROR HANDLING ///////////////



static void click_config_provider_uploadresult(void *context) {
  // Register the ClickHandlers
 // window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_populate);
}
void uploadresult_load_window(Window * window)
{
    APP_LOG(APP_LOG_LEVEL_INFO, "uploadresult_load_window called!");
    Layer *window_layer_graph = NULL;
    
    window_layer_graph = window_get_root_layer(uploadresult_window);
  #ifdef PBL_ROUND
      graph_text_layer_uploadresult = text_layer_create(GRect(0, 60, 180, 144));
  #else
    graph_text_layer_uploadresult = text_layer_create(GRect(0, 20, 144, 144));
  #endif
    text_layer_set_text_color(graph_text_layer_uploadresult, COL_DARK);
    text_layer_set_background_color(graph_text_layer_uploadresult, COL_LIGHT);
    text_layer_set_text(graph_text_layer_uploadresult, messageresultwindow);
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
         case BG_UNITS:
         {
           APP_LOG(APP_LOG_LEVEL_INFO, "BG_UNITS: %s", new_tuple->value->cstring);
           snprintf(unitsused, sizeof(unitsused),"%s", new_tuple->value->cstring );
           
           // fixed the mmol to be inline with the mmol for care portal (was mmols)
           char* mmol= "mmol"; 
           if(strstr(unitsused, mmol)!= NULL )
           {
             mmolsunits = true;
             integerpart_bg = MMOL_INTEGER_DEFAULT;
             fractionalpart_bg = MMOL_FRACTIONAL_DEFAULT;
           } 
           else
           {
             mmolsunits = false;
             integerpart_bg = MGDL_DEFAULT;
           }
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

// function used in incrementing the insulin count integer value.
// if the value is less than 0 set it to 0.
int Set_IntegerPart(int increment)
{
    integerpart+= increment;  
    if(integerpart < 0)
    {
      integerpart = 0;
    }
  
    return integerpart;
}

// function used to set the decimal value for the insulin count
// possible todo:  change to similar function as BG for mmol that increments from decimal entirely and not 2 stage
int Set_FractionPart(int increment)
{
  fractionalpart += increment;
  if(fractionalpart < 0)
  {
    fractionalpart = 0;
  }
  else if(fractionalpart >=100)
  {
      fractionalpart = 0;
  }
  return fractionalpart;
}

// function to return the 2 digit decimal as 2 characters if the decimal value is less than 2 digits  (.00, or .05)
char* GetFractionaPartAsChar(int currentvalue) 
{
  if(currentvalue<=5)
  {
      snprintf(fractionaText, sizeof(fractionaText), "0%d", currentvalue);
  }
  else
  {
      snprintf(fractionaText, sizeof(fractionaText), "%d", currentvalue);
  }
  
  return fractionaText; 
}

// determines if the insulin setting is doing the integer section or the decimal section and sets the appropriate value
// possible todo: bring in line with the BG increment from decimal feature
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
         Set_FractionPart(insulin_increment);
      }
      else
      {  
         Set_FractionPart(-insulin_increment);
      }
    }
}

// determines the BG settings for the mg/dl settings.  mmol now uses just the decimal incrementer.
int Set_IntegerPart_BG(int increment)
{
    integerpart_bg+= increment;  
    if(integerpart_bg < 0)
    {
      integerpart_bg = 0;
    }
  
    return integerpart_bg;
}

// incremental change to the decimal section of BG.  Rolls over integer section on boundaries (>9, increment integer, <0 decrement integer if it is not 0)
int Set_FractionPart_BG(int increment)
{
  fractionalpart_bg += increment;
  if(fractionalpart_bg < 0)
  {
      if (integerpart_bg > 0)
      { 
        integerpart_bg--;
        fractionalpart_bg = 9; 
      }
      else 
      {
        fractionalpart_bg = 0; 
      }
  }
  else if(fractionalpart_bg >=10)
  {
    integerpart_bg++;  
    fractionalpart_bg = 0;
  }
   return fractionalpart_bg;
}

// determines if it should be doing the decimal increments for mmol or the integer increment for mg/dl
void Set_Part_BG(bool currentPartSet, int increment) {

  if(mmolsunits)
  {
    Set_FractionPart_BG(increment);
  }
  else  
  {
    Set_IntegerPart_BG(increment);
  }
}

// returns the string of the new site location selection
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
     pumpsiteindex = count - 1;      
   } 
   snprintf(pumpsitechange, sizeof(pumpsitechange), "%s", pumpsitelocations[pumpsiteindex]);
   return pumpsitechange;
}

// return all variables to the initial values 
void ResetToDefaults()
{
  memset(keyname, 0, sizeof(keyname));
  memset(resultvalue, 0, sizeof(resultvalue));
  integerpart = 0;
  fractionalpart = 0;
  bIntegerPart_set = false;
  
  // Temp Basal
  bBasalSet = false;
  iTempBasalPercentage = 0;
  iTempBasalMinutes = 0;
  // reset memory allocation and initialise back to zero length
  memset(duration, 0, sizeof(duration));
  memset(percent, 0, sizeof(percent));
  
  // BG resets
  memset(bgresult, 0, sizeof(bgresult));
   
  // output text reset
  memset(outputtext, 0, sizeof(outputtext));
  
  // set both mmol and mg/dl defaults
  if(mmolsunits)
  {  
    integerpart_bg = MMOL_INTEGER_DEFAULT;
    fractionalpart_bg = MMOL_FRACTIONAL_DEFAULT;  
  }
  else
  {
    integerpart_bg = MGDL_DEFAULT;
  }

  // Combo Bolus
  combo_bolus_insulin_integerpart = 0;
  combo_bolus_insulin_fractionalpart = 0;
  combo_bolus_combo_per = 100;
  combo_bolus_minutes = 0;
  combo_bolus_currentstep = 0; 

}

//////////////////////// POPULATE WINDOW ///////////////////////////////////////
void select_click_handler_populate(ClickRecognizerRef recognizer, void *context) {
   DictionaryIterator *iter;
   AppMessageResult result = app_message_outbox_begin(&iter);
  
   if(result == APP_MSG_OK)
    { 
      if(strlen(keyname) != 0)
      {
          char *const_key;
          const_key = keyname;
          dict_write_cstring(iter, KEY_DATA, const_key);
      }
     
      if(strlen(resultvalue) != 0)
      {
          char *const_result;
          const_result = resultvalue;
          dict_write_cstring(iter, KEY_VALUE, const_result);
      }
      
      if(strlen(eventtype) != 0)
      {
         char *const_eventtype;
         const_eventtype = eventtype;
         dict_write_cstring(iter, KEY_EVENTTYPE, const_eventtype);
      }
     
      if(strlen(duration) != 0)
      {
          char *const_duration;
          const_duration = duration;
          dict_write_cstring(iter, DURATION, const_duration);
      }

      if(strlen(percent) != 0)
      {
          char *const_percent;
          const_percent = percent;
          dict_write_cstring(iter, PERCENT, const_percent);
      }
 
      if(strlen(bgresult) != 0)
      {
          char *const_bgresult;
          const_bgresult = bgresult;
          dict_write_cstring(iter, GLUCOSE, const_bgresult);
      }
 
     
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
  #ifdef PBL_ROUND
    graph_text_layer_populate = text_layer_create(GRect(0, 60, 180, 144));
  #else
    graph_text_layer_populate = text_layer_create(GRect(0, 20, 144, 144));
  #endif
    text_layer_set_text(graph_text_layer_populate, outputtext);
    text_layer_set_text_color(graph_text_layer_populate, COL_DARK);
    text_layer_set_background_color(graph_text_layer_populate, COL_LIGHT);
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
void Set_GraphText_layer_Insulin(TextLayer* currentlayer, bool currentPartSet, int increment)
{
  Set_Part(bIntegerPart_set, increment);
  int temp_integerpart = integerpart;
  static char s_packet_id_text[30];
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin: %d.%s units", temp_integerpart, GetFractionaPartAsChar(fractionalpart));
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void select_click_handler_insulin(ClickRecognizerRef recognizer, void *context) {
    if(!bIntegerPart_set)
    {
      bIntegerPart_set = true;  
    }
    else
    {
      snprintf(outputtext, sizeof(outputtext), "You are adding 'Insulin: %d.%s units'  to Care Portal.", integerpart, GetFractionaPartAsChar(fractionalpart));

      snprintf(keyname, sizeof(keyname), "insulin");
      snprintf(resultvalue, sizeof(resultvalue), "%d.%s", integerpart, GetFractionaPartAsChar(fractionalpart));
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

static void click_config_provider_insulin(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_insulin);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_insulin);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_insulin);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_insulin);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_insulin);
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
  snprintf(outputtext, sizeof(outputtext), "You are adding 'Carbs: %d g'  to Care Portal.", integerpart);
  snprintf(keyname, sizeof(keyname), "carbs");
  snprintf(resultvalue, sizeof(resultvalue), "%d", integerpart);
  snprintf(eventtype,sizeof(eventtype), "Note");

  create_populate_window();
}

static void down_click_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_carbs(graph_text_layer_carbs, -1);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_carbs: Exiting###");
}

static void click_config_provider_carbs(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_carbs);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_carbs);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_carbs);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_carbs);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_carbs);
}

void carbs_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(carbs_window);
 #ifdef PBL_ROUND
  graph_text_layer_carbs = text_layer_create(GRect(0, 75, 180, 37));
 #else
 graph_text_layer_carbs = text_layer_create(GRect(0, 20, 144, 37));
#endif
  text_layer_set_text_color(graph_text_layer_carbs, COL_DARK);
  text_layer_set_text(graph_text_layer_carbs, "Carbs: 0 g");
  text_layer_set_background_color(graph_text_layer_carbs, COL_LIGHT);
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
 #ifdef PBL_ROUND
  graph_text_layer_insulin = text_layer_create(GRect(0, 75, 180, 27));
#else
  graph_text_layer_insulin = text_layer_create(GRect(0, 20, 144, 27));
#endif  
  text_layer_set_text(graph_text_layer_insulin, "Insulin: 0.00 units");
  text_layer_set_text_color(graph_text_layer_insulin, COL_DARK);
  text_layer_set_background_color(graph_text_layer_insulin, COL_LIGHT);
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
}

static void up_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_pumpsitechange: Exiting###");
}

static void select_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) {
    char * sitechange = GetPumpSiteChangeLocation(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_pumpsitechange - Pump Site Location: %s", sitechange);
    snprintf(outputtext, sizeof(outputtext), "You are adding 'Pump Site Location: %s'  to Care Portal.", sitechange);
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
#ifdef PBL_ROUND
  graph_text_layer_pumpsitechange = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_pumpsitechange = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, INITIAL);
  text_layer_set_text_color(graph_text_layer_pumpsitechange, COL_DARK);
  text_layer_set_background_color(graph_text_layer_pumpsitechange, COL_LIGHT);
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

///////////////////// START OF TEMP BASAL ////////////////////////////////////
void SetTempBasalData(int change)
{
    
    if(!bBasalSet)
    {
      iTempBasalPercentage += change;
      if(iTempBasalPercentage < -100)
        iTempBasalPercentage = -100;
      else if (iTempBasalPercentage > 200)
        iTempBasalPercentage = 200;
    }
    else
    {
      iTempBasalMinutes += change;
      if(iTempBasalMinutes < 0)
        iTempBasalMinutes = 0;
    }
}

void Set_GraphText_layer_TempBasal(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];
 
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "TempBasal: %+d%% \n over %d minutes", iTempBasalPercentage, iTempBasalMinutes);

  text_layer_set_text(currentlayer, s_packet_id_text);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_TempBasal: Exiting###");
}

static void up_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
 
  SetTempBasalData(10);  
  
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_TempBasal: Exiting###");
}

static void select_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
  
  
  if(!bBasalSet)
  {
      bBasalSet = true;
  }
  else
  {
      snprintf(outputtext, sizeof(outputtext), "You are adding 'TempBasal' %+d%% over %d minutes to Care Portal.", iTempBasalPercentage, iTempBasalMinutes);
      snprintf(keyname, sizeof(keyname), "notes");
      snprintf(resultvalue, sizeof(resultvalue), "Temp Basal %+d%% over %d minutes.", iTempBasalPercentage, iTempBasalMinutes);
      snprintf(eventtype,sizeof(eventtype), "Temp Basal");
      snprintf(duration,sizeof(duration), "%d",iTempBasalMinutes );
      snprintf(percent,sizeof(percent), "%d", iTempBasalPercentage);
    
      create_populate_window();
  }
}

static void down_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
  SetTempBasalData(-10);  
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_TempBasal: Exiting###");
}


static void click_config_provider_TempBasal(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_TempBasal);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_TempBasal);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_TempBasal);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_TempBasal);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_TempBasal);
}

void tempbasal_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(tempbasal_window);
#ifdef PBL_ROUND
  graph_text_layer_TempBasal = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_TempBasal = text_layer_create(GRect(0, 20, 144, 170));
#endif
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, INITIAL);
  
  text_layer_set_text_color(graph_text_layer_TempBasal, COL_DARK);
  text_layer_set_background_color(graph_text_layer_TempBasal, COL_LIGHT);
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
  
   ResetToDefaults();
   window_destroy(tempbasal_window);
}

///////////////////// END OF Temp Basal CHANGE ///////////////////////////////

///////////////////// Start of glucose //////////////////////

// removed the currentPartSet as we're now setting decimals to start with for mmol
void Set_GraphText_layer_bg(TextLayer* currentlayer, int increment)
{
  Set_Part_BG(bIntegerPart_set, increment);
  int temp_integerpart = integerpart_bg;
  static char s_packet_id_text[30];
  
  if(mmolsunits)
  {
      snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d.%d %s", temp_integerpart, fractionalpart_bg, unitsused);
  }
  else
  {
      snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d %s", temp_integerpart, unitsused);
  }
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void select_click_handler_bg(ClickRecognizerRef recognizer, void *context) {
  if(mmolsunits)
  {
    snprintf(outputtext, sizeof(outputtext), "You are adding 'BG result: %d.%d %s'  to Care Portal.", integerpart_bg, fractionalpart_bg, unitsused);
    snprintf(bgresult, sizeof(bgresult), "%d.%d", integerpart_bg,fractionalpart_bg );
  }
  else
  {
    snprintf(outputtext, sizeof(outputtext), "You are adding 'BG result: %d %s'  to Care Portal.", integerpart_bg,unitsused);
    snprintf(bgresult, sizeof(bgresult), "%d", integerpart_bg);
  }
  snprintf(eventtype,sizeof(eventtype), "BG Check");
  create_populate_window();
}

static void up_click_handler_bg(ClickRecognizerRef recognizer, void *context) { //I WOULD LIKE THE UP BUTTON PRESS TO GO TO A WINDOW CALLED WINDOW_GRAPH
	  Set_GraphText_layer_bg(graph_text_layer_bg, UP);
    // app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_bg: Exiting###");
}

static void down_click_handler_bg(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_bg(graph_text_layer_bg, DOWN);
    // app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_bg: Exiting###");
}

static void click_config_provider_bg(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_bg);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_bg);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_bg);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_bg);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_bg);
}
void bg_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(bg_window);
#ifdef PBL_ROUND
  graph_text_layer_bg = text_layer_create(GRect(0, 75, 180, 27));
#else
  graph_text_layer_bg = text_layer_create(GRect(0, 20, 144, 27));
#endif
  if(mmolsunits)
  {
     snprintf(outputtext, sizeof(outputtext), "BG: %d.%d %s", integerpart_bg, fractionalpart_bg, unitsused);
  }
  else
  {
      snprintf(outputtext, sizeof(outputtext), "BG: %d %s", integerpart_bg, unitsused);
  }

  text_layer_set_text(graph_text_layer_bg, outputtext);
  text_layer_set_text_color(graph_text_layer_bg, COL_DARK);
  text_layer_set_background_color(graph_text_layer_bg, COL_LIGHT);
  text_layer_set_font(graph_text_layer_bg, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_bg, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_bg));
  
  window_set_click_config_provider(bg_window,(ClickConfigProvider)click_config_provider_bg);
}

void bg_unload_graph(Window *window) {
   if(graph_text_layer_bg)
   {
     text_layer_destroy(graph_text_layer_bg);
   }
   window_destroy(bg_window);
}
////////////end of bg mmol window///////////////

/////////// COMBO BOLUS WINDOW///////////////
void UpdateComboBolusDetails(int change)
{
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails: %d", combo_bolus_currentstep);
    if(combo_bolus_currentstep == 0)
    {
        app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_insulin_integerpart: %d", combo_bolus_insulin_integerpart);
        combo_bolus_insulin_integerpart += change;
    }
    else if(combo_bolus_currentstep == 1)
    {
        app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_insulin_combo_bolus_insulin_fractionalintegerpart: %d", combo_bolus_insulin_fractionalpart);
        
        if(change == UP)
        {
            combo_bolus_insulin_fractionalpart += insulin_increment;
        }
        else
        {
            combo_bolus_insulin_fractionalpart -= insulin_increment;
        }

        if(combo_bolus_insulin_fractionalpart < 0)
        {
            combo_bolus_insulin_fractionalpart = 0;
        }
        else if(combo_bolus_insulin_fractionalpart >=100)
        {
            combo_bolus_insulin_fractionalpart = 0;
        }
    }
    else if(combo_bolus_currentstep == 2)
    {
        int increment = 5;
        if(combo_bolus_combo_per != 100 && combo_bolus_combo_per != 0)
        {
            if(change == UP)
            {
                combo_bolus_combo_per += increment;
            }
            else if(change == DOWN)
            {
                combo_bolus_combo_per -= increment;
            }

        }
        else if(combo_bolus_combo_per == 100)
        {
            if(change == DOWN)
            {
                combo_bolus_combo_per -= increment;
            }
            else
            {
                app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_combo_per > 100 not allowed");
            }
        }
        else if(combo_bolus_combo_per == 0)
        {
            if(change == UP)
            {
                 combo_bolus_combo_per += increment;
            }
            else
            {
                app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_combo_per < 0 not allowed");

            }
        }
        

    }
   else if(combo_bolus_currentstep == 3)
    {
        int timestepincrement = 10;
        if(combo_bolus_minutes != 0 )
        {
            if(change == UP)
            {
                combo_bolus_minutes += timestepincrement;
            }
            else if(change == DOWN)
            {
                combo_bolus_minutes -= timestepincrement;
            }

        }
        else if(combo_bolus_minutes == 0)
        {
            if(change == UP)
            {
                combo_bolus_minutes += timestepincrement;
            }
            else if(change == DOWN)
            {
               app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_minutes < 0 not allowed");
            }
        }

        
    }
}




void Set_GraphText_layer_combobolus(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[60];
  UpdateComboBolusDetails(change);
  //char * sitechange = GetPumpSiteChangeLocation(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin: %d.%s units.\nCombo bolus %d/%d\n over\n %d minutes",
            combo_bolus_insulin_integerpart, GetFractionaPartAsChar(combo_bolus_insulin_fractionalpart), combo_bolus_combo_per, 100 - combo_bolus_combo_per,combo_bolus_minutes );
 // app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "Pump Site Location: %s", sitechange);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_combobolus(graph_text_layer_combobolus, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_combobolus: Exiting###");
}

static void select_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
   //char * sitechange = GetPumpSiteChangeLocation(INITIAL);

    if(combo_bolus_currentstep < 4)
    {
        combo_bolus_currentstep += 1;
    }
    else
    {
        app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_combobolus");
        snprintf(outputtext, sizeof(outputtext), "You are adding Insulin: %d.%s units.\nCombo bolus %d/%d\n over\n %d minutes",
                combo_bolus_insulin_integerpart, GetFractionaPartAsChar(combo_bolus_insulin_fractionalpart), combo_bolus_combo_per, 
                100 - combo_bolus_combo_per,combo_bolus_minutes );

        //snprintf(keyname, sizeof(keyname), "notes");
       //snprintf(resultvalue, sizeof(resultvalue), "%s", sitechange);
       //snprintf(eventtype,sizeof(eventtype), "Site Change");
        create_populate_window();
        combo_bolus_currentstep = 0;
  }


}

static void down_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_combobolus(graph_text_layer_combobolus,DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_combobolus: Exiting###");
}


static void click_config_provider_combobolus(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_combobolus);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_combobolus);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_combobolus);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_combobolus);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_combobolus);
}

void combobolus_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(combobolus_window);
#ifdef PBL_ROUND
  graph_text_layer_combobolus = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_combobolus = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_combobolus(graph_text_layer_combobolus, INITIAL);
  text_layer_set_text_color(graph_text_layer_combobolus, COL_DARK);
  text_layer_set_background_color(graph_text_layer_combobolus, COL_LIGHT);
  text_layer_set_font(graph_text_layer_combobolus, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_combobolus, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_combobolus));
  
  window_set_click_config_provider(combobolus_window,(ClickConfigProvider)click_config_provider_combobolus);
 
}

void combobolus_unload_graph(Window *window) {
   if(graph_text_layer_combobolus)
   {
     text_layer_destroy(graph_text_layer_combobolus);
   }
  
   window_destroy(combobolus_window);
}
/////////////////////// END 


////////////End of Combo Bolus Window///////////////

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
        bg_window = window_create();
    	  window_set_window_handlers(bg_window, 
    							   (WindowHandlers){
    									                .load   = bg_load_graph,
    								                  .unload = bg_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(bg_window, true);
  }
  else if(index == 4)
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
  else if(index == 5)
    {
        combobolus_window = window_create();
    	  window_set_window_handlers(combobolus_window, 
    							   (WindowHandlers){
    									                .load   = combobolus_load_graph,
    								                  .unload = combobolus_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(combobolus_window, true);

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
    .title = "Blood Glucose",
    .callback = menu_select_callback,
  };
   s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Pump Site Change",
    .callback = menu_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Combo Bolus",
    .callback = menu_select_callback,
  };
 
  snprintf(version, sizeof(version), "Version %d.%d", __pbl_app_info.process_version.major, __pbl_app_info.process_version.minor);
 
  
   s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = version,
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
