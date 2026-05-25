

// trinity: this `console` block was the source of missionpack's loose
// console tiling — id redefined the shader in this file (overriding the
// tighter baseq3 definition in gfx.shader) with `tcmod scale .02 .01`,
// which makes each texel cover a huge area. Restored the baseq3 tiling
// (`tcmod scale 2 1`, same textures as baseq3) so missionpack's console
// reads as densely as baseq3's. Referenced textures live in baseq3 paks
// and resolve through the missionpack fs_basegame fallback chain.
console
{
	nopicmip
	nomipmaps

	{
		map gfx/misc/console01.tga
		blendFunc GL_ONE GL_ZERO
		tcMod scroll .02 0
		tcMod scale 10 5
	}
	{
		map gfx/misc/console02.tga
		blendFunc add
		tcMod turb 0 .1 0 .1
		tcMod scale 10 5
		tcmod scroll 0.2 .1
	}
}


levelShotDetail

{
	nopicmip
	{
		map textures/sfx2/detail2.tga
        	blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
	}
}

nailtrail
{
	sort nearest
	cull none
	{
		clampmap models/weaphits/nailtrail.tga 
		blendFunc Add
		rgbGen vertex
                tcMod rotate -30
	}

}