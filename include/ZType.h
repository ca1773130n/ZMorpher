#pragma once

#define EPSILON						0.001f
#define MIN_EPSILON					0.000001f

#define PI							3.141592f
#define DEGREE_TO_RADIAN(x)			(x * PI / 180.0f)
#define RADIAN_TO_DEGREE(x)			(x * 180.0f / PI)

typedef enum { ZTYPE_ANY, ZTYPE_SRC, ZTYPE_TGT, ZTYPE_DIV, ZTYPE_CUT }	ZType;
typedef enum { ZVAL_ORG, ZVAL_CUR, ZVAL_MIN, ZVAL_MAX }					ZValue;
typedef enum { ZSMTYPE_SINGLE, ZSMTYPE_COMPOSED }						ZSubsetType;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 버텍스 정의 : 바운딩 박스 및 간단한 폴리곤에 쓰이는 버텍스들
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct LASSOVERTEX
{
	struct VERTEX vertex;
	struct LASSOVERTEX *next;
};

struct MESHVERTEX 
{
	D3DXVECTOR3 pos;	
	D3DXVECTOR3 normal;
	D3DXVECTOR2 texcoord;		
};
#define D3DFVF_MESHVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

