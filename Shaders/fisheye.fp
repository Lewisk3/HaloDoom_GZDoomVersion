// Modified from https://www.shadertoy.com/view/4sSSzz by Nash Muhandes

void main()
{
    vec2 texSize = textureSize(InputTexture, 0);
    //vec2 uv = TexCoord.xy / texSize.xy;
    vec2 uv = TexCoord.xy;
    float aspectRatio = texSize.x / texSize.y;
    float strength = 0.03;

    vec2 intensity = vec2(strength * aspectRatio,
                          strength * aspectRatio);

    vec2 coords = uv;
    coords = (coords - 0.5) * 2.0;

    vec2 realCoordOffs;
    realCoordOffs.x = (1.0 - coords.y * coords.y) * intensity.y * (coords.x);
    realCoordOffs.y = (1.0 - coords.x * coords.x) * intensity.x * (coords.y);

    vec4 color = texture(InputTexture, uv - realCoordOffs);

    FragColor = vec4(color);
}