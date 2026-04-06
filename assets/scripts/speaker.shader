//
// Speaker icons for VOIP
//

gfx/2d/speaker
{
	nopicmip
	{
		map gfx/2d/speaker.tga
		blendFunc BLEND
		rgbGen exactVertex
	}
}

gfx/2d/speaker_idle
{
	nopicmip
	{
		map gfx/2d/speaker_idle.tga
		blendFunc BLEND
		rgbGen exactVertex
	}
}

gfx/2d/speaker_muted
{
	nopicmip
	{
		map gfx/2d/speaker_idle.tga
		blendFunc BLEND
		rgbGen exactVertex
	}
	{
		map gfx/2d/speaker_x.tga
		blendFunc BLEND
		rgbGen identity
		alphaGen vertex
	}
}
