#pragma once

#include "ZPublic.h"
#include "ZVector.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 정점 클래스. vertex를 표현하는 클래스이다.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZVert
{
public:
	ZType					type;					///< vertex의 종류. ZType 열거형을 참조. vertex가 소스 / 타겟 메시로부터 생성되었는지, 또는 Divide / Cut 과정에 의해 생성된 것인지를 나타냄.
	WORD					index;					///< 인덱스 넘버. 
	BOOL					fixed;					///< 고정되었는지 여부. Spherical Parameterize에 사용되며, 다른 용도로도 쓸수 있음. 

	ZVector3				pos;					///< 위치 벡터
	D3DXVECTOR2				barycentricUV;			///< barycentric 보간을 위한 f, g 값을 갖는 2D 벡터

	CGrowableArray<ZEdge*>	edges;					///< 이 vertex로부터 뻗어나가는 엣지들의 배열
	ZVert*					correspond;				///< 대응하는 상대편 vertex 포인터
	ZVert*					interverts[4];			///< vertex가 Z_TYPE_CUT 타입인 경우, 교차하는 두 엣지를 구성하는 4개의 vertex들의 포인터 배열. 위치 및 텍스쳐좌표 보간을 위해 필요.
	ZEdge*					interedges[2];			///< vertex가 Z_TYPE_CUT 타입인 경우, 교차하는 두 엣지의 포인터 배열. 위치 및 텍스쳐좌표 보간을 위해 필요.
	ZFace*					containFace;			///< vertex를 구면상에서 포함하는 face의 포인터. vertex의 최종 위치를 결정하기 위해 필요.

	CGrowableArray<ZVert*>	dummies;				///< 중복 vertex weld를 위한 더미 배열
	ZVert*					dummy_parent;			///< 중복 vertex들을 대표하는 부모 vertex 포인터

public:
	ZVert( VOID );
	ZVert( ZType typecode, D3DXVECTOR3 vPos );

	/// 동등 연산자
	ZVert&					ZVert::operator=( ZVert& v );

	/// 이 버텍스로부터 특정 버텍스까지의 엣지를 찾아 반환
	ZEdge*					GetEdgeToVertex( ZVert* v2 );

	/// 이 버텍스로부터 뻗어나가는 엣지들의 next들 중 특정 엣지가 교차하는 것을 찾아 반환
	ZEdge*					FindEdgeIntersect( ZEdge* pE );

	/// 버텍스로부터 뻗어나가는 엣지 하나를 제거
	BOOL					RemoveEdge( ZEdge* pE );
};