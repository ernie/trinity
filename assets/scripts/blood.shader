// Blood splat decals (gib bounce marks, droplet impacts, gib-spray ground splat,
// surface splat). Source art is already red with alpha; rgbGen const darkens it
// toward stock blood. The animated bloodGout lives in blood.shaderx (needs the
// 10-frame extended animMap).

bloodSplat0
{
	polygonOffset
	{
		map gfx/blood/splat0.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const ( 0.32 0.0 0.04 )
		alphaGen vertex
	}
}

bloodSplat1
{
	polygonOffset
	{
		map gfx/blood/splat1.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const ( 0.32 0.0 0.04 )
		alphaGen vertex
	}
}

bloodSplat2
{
	polygonOffset
	{
		map gfx/blood/splat2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const ( 0.32 0.0 0.04 )
		alphaGen vertex
	}
}

bloodSplat3
{
	polygonOffset
	{
		map gfx/blood/splat3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen const ( 0.32 0.0 0.04 )
		alphaGen vertex
	}
}
