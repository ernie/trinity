// VR button glyphs: mipmapped, since the 64-pixel cells draw at a third of their size or less
gfx/vr/glyphs
{
	nopicmip
	{
		map gfx/vr/glyphs.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
