// Lab 8 - a second shader pair that does real work in each stage.
// Vertex stage: displaces a flat grid into a travelling wave and computes the
// analytic surface normal from the wave's partial derivatives.
// Fragment stage: colours by height and applies Lambert diffuse lighting from
// the scene's directional light.
Shader "Lab8/WaveLit"
{
    Properties
    {
        _Amplitude ("Amplitude", Float) = 0.25
        _Frequency ("Frequency", Float) = 3.0
        _Speed     ("Speed", Float) = 2.0
        _LowColor  ("Trough Colour", Color) = (0.05, 0.15, 0.45, 1)
        _HighColor ("Crest Colour", Color) = (0.55, 0.95, 1.0, 1)
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "Queue" = "Geometry" }

        Pass
        {
            Tags { "LightMode" = "ForwardBase" }

            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            #include "Lighting.cginc"

            float _Amplitude;
            float _Frequency;
            float _Speed;
            fixed4 _LowColor;
            fixed4 _HighColor;

            struct appdata
            {
                float4 vertex : POSITION;
            };

            struct v2f
            {
                float4 pos         : SV_POSITION;
                float3 worldNormal : TEXCOORD0;
                float  height      : TEXCOORD1;
            };

            v2f vert(appdata v)
            {
                float t = _Time.y * _Speed;
                float k = _Frequency;
                float ax = k * v.vertex.x + t;
                float az = k * v.vertex.z + 0.7 * t;

                // y = A sin(kx + t) cos(kz + 0.7t)
                float y = _Amplitude * sin(ax) * cos(az);
                v.vertex.y += y;

                // n = (-dy/dx, 1, -dy/dz)
                float dydx =  _Amplitude * k * cos(ax) * cos(az);
                float dydz = -_Amplitude * k * sin(ax) * sin(az);
                float3 objNormal = normalize(float3(-dydx, 1.0, -dydz));

                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                o.worldNormal = UnityObjectToWorldNormal(objNormal);
                o.height = y / max(_Amplitude, 1e-4);   // -1 .. 1
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float3 n = normalize(i.worldNormal);
                float3 l = normalize(_WorldSpaceLightPos0.xyz);
                float diffuse = max(0.0, dot(n, l));

                fixed3 albedo = lerp(_LowColor.rgb, _HighColor.rgb, saturate(i.height * 0.5 + 0.5));
                fixed3 ambient = 0.25 * albedo;
                return fixed4(ambient + albedo * _LightColor0.rgb * diffuse, 1.0);
            }
            ENDCG
        }
    }
}
