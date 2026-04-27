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
		clampmap gfx/trinity_flame_a.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen const ( 1.0 0.6 0.15 )
		alphaGen wave sin 0.65 0.2 0 0.7
		tcmod rotate 40
	}
	{
		clampmap gfx/trinity_flame_b.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen const ( 1.0 0.5 0.1 )
		alphaGen wave sin 0.7 0.2 0.35 0.9
		tcmod rotate -35
	}
}
