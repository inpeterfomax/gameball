#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "./bitmaps/icon"
#include "./bitmaps/logo"
#include "./bitmaps/leftarrow"
#include "./bitmaps/rightarrow"


/* Macros */

#define pos 1 /* vectors for ball coordinates */
#define neg 0


/* Global Variables */

Pixmap icon_pixmap;     /* Pixel Maps */
Pixmap logo_pixmap;
Pixmap left_arrow_pixmap;
Pixmap right_arrow_pixmap;
Pixmap box; /* Pixel Map Used as graphics buffer */
Screen *scn; /* Data on physical screen */
char *window_name = "Game Ball";
char *icon_name = "Game Ball";
int display_width;
int display_height;
XTextProperty windowname, iconname; /* X11 Window Data */
XWMHints wmhints;
XClassHint classhints;
XSizeHints sizehints;
XEvent event;   /* X11 Event Data */     
GC gcontext;   /* Black foreground Graphics Context */
GC gcontext_white; /* White foreground Graphics Context */
XGCValues gcvalues;
XGCValues gcvalues_white;
Window mainwindow;
XWindowAttributes mainwindowattributes;
int main_window_width;
int main_window_height;
Display *display;
Font text_font;
int box_width;  /* Coordinates of play box */ 
int box_height;
int box_x;
int box_y;
int ball_x;       /*Ball coordinates*/
int ball_y;
int ball_width = 25;
int ball_speed = 3;
int paddle_x;                    /* Paddle Location */
int paddle_width = 50;
int paddle_speed = 3;
int x_vector = pos;      /* X and Y vector of ball */
int y_vector = neg;
double msec = 0;
int trigger = 5;      /* Milliseconds between each frame */
clock_t before;       /* Clock timing data */
clock_t difference;
int score = 0;          
int per_cycle; /* How many timer cycles does it take to increment the balls y value*/
bool cycle_yesno; /* Do I increment the y value this timer cycle? */
int cycle_counter; /* How many timer cycles have passed since the ball's last Y increment */
bool left_key_pressed =  false;  
bool right_key_pressed = false;
bool update_score_yesno = true;   /* when set to false score does not update, this means score will only increment once even when ball stays on paddle */
bool restart = false;   /* in restart mode certain inputs need to be turned off */
bool no_check_collision = false;   /* in certain cases, such as when the ball is below the paddle, we don't want to check for collision */
int increment_speed_counter = 0; /*after 10 succesful paddle deflections, the speed of the ball increases */


void update_screen(void);
void init_draw_variables(void);
void key_event(void);
void ball_move(int x, int y);
void check_collision(void);
bool predproc(Display *display, XEvent *event, XPointer *arg);
void draw_bottom(void);
void update_score(void);
int how_many_digits(int num);
int random_number(int max, int min); /* Generate random number between min and max */
int game_loop(void);
void  randomize_bounce(void);
void update_cycle(void);
void game_over(void);
void reset_game(void);
void draw_game_over(void);

int main (int argc, char **argv)
{

  /* Set up  display */

  if ((display = XOpenDisplay(NULL)) == NULL){
    fprintf(stderr, "Cannot open display server.");
    exit(-1);
  }
  
  scn = DefaultScreenOfDisplay(display);
  display_width = DisplayWidth(display, 0);
  display_height = DisplayHeight(display, 0);
  main_window_width = (display_width / 3);
  main_window_height = (display_height - 50);
  
  
  
  
  
  /*Setup Main Window*/
  
  
  mainwindow = XCreateSimpleWindow(display, XDefaultRootWindow(display),0,0,main_window_width, main_window_height,1,0,0x00ffffff);   /* TODO: set minimum size */
  icon_pixmap = XCreateBitmapFromData(display, mainwindow, icon_bits,icon_width,icon_height);
  logo_pixmap = XCreatePixmapFromBitmapData(display, mainwindow, logo_bits,logo_width,logo_height,0,0x00ffffff,DefaultDepthOfScreen(scn));
  left_arrow_pixmap = XCreatePixmapFromBitmapData(display, mainwindow, leftarrow_bits, leftarrow_width, leftarrow_height, 0, 0x00ffffff,DefaultDepthOfScreen(scn));
  right_arrow_pixmap = XCreatePixmapFromBitmapData(display, mainwindow, rightarrow_bits, rightarrow_width, rightarrow_height, 0, 0x00ffffff,DefaultDepthOfScreen(scn));
  
  
  wmhints.icon_pixmap = icon_pixmap;
  wmhints.flags = IconPixmapHint;
  sizehints.flags = PMinSize;
  sizehints.min_width = 455;
  sizehints.min_height = 718;
  XStringListToTextProperty(&window_name, 1, &windowname);
  XStringListToTextProperty(&icon_name, 1, &iconname);
  classhints.res_name = "Gameball";
  classhints.res_class = "Gameball";
  XSetWMProperties(display, mainwindow, &windowname, &iconname, argv, argc, &sizehints, &wmhints, &classhints);
  XSelectInput(display, mainwindow, KeyPressMask | KeyReleaseMask | ExposureMask );
  XMapWindow(display, mainwindow);

  /* Initialize Graphics Variables */
  init_draw_variables();

  /* Draw the lower portion of the window */
  draw_bottom();

  
  
  
  /* Randomize ball coordinates */
   ball_x = random_number(1,box_width);
   ball_y = random_number(1, (box_height/2)); 


   /* Start the game */
  game_loop();


  
    
}

