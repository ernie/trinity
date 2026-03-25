models/powerups/armor/newgreen
{
	{
		map textures/sfx/specular.tga
		tcGen environment
		rgbGen identity
	}
	{
		map models/powerups/armor/newgreen.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
	}
}

// Override pak0's energy_grn1 (env map effect) to match red/yellow style (scrolling texture)
models/powerups/armor/energy_grn1
{
	{
		map models/powerups/armor/energy_grn1.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 7.4 1.3
	}
}
