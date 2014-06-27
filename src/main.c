#include <pebble.h>
#include <pebble_fonts.h>
#include <string.h>

/*
*
*  Woolly Watchface v1      
*  - by james@5point6.net, June 2014
*
*  A plain english text based watch face for Pebble using fuzzy time.
*  Also include wrist-twist gesture to show date and actual time.
*
*  e.g. "14:44" appears as "quarter to three"
*
*
*
*  Background:
*  Originally produced as a hobby 'toy' project loosely based on the Fuzzy 
*  time example shipped with the pebble sdk and also some inspiration from 
*  Jesse Hallet's excellent Fuzzy Text International Watch face app that 
*  I used to use before building my own! 
*
*  Written from scratch though I may have borrowed some code/ideas from those 
*  projects... this is my first C program in over 10 years!
* 
*  Jesse's app: https://github.com/hallettj/Fuzzy-Text-International
*
*/


#define REALTIME_Y 146
#define REALTIME_Y_OFF 170  
#define BUFFER_SIZE 10

//change to SECOND_MINUTE when testing  
#define TICKCHECK MINUTE_UNIT

#define LINE_Y_GAP 35
#define HALF_LINE_Y_GAP 17  
#define LINE_Y_TOP -12 


/* text Line definition */
typedef struct {
	TextLayer *currentLayer;
	TextLayer *nextLayer;
	char thisStr[BUFFER_SIZE];
	char prevStr[BUFFER_SIZE];
} Line;  

/* global vars definition */
static struct CommonWordsData {
  Window *window;     //the main window
  Line lines[4];      //The individual lines of text                                                  
  TextLayer *realTime;      //the label container for the accurate time/date
  char realTimeBuffer[30];  //a re-usable buffer for date manipulation
  bool realVisible;         //flag indicating if real time is visible
  int lastHour;      //the last hour displayed
  int lastMin;       //the last minute displayed
} data;

/* woolly time definitions */
static const char* const TIMES[][4] = {
  {"HOUR","o'clock","",""},
  {"five","past","HOUR",""},
  {"ten","past","HOUR",""},
  {"quarter","past","HOUR",""},
  {"twenty","past","HOUR",""},
  {"twenty","five","past","HOUR"},
  {"half","past","HOUR",""},
  {"twenty","five to","HOUR",""},
  {"twenty","to","HOUR",""},
  {"quarter","to","HOUR",""},
  {"ten to","HOUR","",""},
  {"five to","HOUR","",""}
};

/* hours in english */
static const char* const HOURS[13] = { 
  "twelve","one","two","three","four","five","six","seven","eight","nine","ten","eleven","twelve"
};


/* ANIMATIONS ------------------------------------------------------------------- */

/* animation cleanup */
void on_animation_stopped(Animation *anim, bool finished, void *context)
{
  //free anim memory
  property_animation_destroy((PropertyAnimation *) anim);

}

/* generic animation handler */
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay) {
  //declare anim
  PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);

  //set characteristics
  animation_set_duration((Animation*) anim, duration);
  animation_set_delay((Animation*) anim, delay);

  //set stopped handler to free memory
  AnimationHandlers handlers = {
	.stopped = (AnimationStoppedHandler) on_animation_stopped
  };
  animation_set_handlers((Animation*) anim, handlers, layer);

  //start anim
  animation_schedule((Animation*) anim);
}

/* REAL TIME ------------------------------------------------------------------- */

/* display the real time, animated */
void show_realtime(){
  GRect start= layer_get_frame(text_layer_get_layer(data.realTime));
  GRect finish=GRect(start.origin.x,REALTIME_Y,start.size.w,start.size.h);
  animate_layer(text_layer_get_layer(data.realTime),&start, &finish, 400, 100);
}

/* hide the real time, animated */
void hide_realtime(){
  GRect start= layer_get_frame(text_layer_get_layer(data.realTime));
  GRect finish=GRect(start.origin.x,REALTIME_Y_OFF,start.size.w,start.size.h);
  animate_layer(text_layer_get_layer(data.realTime),&start, &finish, 400, 0);
}


/* TAP LISTENER ------------------------------------------------------------------- */

void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  //toggle real time visibility
  if(data.realVisible) {
	//hide time
   hide_realtime();
   data.realVisible=0;
  }  else {
	//show time
	show_realtime();
	data.realVisible=1;
  }  
}



/* TIME DISPLAY ------------------------------------------------------------------- */