void init_draw_variables(void){

  /* Geometry for play box */

  XGetWindowAttributes(display, mainwindow, &mainwindowattributes);
  main_window_width = mainwindowattributes.width;
  main_window_height = mainwindowattributes.height;
  box_x = (main_window_width *.05);
  box_y = box_x;
  box_width = (main_window_width - (2 * box_x));
  box_height = (.65 * main_window_height);
  paddle_x = (box_width / 2);

  /* Initialize play box Pixel map (used for double buffering) */

  box = XCreatePixmap(display, mainwindow, (box_width - 1), (box_height - 1),DefaultDepthOfScreen(scn));

  /* X11 Graphics Context */

  gcvalues.foreground = 0;     /* Used for drawing in black */
  gcvalues.background = 0x00ffffff;
  gcvalues.line_width = 1;
  gcvalues.fill_style = FillSolid;
  gcontext = XCreateGC(display, mainwindow, GCForeground | GCBackground | GCLineWidth | GCFillStyle, &gcvalues);

  gcvalues_white.foreground = 0x00ffffff; /* Used for drawing in white */
  gcvalues_white.background = 0x00ffffff;
  gcvalues_white.line_width = 1;
  gcvalues_white.fill_style = FillSolid;
  gcontext_white = XCreateGC(display, mainwindow, GCForeground | GCBackground | GCLineWidth | GCFillStyle, &gcvalues_white);

  /*Set Font */
  text_font = XLoadFont(display,"fixed");
  XSetFont(display, gcontext, text_font);
 

  
}




void update_screen(void) /*Draws playbox with current postion of ball and paddle */
{
  update_cycle(); /*Call to function which determines whether or not to increment y position of ball this timer cycle */
  
  if (left_key_pressed == true && paddle_x > 0){  /* If left or right keys are currently pressed move paddle */
    paddle_x = paddle_x - paddle_speed;
  }
  if (right_key_pressed == true && paddle_x < ((box_width - 2) - paddle_width)){
    paddle_x = paddle_x + paddle_speed;
  }
  
  XDrawRectangle (display, mainwindow, gcontext, box_x, box_y, box_width, box_height);
  
  XFillRectangle (display, box, gcontext_white, 0,0,(box_width - 1),(box_height - 1)); /*Clear old image */
  XDrawArc(display, box, gcontext, ball_x, ball_y, ball_width,ball_width,64,(360 * 64)); /* draw ball */
  XFillRectangle (display, box, gcontext, paddle_x,(box_height - 12), paddle_width, box_height); /* Draw paddle */
  XCopyArea(display, box, mainwindow, gcontext, 0,0,(box_width - 1),(box_height - 1),(box_x + 1),(box_y + 1)); /* Move framebuffer to screen */
 
  
}

void key_event(void){  /* Function is called when game loop detects relevant keys pressed */
  
  if (event.xkey.keycode == 113 && event.xkey.type == KeyPress){    /* Key pressed */
    left_key_pressed = true;
    right_key_pressed = false;
  }
  if (event.xkey.keycode == 113 && event.xkey.type == KeyRelease){   /* key released */
    left_key_pressed = false;
  }
  if (event.xkey.keycode == 114 && event.xkey.type == KeyPress){
    right_key_pressed = true;
    left_key_pressed = false;
  }
  if (event.xkey.keycode == 114 && event.xkey.type == KeyRelease){
    right_key_pressed = false;
  }
  if (event.xkey.keycode == 26){                  /* e for exit */
    exit(0);
  }
  if (event.xkey.keycode == 27 && event.xkey.type == KeyRelease){    /*r for restart */
    restart = true;
    reset_game();
    XFlush(display);
  }

  /* The key_pressed variables which indicate the key either to be up or down is necessary because of the way X11 reports key events a key which is held down will appear to be released and then held down */
    
}


