// Slider steps and value text shared by both UIs. No engine or library calls.
#ifndef UI_SLIDERVALUE_H
#define UI_SLIDERVALUE_H

// Also in ui/menudef.h for the menu scripts; the two lists must stay identical.
#define SLIDER_VALUE_NONE		0
#define SLIDER_VALUE_INT		1
#define SLIDER_VALUE_SIGNED		2
#define SLIDER_VALUE_DEC1		3
#define SLIDER_VALUE_DEC2		4
#define SLIDER_VALUE_PERCENT	5

int SliderValue_StepCount( float min, float max, float step );
/* The nearest step, so an off-step value already in a config lands on one. */
int SliderValue_Index( float value, float min, float max, float step );
/* Clamped to min..max, then on a step when step is positive. */
float SliderValue_Snap( float value, float min, float max, float step );
/* Whole steps a key press moves, at least one. */
int SliderValue_KeySteps( float base, float step );
/* "+5", "1.25", "30%" */
void SliderValue_Text( int format, float value, char *buf, int size );
/* The shown number as a cvar string: "5", "1.25", "0.30" */
void SliderValue_CvarString( int format, float value, char *buf, int size );
#endif
