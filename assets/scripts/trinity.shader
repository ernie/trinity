models/trinity/trinity
{
	{
		map models/trinity/trinity.tga
		rgbGen lightingDiffuse
	}
	{
		map $whiteimage
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen const ( 0.20 0.20 0.20 )
	}
	{
		map textures/sfx/specular.tga
		tcGen environment
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.08 0.08 0.08 )
	}
}

gfx/trinity/flameball
{
	cull none
	nomipmaps
	{
		clampmap models/mapobjects/baph/bapholamp_fx.tga
		blendFunc GL_ONE GL_ONE
		tcmod rotate 40
		rgbGen wave sin 0.65 0.2 0 0.7
	}
	{
		clampmap models/mapobjects/baph/bapholamp_fx2.tga
		blendFunc GL_ONE GL_ONE
		tcmod rotate -35
		rgbGen wave sin 0.7 0.2 0.35 0.9
	}
}
