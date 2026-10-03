// VR keyboard glow: alpha-weighted additive so the engine's tint and alpha set its color and strength
gfx/vkb/glow
{
	nopicmip
	nomipmaps
	{
		map gfx/vkb/glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}

// VR keyboard icons: mipmapped, since the 128-pixel cells draw at a fraction of their size on the virtual screen
gfx/vkb/icons
{
	nopicmip
	{
		map gfx/vkb/icons.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
