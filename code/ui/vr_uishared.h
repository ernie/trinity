#ifndef VR_UISHARED_H
#define VR_UISHARED_H

qboolean UI_VR_AdjustFrom640( float *x, float *y, float *w, float *h );
void UI_VR_CompensateModelFov( refdef_t *rd, float desiredFovX, float desiredFovY );

#endif // VR_UISHARED_H
