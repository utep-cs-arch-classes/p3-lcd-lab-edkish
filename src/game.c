#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdlib.h>
#include "buzzer.h"

#define GREEN_LED BIT6
#define S1 !(p2sw_read() &BIT0)
#define S2 !(p2sw_read() &BIT1)
#define S3 !(p2sw_read() &BIT2)
#define S4 !(p2sw_read() &BIT3)
int lives = 3;
int game_status = 1; //used to determine with game is still going 
// like abRectCheck, but excludes a triangle
int 
abSlicedRectCheck(const AbRect *rect, const Vec2 *centerPos, const Vec2 *pixel)
{
  Vec2 relPos;
  vec2Sub(&relPos, pixel, centerPos); /* vector from center to pixel */
  /* reject pixels in slice */
  if (relPos.axes[1] >= 0 && relPos.axes[1]/3 > relPos.axes[0] || (relPos.axes[0] >= 0 && relPos.axes[0]/2 < relPos.axes[1])){
    if(relPos.axes[0] >= 0 && relPos.axes[0]/2 < relPos.axes[1]){
      return 0;}
  return 0;}
  else
    return abRectCheck(rect, centerPos, pixel);
}

AbRect rect10 = {abRectGetBounds, abSlicedRectCheck, 3,2};;

AbRectOutline fieldOutline = {	
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer bullet = {
  (AbShape *)&rect10,
  {(screenWidth/2)+1010, screenHeight+15}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};
int bullets = 0;

Layer layer3 = {		/**< Layer with an asteriod */
  (AbShape *)&circle8,
  {(screenWidth/2)+15, 20}, 
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GRAY,
  &bullet,
};


Layer layer2 = {		/**< Layer with an asteriod */
  (AbShape *)&circle6,
  {(screenWidth/2),32 }, /**< center {screenWidth/2, screenHeight/2}, */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GRAY,
  &layer3,
};


Layer layer1 = {		/**< Layer with an asteriod */
  (AbShape *)&circle6,
  {(screenWidth/2)+5,  56}, /**< center {screenWidth/2, screenHeight/2}, */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GRAY,
  &layer2,
};
int asteriod =0 ;
Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_YELLOW,
  &layer1
};

Layer layer0 = {		/**< Layer player */
  (AbShape *)&rect10,//&circle14
  {(screenWidth/2)-10, (screenHeight/2)+60}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED, //COLOR_ORANGE
  &fieldLayer, //&layer1
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
// MovLayer ml4 = { &layer4, {1,1}, 0}
MovLayer ml5 = {&bullet, {0,0}, 0};
MovLayer ml4 = {&layer3, {-1,1},&ml5}; 
MovLayer ml3 = { &layer2, {3,1}, &ml4};// < not all layers move //purple circle
MovLayer ml2 = { &layer1, {1,1}, &ml3}; //&layer1, {1,2}, &m13, red square 
MovLayer ml0 = { &layer0, {0,0}, &ml2 }; //{2,1}
void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  drawString5x7(1,1,"SCORE:", COLOR_WHITE,COLOR_BLACK);
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
    if(ml->velocity.axes[1] < -5 && ml->layer->pos.axes[1]<50){
      ml->layer->color = COLOR_BLACK;
    }
    if(ml->velocity.axes[1]>3){
      ml5.velocity.axes[1] = 0;
      ml5.layer->pos.axes[0] = ml0.layer->pos.axes[0];
      ml5.layer->pos.axes[1] = ml0.layer->pos.axes[1]-10;
      ml5.layer->posLast.axes[0] = ml0.layer->posLast.axes[0];
      ml5.layer->posLast.axes[1] = ml0.layer->posLast.axes[1];
      ml5.layer->posNext.axes[0] = ml0.layer->posNext.axes[0];
      ml5.layer->posNext.axes[1] = ml0.layer->posNext.axes[1];
    }
  } /**< for ml */
 }

void collision(MovLayer *ml){
  u_char axis;
  u_char cnt = 0;
  ml = ml->next;
  for(;ml && cnt < 3; ml = ml->next){
  if(ml0.layer->pos.axes[0] < ml->layer->pos.axes[0] +10 && ml0.layer->pos.axes[0] > ml->layer->pos.axes[0] - 10 && ml0.layer->pos.axes[1] < ml->layer->pos.axes[1] +10 && ml0.layer->pos.axes[1] > ml->layer->pos.axes[1] - 10){
    buzzer_set_period(1550);
    game_status = 0;
    return;
  }
  cnt++;
}
}
u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= 0;

  configureClocks();
  buzzer_init();
  lcd_init();
  shapeInit();
  p2sw_init(BIT0 | BIT1 | BIT2 | BIT3);

  shapeInit();
  layerInit(&layer0);
  layerDraw(&layer0);
 

  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  
  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    if(!game_status)
      drawString5x7(40,80,"GAME OVER", COLOR_RED,COLOR_BLACK);
    else{
      movLayerDraw(&ml0, &layer0);
    }}
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler(){
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15){
    u_int switches = p2sw_read();
    if(!game_status){
      return;
    }
    
    if(S1)
      ml0.velocity.axes[0] = -4;
    else if (S4)
      ml0.velocity.axes[0] = 4;
    else
      ml0.velocity.axes[0]= 0;;

    
    if(S2)
      ml0.velocity.axes[1] = -4;
    else if (S3)
      ml0.velocity.axes[1] = 4;
     else
      ml0.velocity.axes[1]= 0;
    
   collision(&ml0);
    mlAdvance(&ml0, &fieldFence);

    redrawScreen = 1;
    count = 0;
}
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
 
