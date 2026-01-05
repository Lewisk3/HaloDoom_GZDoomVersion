float noise1d(float n){
    return fract(cos(n*89.42)*343.42);
}
float noise2d(vec2 co){
  return fract(sin(dot(co.xy ,vec2(1.0,73))) * 43758.5453);
}

vec2 hash2( vec2 p )
{
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    uv = uv*sin(iTime);

    vec3 col = vec3(noise2d(uv));
    fragColor = vec4(col,1.0);
}