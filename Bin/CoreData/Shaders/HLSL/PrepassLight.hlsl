#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"

void VS(float4 iPos : POSITION,
    #ifdef DIRLIGHT
        out float2 oScreenPos : TEXCOORD0,
    #else
        out float4 oScreenPos : TEXCOORD0,
    #endif
    out float3 oFarRay : TEXCOORD1,
    #ifdef ORTHO
        out float3 oNearRay : TEXCOORD2,
    #endif
    out float4 oPos : POSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    #ifdef DIRLIGHT
        oScreenPos = GetScreenPosPreDiv(oPos);
        oFarRay = GetFarRay(oPos);
        #ifdef ORTHO
            oNearRay = GetNearRay(oPos);
        #endif
    #else
        oScreenPos = GetScreenPos(oPos);
        oFarRay = GetFarRay(oPos) * oPos.w;
        #ifdef ORTHO
            oNearRay = GetNearRay(oPos) * oPos.w;
        #endif
    #endif
}

void PS(
    #ifdef DIRLIGHT
        float2 iScreenPos : TEXCOORD0,
    #else
        float4 iScreenPos : TEXCOORD0,
    #endif
    float3 iFarRay : TEXCOORD1,
    #ifdef ORTHO
        float3 iNearRay : TEXCOORD2,
    #endif
    out float4 oColor : COLOR0)
{
    // If rendering a directional light quad, optimize out the w divide
    #ifdef DIRLIGHT
        #ifdef ORTHO
            float depth = tex2D(sDepthBuffer, iScreenPos).r;
            float3 worldPos = lerp(iNearRay, iFarRay, depth);
        #else
            float depth = tex2D(sDepthBuffer, iScreenPos).r;
            float3 worldPos = iFarRay * depth;
        #endif
        float4 normalInput = tex2D(sNormalBuffer, iScreenPos);
    #else
        #ifdef ORTHO
            float depth = tex2Dproj(sDepthBuffer, iScreenPos).r;
            float3 worldPos = lerp(iNearRay, iFarRay, depth) / iScreenPos.w;
        #else
            float depth = tex2Dproj(sDepthBuffer, iScreenPos).r;
            float3 worldPos = iFarRay * depth / iScreenPos.w;
        #endif
        float4 normalInput = tex2Dproj(sNormalBuffer, iScreenPos);
    #endif

    float3 normal = normalize(normalInput.rgb * 2.0 - 1.0);
    float4 projWorldPos = float4(worldPos, 1.0);
    float3 lightColor;
    float3 lightDir;
    float diff;

    // Accumulate light at half intensity to allow 2x "overburn"
    #ifdef DIRLIGHT
        diff = 0.5 * GetDiffuse(normal, cLightDirPS, lightDir);
    #else
        float3 lightVec = (cLightPosPS.xyz - worldPos) * cLightPosPS.w;
        diff = 0.5 * GetDiffuse(normal, lightVec, lightDir);
    #endif

    #ifdef SHADOW
        diff *= GetShadowDeferred(projWorldPos, depth);
    #endif

    #if defined(SPOTLIGHT)
        float4 spotPos = mul(projWorldPos, cLightMatricesPS[0]);
        lightColor = spotPos.w > 0.0 ? tex2Dproj(sLightSpotMap, spotPos).rgb * cLightColor.rgb : 0.0;
    #elif defined(CUBEMASK)
        lightColor = texCUBE(sLightCubeMap, mul(lightVec, (float3x3)cLightMatricesPS[0])).rgb * cLightColor.rgb;
    #else
        lightColor = cLightColor.rgb;
    #endif

    #ifdef SPECULAR
        float spec = lightColor.g * GetSpecular(normal, -worldPos, lightDir, normalInput.a * 255.0);
        oColor = diff * float4(lightColor, spec * cLightColor.a);
    #else
        oColor = diff * float4(lightColor, 0.0);
    #endif
}
