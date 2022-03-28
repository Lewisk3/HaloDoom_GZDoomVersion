void SetupMaterial(inout Material matid)
{
	vec2 texCoord = vTexCoord.xy;
	texCoord.y = 1.0 - texCoord.y;
	vec4 camTexColor = texture(camTex, texCoord);
	matid.Base = camTexColor;
	matid.Bright = texture(brighttexture, texCoord);
}

/*
vec4 Process(vec4 color)
{
	vec2 texCoord = gl_TexCoord[0].xy;
	vec4 curColor = getTexel(texCoord);

	vec4 tint = vec4(0, 0.35, 0, 0.5);
	
    return curColor * tint;
}
*/