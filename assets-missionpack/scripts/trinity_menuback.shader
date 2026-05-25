// trinity: override vanilla menuback/levelShotDetail shaders to use the HD
// textures imported from HQQ3 (logo1024 / detail1024). Everything else in
// vanilla's gfx.shader is left untouched.

menuback
{
	nopicmip
	nomipmaps
	{
		map textures/sfx/logo1024.tga
		rgbGen identity
	}
}

menubackRagePro
{
	nopicmip
	nomipmaps
	{
		map textures/sfx/logo1024.tga
	}
}

levelShotDetail
{
	nopicmip
	{
		map textures/sfx2/detail1024.tga
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbGen identity
	}
}
