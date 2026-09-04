models/trinity/trinity
{
	{
		map models/trinity/trinity.tga
		rgbGen lightingDiffuse
	}
	{
		map $whiteimage
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen const ( 0.92 0.94 1.00 )
	}
	{
		map textures/sfx/specular.tga
		tcGen environment
		blendFunc GL_ONE GL_ONE
		rgbGen const ( 0.10 0.10 0.11 )
	}
	{
		map models/trinity/trinity_glow0.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		alphaGen wave sin 0.35 0.45 0 0.27
	}
	{
		map models/trinity/trinity_glow1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		alphaGen wave triangle 0.35 0.45 0.333 0.38
	}
	{
		map models/trinity/trinity_glow2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		alphaGen wave sin 0.35 0.45 0.667 0.21
	}
}

gfx/trinity/flare
{
	{
		map models/trinity/trinity_glow0.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
	{
		map models/trinity/trinity_glow1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
	{
		map models/trinity/trinity_glow2.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
	}
}

gfx/trinity/wordmark
{
	nopicmip
	nomipmaps
	{
		map gfx/trinity/wordmark.tga
		blendFunc blend
		rgbGen vertex
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