/* move the lables around on an update */
void position_label(int line_num, int total_lines, int boldLine, bool firstRun) {

	//some setup to control positioiing of the labels
	int y=LINE_Y_TOP; 
	const int y_gap = LINE_Y_GAP;
	const int half_y_gap = HALF_LINE_Y_GAP;

	//spread them out evenly vertically
	y=y+(line_num*y_gap); 

	//adjust positioning a little based on number of total lines that have text
	if(total_lines == 1 ) {
	  y=y+(y_gap*1)+half_y_gap;
	} 
	if(total_lines ==2 ) {
	  y=y+(y_gap*1)+half_y_gap;
	}
	if(total_lines ==3 ) {
	  y=y+(y_gap*1);
	} 

  //On the first run we just show the time, no need to animate or do anything too funky
  if(!firstRun) {
	// This is not the first run, so animate transition between times


	// Current layer is the one currently in view and needs sliding out
	TextLayer *currentLine=data.lines[line_num].currentLayer;
	text_layer_set_text( currentLine, data.lines[line_num].prevStr);  

	//Slide out current layer off the left of window   
	GRect startOUT= layer_get_frame(text_layer_get_layer(currentLine));
	GRect finishOUT=GRect(-144,startOUT.origin.y,startOUT.size.w,startOUT.size.h);
	animate_layer(text_layer_get_layer(currentLine),&startOUT, &finishOUT, 200, 300+(line_num*100));



	//slide in the next layer
	TextLayer *nextLine=data.lines[line_num].nextLayer;

	//slide in
	GRect startIN= layer_get_frame(text_layer_get_layer(nextLine));
	startIN.origin.x=144;
	startIN.origin.y=y;  
	GRect finishIN=GRect(0,y,startIN.size.w,startIN.size.h);

	//set the format
	if(boldLine==line_num) {
	  text_layer_set_font(nextLine, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	} else {
	  text_layer_set_font(nextLine, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	}

	text_layer_set_text( nextLine, data.lines[line_num].thisStr);
	animate_layer(text_layer_get_layer(nextLine),&startIN, &finishIN, 200, 300+(line_num*100));


	//swap the pointers so that on next iteration we use the correct text layers
	TextLayer *swp = data.lines[line_num].nextLayer;
	data.lines[line_num].nextLayer = data.lines[line_num].currentLayer;
	data.lines[line_num].currentLayer = swp;

  } else {
	//This is first run so no need to animate, otherwise as above

	TextLayer *currentLine=data.lines[line_num].currentLayer;
	text_layer_set_text( currentLine, data.lines[line_num].thisStr);  

	GRect position= layer_get_frame(text_layer_get_layer(currentLine));
	position.origin.y=y;
	layer_set_frame(text_layer_get_layer(currentLine), position);

	if(boldLine==line_num) {
	  text_layer_set_font(currentLine, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	} else {
	  text_layer_set_font(currentLine, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	}
  }

}


/* Change the time, Work out the text to display */
void set_time_labels(int timeIndex, int hour, bool firstRun) {
  int boldLine=0;
  for(int i=0; i < 4; i++) {  
	  //remember the previous time
	  strcpy(data.lines[i].prevStr,data.lines[i].thisStr);

    //construct new time labels from available strings.
  	if(strncmp(TIMES[timeIndex][i], "HOUR", 4)==0) {
  		  strcpy(data.lines[i].thisStr,(char *) HOURS[hour]);
  		  boldLine=i;
  	} else {
  		  strcpy(data.lines[i].thisStr,(char *) TIMES[timeIndex][i]);
  	}
  }

   //caclulate the number of lines that have some text
  int numLines=0;
  for(int i=0; i < 4; i++) {  
	  if(strlen(data.lines[i].thisStr) != 0) { numLines++; }
  }

  //re-position lines as required
  for(int i=0; i < 4; i++) {  
	  position_label(i,numLines,boldLine, firstRun);
  }
  
  hide_realtime(); // always try to hide the real time on an update
}


/* WOOLLY TIME! ------------------------------------------------------------------- */

/* determine the woolly time */
void woolly_time_to_words(int hours, int minutes, bool firstRun) {
  int woolly_hours=hours;
  int woolly_minutes=((minutes+2)/5) * 5; //round to nearest five

  //check if we're approaching the next hour or past the current hour
  if(woolly_minutes > 55) {
	  woolly_minutes=0;
	  woolly_hours += 1;
  } else if(woolly_minutes > 30) {
	  woolly_hours+=1;
  }

  int timeIndex=-1;

  //deal with roll over midnight hours
  if(woolly_hours > 23) {
	woolly_hours=0;
  }

  //convert hours to twelve hour clock
  if(woolly_hours > 12 ) {
	  woolly_hours=woolly_hours-12;
  }

  //get the minutes words
  if(woolly_minutes==0) { timeIndex=0;}
  if(woolly_minutes==5) { timeIndex=1;}
  if(woolly_minutes==10) { timeIndex=2;}
  if(woolly_minutes==15) { timeIndex=3;}
  if(woolly_minutes==20) { timeIndex=4;}
  if(woolly_minutes==25) { timeIndex=5;}
  if(woolly_minutes==30) { timeIndex=6;}
  if(woolly_minutes==35) { timeIndex=7;}
  if(woolly_minutes==40) { timeIndex=8;}
  if(woolly_minutes==45) { timeIndex=9;}
  if(woolly_minutes==50) { timeIndex=10;}
  if(woolly_minutes==55) { timeIndex=11;}

  //only update if the time is new
  if(timeIndex >= 0 && (data.lastMin!=woolly_minutes || data.lastHour!=woolly_hours)){
	  set_time_labels(timeIndex,woolly_hours,firstRun);
	  data.lastMin=woolly_minutes;
	  data.lastHour=woolly_hours;
  } 


}

/* REAL TIME  ------------------------------------------------------------------- */

/* set the real time text layer */
void update_real_time(struct tm *tick_time) {
  memset(data.realTimeBuffer, 0, 30);
  strftime(data.realTimeBuffer, 30, "%a %e %b      %H:%M", tick_time);
  text_layer_set_text( data.realTime, data.realTimeBuffer);
}



/* TIME CHANGE HANDLERS  ------------------------------------------------------------------- */

/* time update handler */
static void update_time(struct tm *t, bool firstRun) {
  /* 

  Test mode
  Set  TICKCHECK to SECOND_UNIT and uncomment this code block to get updates every 5 seconds.
  
  */

  /* being test chunk */
  /*
  if(t->tm_sec == 0) {
	woolly_time_to_words(12, 0, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 55 == 0) {
	woolly_time_to_words(1, 55, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 50 == 0) {
	woolly_time_to_words(2, 50, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 45 == 0) {
	woolly_time_to_words(3, 45, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 40 == 0) {
	woolly_time_to_words(4, 40, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 35 == 0) {
	woolly_time_to_words(5, 35, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 30 == 0) {
	woolly_time_to_words(6, 30, 0);  
	update_real_time(t);
  } else   if(t->tm_sec % 25 == 0) {
	woolly_time_to_words(7, 25, 0);  
	update_real_time(t);
  } else   if(t->tm_sec % 20 == 0) {
	woolly_time_to_words(8, 20, 0);  
	update_real_time(t);
  } else  if(t->tm_sec % 15 == 0) {
	woolly_time_to_words(9, 15, 0);  
	update_real_time(t);
  } else if(t->tm_sec % 10 == 0) {
	woolly_time_to_words(10, 10, 0); 
	update_real_time(t);
  } else if(t->tm_sec % 5 == 0) {
	woolly_time_to_words(11, 5, 0);  
	update_real_time(t);
  }
  */
  /* end test chunk */


  /* live code use current time */

  woolly_time_to_words(t->tm_hour, t->tm_min, firstRun);              
  update_real_time(t);
}

/* pass through for the handler above */
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time(tick_time,0);
}

/* SETUP  ------------------------------------------------------------------- */

/* initialise a time line */
TextLayer* init_label(GRect position) {
  TextLayer* label=text_layer_create(position); 
  text_layer_set_background_color(label, GColorClear);
  text_layer_set_text_color(label,GColorWhite);
  text_layer_set_font(label, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_alignment(label, GTextAlignmentLeft );
  return label; 
}

/* create a new time line and add to the window */
void setup_line(Window *window, int line_number, GRect position) {
  data.lines[line_number].currentLayer=init_label(position);
  layer_add_child(window_get_root_layer(window),(Layer*) data.lines[line_number].currentLayer);
  
  //the next layer gets inititalised off screen
  position.origin.x=144;
  data.lines[line_number].nextLayer=init_label(position);
  layer_add_child(window_get_root_layer(window),(Layer*) data.lines[line_number].nextLayer);

}


/* load handler */
void window_load(Window *window)
{
  data.realVisible=0; //we dont show the real time by default
  window_set_background_color(window, GColorBlack);

  //create woolly time layers
  for (int i=0; i < 4; i++) {
  	setup_line(window,i,GRect(0,LINE_Y_TOP+(LINE_Y_GAP*i),144,60));
  }

  //setup the 'real' time layers
  data.realTime = text_layer_create(GRect(0,REALTIME_Y_OFF,144,60));
  text_layer_set_background_color(data.realTime, GColorClear);
  text_layer_set_text_color(data.realTime,GColorWhite);
  text_layer_set_font(data.realTime, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(data.realTime, GTextAlignmentCenter );
  layer_add_child(window_get_root_layer(window),(Layer*) data.realTime); 

   //set the time on first load
  struct tm *t;
  time_t temp;
  temp=time(NULL);
  t=localtime(&temp);
  update_time(t,1); //the 1 here means its a first run
}

/* destroy stuff and unload from memory */
void window_unload(Window *window)
{
  //destroys window stuff here
  text_layer_destroy(data.lines[0].currentLayer);
  text_layer_destroy(data.lines[1].currentLayer);
  text_layer_destroy(data.lines[2].currentLayer);
  text_layer_destroy(data.lines[3].currentLayer);
  text_layer_destroy(data.realTime);
} 

void init()
{
  //initialise app elements here
  data.window=window_create();
  window_set_window_handlers(data.window, (WindowHandlers) {
	  .load = window_load,
	  .unload = window_unload,
  });
  window_stack_push(data.window,true);
  tick_timer_service_subscribe(TICKCHECK, (TickHandler) tick_handler);
  accel_tap_service_subscribe(&accel_tap_handler);

}

void deinit()
{
  //de init here
  window_destroy(data.window);
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();

}


int main(void)
{
  init();
  app_event_loop();
  deinit();
}