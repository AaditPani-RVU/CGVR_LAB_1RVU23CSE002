// Lab 8 - the Unity equivalent of shader_triangle.cpp's GLSL pair.
// The vertex stage transforms each vertex to clip space and forwards its RGB
// colour; the rasteriser interpolates that colour across the triangle, and the
// fragment stage writes the interpolated value straight to the framebuffer.
Shader "Lab8/VertexColor"
{
    SubShader
    {
        Tags { "RenderType" = "Opaque" "Queue" = "Geometry" }
        Cull Off

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float4 color  : COLOR;
            };

            struct v2f
            {
                float4 pos   : SV_POSITION;
                float4 color : COLOR0;
            };

            // Vertex shader: object space -> clip space (model * view * projection).
            v2f vert(appdata v)
            {
                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                o.color = v.color;
                return o;
            }

            // Fragment shader: receives the barycentrically interpolated colour.
            fixed4 frag(v2f i) : SV_Target
            {
                return i.color;
            }
            ENDCG
        }
    }
}
