#pragma once

#include "ZPublic.h"
#include "ZVector.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 엣지 클래스. half edge를 표현하는 클래스이다.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZEdge
{
public:
	ZType					type;					///< edge의 종류. ZType 참조.
	DWORD					index;					///< 엣지의 인덱스

	BOOL					marked;					///< 표시되었는지 여부
	ZVector2				texcoord;				///< 텍스쳐 좌표. 엣지마다 할당한다. 엣지의 시작 지점(v1)에서의 텍스쳐 좌표
	ZVector3				normal;					///< 노멀 벡터. 엣지마다 할당. 엣지의 시작 지점(v1)에서의 노멀 벡터. 

	ZVert					*v1, *v2;				///< 엣지의 양 끝 vertex 포인터
	ZEdge*					twin;					///< twin edge(반대 방향) 포인터
	ZEdge*					next;					///< 시계 방향의 다음 boundary 엣지 포인터

	ZFace*					f;						///< 이 엣지를 포함하는 face 포인터. 이 엣지가 f의 시계방향 boundary 엣지임.

private:
	VOID					CalcT( D3DXVECTOR3* vGamma );

public:
	ZEdge( VOID );
	ZEdge( ZVert* pV1, ZVert* pV2, ZFace* pF, D3DXVECTOR3 co, D3DXVECTOR2 no, ZType typecode );

	/// 보간 초기값을 현재 속성으로 지정하는 함수
	VOID					SetMin( VOID );

	/// 주어진 엣지와의 각도를 반환하는 함수
	FLOAT					GetAngleBetween( ZEdge* pE );

	/// 주어진 두 엣지와 함께 v1을 공유하고 같은 평면상에 있는 경우 두 엣지가 이루는 부채꼴 내에 이 엣지가 존재하는지 여부를 반환
	BOOL					IsBetween( ZEdge* e1, ZEdge* e2 );

	/// 주어진 정점으로부터 반시계방향으로 이 엣지와 가장 가까운 각을 이루는 엣지를 반환
	ZEdge*					FindClosestCCWEdge( ZEdge* pEdge, ZVert* pV );

	/// 주어진 엣지와 이 엣지의 교차여부를 반환하고 교점을 계산
	BOOL					CheckIntersect( ZEdge* e2, ZVert** ppNewVert );
};