void ball_move(int x, int y){   /* Calculates new postition of ball */
  if (x == neg){
    ball_x = ball_x - ball_speed;
  }
  if (x == pos){
    ball_x = ball_x + ball_speed;
  }
  if (y == neg && cycle_yesno == true){ /* only increment the Y postion if cycle_yesno is true for this cycle. This allows for different angles of ball movement*/
    ball_y = ball_y - ball_speed;
  }
  if (y == pos && cycle_yesno == true){
    ball_y = ball_y + ball_speed;
  }
}

void check_collision(void){
  if (ball_y > (box_height + 20)){  /* If ball is below the box game over */
    game_over();
  }
  
  if ((ball_x + ball_width) > box_width){  /* If ball hits the side of the box change direction */
    x_vector = neg;     /* Change vector */
    randomize_bounce();    /* Pick random angle for ball to bounce at */
    update_score_yesno = true;  
  }
  if (ball_x < 0){
    x_vector = pos;
    update_score_yesno = true;
    randomize_bounce();
  }
  if ((ball_y + ball_width + ball_speed) >= (box_height - 12) && no_check_collision == false){              /*If ball hits paddle */
    if (((ball_x > (paddle_x)) && (ball_x < (paddle_x + (paddle_width)))) || ((ball_x + ball_width) > (paddle_x)) && ((ball_x + ball_width) < (paddle_x + (paddle_width)))){
      if (update_score_yesno == true) {
      score = score + 10;
      update_score();
      }
      update_score_yesno = false; /* Only let the score update once, even if ball hangs out around paddle */
      y_vector = neg;
      per_cycle = 1;   /* A specific angle is needed in this postion, otherwise there is a bug where the ball becomes unpredictable */
    
    
    }
    else if ((ball_y + ball_width) > (box_height - 12)){ /*Ball goes below paddle surface but on either side of paddle */
      x_vector = pos;    /*Send ball straight down as quickly as possible */
	y_vector = pos;
	per_cycle = 1;
	no_check_collision = true;
      
    }
  }
    

    
  if (ball_y <= 0){
    y_vector = pos;
    update_score_yesno = true;
    per_cycle = 1;   /* Specific angle needed */
  }
}
 
bool predproc(display, event, arg) /* Predicate function for sifting out certain key events */
  Display *display;
     XEvent *event;
XPointer *arg;
{
  if (event->type == KeyPress || event->type == KeyRelease || event->type == Expose){
    return(true);
  }
  return(false);
}


void draw_bottom(void){   /*Draw the bottom of the window */
  update_score();
  XCopyArea (display, logo_pixmap, mainwindow, gcontext,0,0,logo_width,logo_height,box_x,(box_y + box_height + 84));
  XCopyArea (display, left_arrow_pixmap, mainwindow, gcontext,0,0,leftarrow_width,leftarrow_height, (box_x + 180),(box_y + box_height + 134));
  XCopyArea (display, right_arrow_pixmap, mainwindow, gcontext,0,0,rightarrow_width,rightarrow_height, (box_x + 280), (box_y + box_height + 134));
  XDrawString(display, mainwindow,gcontext,200,(box_y + box_height + 50),"GAME BALL",9);
  XDrawString(display, mainwindow,gcontext,200,(box_y + box_height + 70),"<r> Restart",11);
  XDrawString(display, mainwindow,gcontext,200,(box_y + box_height + 90),"<e> Exit",8);
}

