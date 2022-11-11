vec2 getFishEye(vec2 uv, float level) {
    float len = length(uv);
    float a = len * level;
    return vec2(uv / len * sin(a));
}

vec4 getColor(vec2 ray) {
    return texture(InputTexture,ray);
}

void main() {   
	vec2 fragCoord = TexCoord;
	vec2 res = textureSize(InputTexture,0);

	vec2 uv = fragCoord.xy;
    uv.x *= (res.x / res.y);
    vec2 dir = getFishEye(uv,1.0);
    
    // color
    float fish_eye = smoothstep(2.0,1.6,length(uv)) * 0.25 + 0.75;
	FragColor = vec4(getColor(dir) * fish_eye);
}
