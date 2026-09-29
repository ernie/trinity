// Supersampling picker shared by both UIs: 1.0..2.0 in tenths, and the per-eye size a step would render at.
// No engine or library calls; the UI supplies the engine's "vr_eyesize" answer.
#ifndef VR_SUPERSAMPLE_H
#define VR_SUPERSAMPLE_H

#define VRSS_MIN_TENTHS 10
#define VRSS_MAX_TENTHS 20

/* The nearest tenth within range, so an old off-step value lands on a step. */
int VRSS_Tenths( float value );
float VRSS_Value( int tenths );
/* "recW recH maxW maxH" from the engine; 0 unless four numbers with a positive recommended size. */
int VRSS_ParseEyeSize( const char *text, int *recW, int *recH, int *maxW, int *maxH );
/* "1.3" */
void VRSS_Multiplier( int tenths, char *buf, int size );
/* "2683x2870": the eye size that step renders at; empty without a size. */
void VRSS_Resolution( int tenths, int recW, int recH, int maxW, int maxH, char *buf, int size );
#endif
