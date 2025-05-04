const float PI = 3.14159265359;
const float l2 = 0.6931471805599;

float4 LinearTosRGB(float4 color) {
    color.rgb = pow(color.rgb, (1.0/2.2).xxx); 
	return color;
}

float3 LinearTosRGB(float3 color) {
    color = pow(color, (1.0/2.2).xxx); 
	return color;
}

float4 sRGBToLinear(float4 color) {
    return float4(pow(color.rgb, 2.2.xxx), color.a);
}

float3 sRGBToLinear(float3 color) {
    return pow(color, 2.2.xxx);
}

float2 EncodeNormal(float3 normal) {
    return (normalize(normal.xy) * (sqrt(-normal.z*0.5+0.5))) * 0.5 + 0.5;
}

float3 GetNormalFromXY(float2 packednormal) {
    float4 nn = float4(packednormal, packednormal) * float4(2,2,0,0) + float4(-1,-1,1,-1);
    float l = dot(nn.xyz,-nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2 + float3(0,0,-1);
}

float map_01(float x, float v0, float v1)
{
  return (x - v0) / (v1 - v0);
}

float saturate(float v)
{
  return clamp(v, 0.0, 1.0);
}

float2 saturate(float2 v)
{
  return clamp(v, 0.0, 1.0);
}

float3 saturate(float3 v)
{
  return clamp(v, 0.0, 1.0);
}

float4 saturate(float4 v)
{
  return clamp(v, 0.0, 1.0);
}

float3 GetWorldSpacePosition(float3 fragmentWorldSpace, float3 CameraPosition, float linearDepth) 
{
    float3 viewRay = normalize(fragmentWorldSpace - CameraPosition);
	return CameraPosition + viewRay * linearDepth;
}

float4x4 CastAffine(float3x4 mat)
{
    float4x4 ret;
	ret[0] = mat[0];
	ret[1] = mat[1];
	ret[2] = mat[2];
	ret = transpose(ret);
	ret[3] = float4(0, 0, 0, 1);
	return ret;
}

float3 convertRGB2XYZ(float3 _rgb)
{
	// Reference:
	// RGB/XYZ Matrices
	// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	float3 xyz;
	xyz.x = dot(float3(0.4124564, 0.3575761, 0.1804375), _rgb);
	xyz.y = dot(float3(0.2126729, 0.7151522, 0.0721750), _rgb);
	xyz.z = dot(float3(0.0193339, 0.1191920, 0.9503041), _rgb);
	return xyz;
}

float3 convertXYZ2RGB(float3 _xyz)
{
	float3 rgb;
	rgb.x = dot(float3( 3.2404542, -1.5371385, -0.4985314), _xyz);
	rgb.y = dot(float3(-0.9692660,  1.8760108,  0.0415560), _xyz);
	rgb.z = dot(float3( 0.0556434, -0.2040259,  1.0572252), _xyz);
	return rgb;
}

float3 convertXYZ2Yxy(float3 _xyz)
{
	// Reference:
	// http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
	float inv = 1.0/dot(_xyz, float3(1.0, 1.0, 1.0) );
	return float3(_xyz.y, _xyz.x*inv, _xyz.y*inv);
}

float3 convertYxy2XYZ(float3 _Yxy)
{
	// Reference:
	// http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	float3 xyz;
	xyz.x = _Yxy.x*_Yxy.y/_Yxy.z;
	xyz.y = _Yxy.x;
	xyz.z = _Yxy.x*(1.0 - _Yxy.y - _Yxy.z)/_Yxy.z;
	return xyz;
}

float3 convertRGB2Yxy(float3 _rgb)
{
	return convertXYZ2Yxy(convertRGB2XYZ(_rgb) );
}

float3 convertYxy2RGB(float3 _Yxy)
{
	return convertXYZ2RGB(convertYxy2XYZ(_Yxy) );
}

float3 convertRGB2Yuv(float3 _rgb)
{
	float3 yuv;
	yuv.x = dot(_rgb, float3(0.299, 0.587, 0.114) );
	yuv.y = (_rgb.x - yuv.x)*0.713 + 0.5;
	yuv.z = (_rgb.z - yuv.x)*0.564 + 0.5;
	return yuv;
}

float3 convertYuv2RGB(float3 _yuv)
{
	float3 rgb;
	rgb.x = _yuv.x + 1.403*(_yuv.y-0.5);
	rgb.y = _yuv.x - 0.344*(_yuv.y-0.5) - 0.714*(_yuv.z-0.5);
	rgb.z = _yuv.x + 1.773*(_yuv.z-0.5);
	return rgb;
}

float3 convertRGB2YIQ(float3 _rgb)
{
	float3 yiq;
	yiq.x = dot(float3(0.299,     0.587,     0.114   ), _rgb);
	yiq.y = dot(float3(0.595716, -0.274453, -0.321263), _rgb);
	yiq.z = dot(float3(0.211456, -0.522591,  0.311135), _rgb);
	return yiq;
}

float3 convertYIQ2RGB(float3 _yiq)
{
	float3 rgb;
	rgb.x = dot(float3(1.0,  0.9563,  0.6210), _yiq);
	rgb.y = dot(float3(1.0, -0.2721, -0.6474), _yiq);
	rgb.z = dot(float3(1.0, -1.1070,  1.7046), _yiq);
	return rgb;
}