uniform float timer;

vec4 Process(vec4 color)
{
	vec2 fragCoord = gl_TexCoord[0].xy;
	vec4 fragColor = getTexel(fragCoord);
	
    // Find our screen coordinates, correct for aspect
    vec2 uv = fragCoord.xy;
}  