void update_score(void){

  

  if (update_score_yesno == true){   /* If score is zero just put SCORE:0 */
    if (score == 0){
      XFillRectangle (display, mainwindow, gcontext_white, 0,(box_y + box_height + 5),box_width,15); /* Clear previous score */
      XDrawString(display, mainwindow,gcontext, box_x, (box_y + box_height + 20),"SCORE:0", 7);
      XSync(display, 0);
   }
    
  
  
  else{
    int num_digits = how_many_digits(score);                    /* How many digits is the score value */
    char *score_string = (char*) malloc(num_digits + 7);      /* Allocate memory for the score string */
    strcpy(score_string, "SCORE:");                          /* Initialize string with first part */
    char score2[num_digits];                                 /* Numeric portion of score string */
    sprintf(score2,"%d",score);                            /* Convert score value to char format */
    strncat(score_string, score2,num_digits);                /* Copy second half of score string */
    XFillRectangle (display, mainwindow, gcontext_white, 0,(box_y + box_height + 5),box_width,15);
    XDrawString(display, mainwindow,gcontext, box_x, (box_y + box_height + 20),score_string, (num_digits + 6));
    update_score_yesno = false;
    increment_speed_counter++;                           /* Increment speed counter, after 10 increments or 100 points the ball speed increases */
    if (increment_speed_counter == 10){               
      ball_speed++;                                /* If increment_speed_counter = 10 increase ball speed by 1 */
      increment_speed_counter = 0;
    }

  }
  }
  
}



int how_many_digits(int num){          /* Generic function to count number of digits in a number that is greater than 1 */
  int count = 1;
  while (num >= 1){
    num =  num /10;
    if (num < 1)
      return count;
    count++;
    }
}

  
int random_number(int min, int max){          /* Generates Random number between min and max */
  srand(time(NULL));
  int num;
  num = rand() % (max + 1);
  return num;
}






int game_loop(void){             /* main loop */
  /*Event Loop*/

  while (1) {                    /* Game Timer */
    before = clock();
    do {
      difference = clock() - before;
      msec = difference  * 1000/ CLOCKS_PER_SEC;
     
    
    }while ( msec < trigger);    /* Loop until trigger is reached */
    msec = 0;
    if (restart == true){       /* If restart is set to true reset variables by calling reset_game() */
      restart = false;
      reset_game();
    }
    
    if (XCheckIfEvent(display, &event, predproc, NULL) == true){  /* XCheckIFEvent checks for specific events using the predicate procedure */
    
    
    if (event.type == KeyPress || event.type == KeyRelease){
      key_event();
      }
    if (event.type == Expose){       /* If an X11 expose event occurs window must be redrawn. Playbox is always redrawn automatically */
	 draw_bottom();
     }
    }
    
  
    
  
 
      
    check_collision();
    ball_move(x_vector, y_vector);
    update_screen();

    
}

}

void randomize_bounce(void){   /* Ball bounces off side walls at random angles. Angles are determined by incrementing the ball's y axis only certain timer cycles. */
  per_cycle = random_number(0,4);
  cycle_counter = 0;
}

void update_cycle(void){    /* Keep track of y-axis cycles */
  if (cycle_counter == per_cycle){       
    cycle_yesno = true;
    cycle_counter = 0;
  }
  else {
    cycle_yesno = false;
    cycle_counter++;
  }
  
}


void game_over(void){     /* Game Over */
  draw_game_over();        /* Display Game Over screen */
  bool continue_loop = true;      
  while (continue_loop == true){    /* Event handling loop, continue until continue_loop is set to false */
    XNextEvent(display, &event);
    if (event.type == Expose){
      draw_bottom();
      draw_game_over();
    }
    if (event.type == KeyRelease){
      if (event.xkey.keycode == 26){
	exit(0);
      }
      if (event.xkey.keycode == 27){
       restart = true;
      reset_game();
      continue_loop = false;
      
      
    }
  }
  
      
  }
}

void reset_game(void){           /* Resets variable in event of game restart */
  ball_x = random_number(1,box_width);    /* Random Ball X and Y */
   ball_y = random_number(1, (box_height/2));
   ball_speed = 3;
   score = 0;                        
   update_score_yesno = true;
   update_score();
   paddle_x = (box_width / 2);
   no_check_collision = false;
   increment_speed_counter = 0;
   left_key_pressed = false;           /* Ensures no paddle movement directly after reset */
   right_key_pressed = false;


}

void draw_game_over(void){                             /* Display game over screen */
  int text_x = (box_width / 3);
  int text_y = (box_height / 2);
  XFillRectangle (display, mainwindow,gcontext, box_x,box_y,box_width,box_height);
  XDrawString(display, mainwindow,gcontext_white, (box_x + text_x), (box_y + text_y),"GAME OVER",9);
  XDrawString(display, mainwindow,gcontext_white,(box_x + text_x),(box_y + text_y + 20),"<r> Restart",11);
  XDrawString(display, mainwindow,gcontext_white,(box_x + text_x) ,(box_y + text_y + 40),"<e> Exit",8);
  XSync(display, false);
  XFlush(display);
}

