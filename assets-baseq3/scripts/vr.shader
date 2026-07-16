
//
// VR Hud Shader Sprite - the image is just a placeholder and is replaced in code
//

sprites/vr/hud
{
	cull disable
	{
		clampmap sprites/plasmaa.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		// Use rgbGen identity to pass HUD texture colors through at full brightness.
		// The HUD buffer content is already dimmed by identityLight during rendering,
		// and the gamma pass applies obScale to compensate. Without this, we get
		// double-dimming (50% brightness) because the default CGEN_IDENTITY_LIGHTING
		// would also apply identityLight to vertex colors.
		rgbGen identity
	}
}