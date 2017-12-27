#pragma once

#include "ZPublic.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
* \brief 면 클래스. face를 표현하는 클래스이다.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZFace
{
public:
	DWORD					m_dwSubsetID;			///< 이 면의 머티리얼 서브셋 ID
	ZType					type;					///< 면의 종류. ZType 참조
	ZEdge*					edge;					///< 이 면의 첫번째 boundary edge 포인터

public:
	ZFace( VOID );
	ZFace( DWORD subsetID, ZType type );

	/// 주어진 정점의 이 face 내에서의 barycentric 보간 지점 f, g값을 계산하여 2D 벡터로 반환
	D3DXVECTOR2				GetBarycentricUV( ZValue valType, D3DXVECTOR3 vPos );

	/// 주어진 정점 이 face 내에서의 barycentric 보간 노멀 벡터를 반환
	D3DXVECTOR3				GetBarycentricNormal( ZValue valType, D3DXVECTOR3 vPos );
};