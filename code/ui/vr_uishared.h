#ifndef VR_UISHARED_H
#define VR_UISHARED_H

qboolean UI_VR_AdjustFrom640( float *x, float *y, float *w, float *h );
void UI_VR_CompensateModelFov( refdef_t *rd, float desiredFovX, float desiredFovY );
qboolean UI_VR_MenuFocusMoved( void );

// Public in stock ui_shared.c, though ui_shared.h does not declare them.
itemDef_t *Menu_GetFocusedItem( menuDef_t *menu );
float Item_Slider_ThumbPosition( itemDef_t *item );

#endif // VR_UISHARED_H
