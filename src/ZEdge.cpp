#include "stdafx.h"
#include "ZVert.h"
#include "ZEdge.h"
#include "ZFace.h"
#include "ZMesh.h"
#include "D3DEx.h"

ZEdge::ZEdge( VOID )
{
	marked		= FALSE;
	v1 = v2		= NULL;
	twin = next = NULL;

	index		= 0;

	normal.SetValAll( D3DXVECTOR3(0,0,0) );
	texcoord.SetValAll( D3DXVECTOR2(0,0) );
}

ZEdge::ZEdge( ZVert* pV1, ZVert* pV2, ZFace* pF, D3DXVECTOR3 no, D3DXVECTOR2 co, ZType typecode )
{	
	new (this) ZEdge();
	v1 = pV1;
	v2 = pV2;
	type = typecode;	

	normal.SetValAll( no );
	texcoord.SetValAll( co );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 현재 값을 보간에 사용할 min 값으로 지정

 min 값은 모핑의 시작 지점의 속성을 갖는 값이다. 현재 속성으로 이 min 값을 지정하는 함수이다.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZEdge::SetMin( VOID )
{
	normal.min = normal;
	texcoord.min = texcoord;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 두 엣지 사이에 이 엣지가 존재하는지 여부를 반환

 이 엣지와 주어진 두 엣지, 이렇게 세 엣지는 모두 시작점 v1이 같은 엣지들이다.
 이때 e1, e2가 어떤 각을 이루며 V자 형태 또는 부채꼴 형태를 가질텐데, 이 엣지가 그 사이에 있는지 여부를 판단한다.
 여기서는 e1과 e2가 이루는 사이각보다 이 엣지와 e1, e2 각각이 이루는 각도가 모두 작은 경우 그에 해당한다고 판단한다.

 * \param e1	기준 엣지 1번 포인터
 * \param e2	기준 엣지 2번 포인터
 * \return		TRUE면 사이에 있음, FALSE면 반대
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL
ZEdge::IsBetween( ZEdge* e1, ZEdge* e2 )
{	
	FLOAT angle = e1->GetAngleBetween( e2 );
	return ( GetAngleBetween(e1) <= angle && GetAngleBetween(e2) <= angle );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 이 엣지와 주어진 엣지가 서로 이루는 라디안 각을 구해 돌려준다.

 이 경우도 역시 두 엣지는 v1을 공유하고 있어야 한다. V자를 이루는 두 엣지가 이루는 각도를 구해 반환한다.
 * \param pE	상대 엣지 포인터
 * \return		라디안 값 사이각
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT 
ZEdge::GetAngleBetween( ZEdge* pE )
{
	return GetAngleBetweenVectors( D3DXVECTOR3(v2->pos - v1->pos), D3DXVECTOR3( pE->v2->pos - pE->v1->pos) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief vertex pV를 v1으로 공유하고 이 엣지와 주어진 pEdge 사이에 있는 엣지들 중에서, 이 엣지와 반시계방향으로 가장 작은 각을 이루는 엣지를 찾아 반환한다

 * \param pEdge		이 엣지와 함께 기준이 되는 엣지 포인터
 * \param pV		공유하는 v1 vertex 포인터
 * \return			찾은 엣지 포인터. 없으면 NULL 반환
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZEdge* 
ZEdge::FindClosestCCWEdge( ZEdge* pEdge, ZVert* pV )
{
	FLOAT angle, minangle = 1000000;
	ZEdge* pMinEdge = 0;

	// pV로부터 뻗어나가는 엣지들 중에서 이 엣지와 사이각이 가장 작고 이 엣지와 pEdge 사이에 놓인 것을 찾는다
	ZEdge** pE = pV->edges.GetData();
	for( DWORD i=0; i < pV->edges.GetSize(); i++ )
	{
		if( pE[i]->IsBetween(pEdge, this) == TRUE )
		{
			angle = GetAngleBetween( pE[i] );
			if( angle < minangle ) 
			{
				minangle = angle;
				pMinEdge = pE[i];
			}
		}
	}
	return pMinEdge;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 교차 판정 함수.

 이 엣지와 주어진 엣지 e2가 서로 구면상에서 교차하는지 판단하고, 교차할 경우 교점을 구해 ppNewVert에 돌려준다

 * \param e2			검사할 상대 엣지 포인터
 * \param ppNewVert		교점 vertex 포인터. 주어진 값이 NULL이면 교차여부만 반환하고, NULL이 아니면 교차할 경우 교점이 여기 저장된다
 * \return				교차 여부를 반환. TRUE면 교차, FALSE면 교차하지 않음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL
ZEdge::CheckIntersect( ZEdge* e2, ZVert** ppNewVert )
{
	ZEdge* e1 = this;	

	// 교차 판정할 필요가 없는 조건들. 서로 v1 또는 v2를 공유하거나 twin관계인 등은 어차피 교차하지 않으므로 제외한다.(서로 한쪽 끝이 맞닿아 있는 경우는 교차하지 않는 것으로 한다)
	if( e1->v1->pos == e2->v1->pos || e1->v2->pos == e2->v1->pos || e1->v1->pos == e2->v2->pos || e1->v2->pos == e2->v2->pos ) return FALSE;
	if( e1->v1 == e2->v1 || 
		e1->v2 == e2->v1 || 
		e1->v1 == e2->v2 || 
		e1->v2 == e2->v2 ||
		e1->v1->correspond == e2->v1 || 
		e1->v2->correspond == e2->v1 || 
		e1->v1->correspond == e2->v2 || 
		e1->v2->correspond == e2->v2 
		) return FALSE;

	D3DXVECTOR3 v0(0,0,0), v[4], vGamma;
	FLOAT s[4];
	D3DXPLANE plane;

	v[0] = e1->v1->pos;
	v[1] = e1->v2->pos;
	v[2] = e2->v1->pos;
	v[3] = e2->v2->pos;

	//////////////////////////////////////////////////////////////////////////
	// 교차 판정 알고리즘
	//
	// 엣지 e1의 v1, v2와 원점 O가 이루는 삼각형이 속한 평면을 구하고,
	// 엣지 e2를 해당 평면에 투과시켜 교점 vGamma를 구한다.
	// 그리고 s값을 구하여 모두 1보다 작은 경우 교차한다고 판단한다.
	// Alexa 논문에 해당 알고리즘이 나와 있으므로 참고할것.
	//////////////////////////////////////////////////////////////////////////
	for( int i=0; i < 4; i += 2 )
	{
		D3DXPlaneFromPoints( &plane, &v0, &v[i], &v[i+1] );
		if( D3DXPlaneIntersectLine( &vGamma, &plane, &v[(i+2)%4], &v[(i+3)%4] ) == NULL ) return FALSE;	

		s[(i+2)%4] = D3DXVec3Length( &(vGamma - v[(i+2)%4]) ) / D3DXVec3Length( &(v[(i+3)%4] - v[(i+2)%4]) );
		s[(i+3)%4] = D3DXVec3Length( &(vGamma - v[(i+3)%4]) ) / D3DXVec3Length( &(v[(i+2)%4] - v[(i+3)%4]) );
	}

	if( s[0] < 1 && s[1] < 1 && s[2] < 1 && s[3] < 1 )
	{		
		// 교점을 저장
		if( ppNewVert != NULL )
		{
			D3DXVec3Normalize( &vGamma, &vGamma );
			ZVert* newVert = new ZVert( ZTYPE_CUT, vGamma );

			// 여기서 포지션 min, max 및 barycentric UV를 정함. 이 값들을 토대로 보간에 사용함
			newVert->pos.org = e2->v1->pos.org + s[2] * ( e2->v2->pos.org - e2->v1->pos.org );
			newVert->pos.max = e1->v1->pos.org + s[0] * ( e1->v2->pos.org - e1->v1->pos.org );
			newVert->barycentricUV.x = s[0];
			newVert->barycentricUV.y = s[2];

			// 이 교점을 생성하게 된 두 엣지의 포인터와, 그 두 엣지의 네 vertex를 저장해 둔다.
			// 나중에 normal 및 텍스쳐 좌표 보간을 위해 쓰일 것임
			newVert->interedges[0] = e2;
			newVert->interedges[1] = e1;

			newVert->interverts[0] = e1->v1->correspond ? e1->v1->correspond : e1->v1;
			newVert->interverts[1] = e1->v2->correspond ? e1->v2->correspond : e1->v2;
			newVert->interverts[2] = e2->v1;
			newVert->interverts[3] = e2->v2;

			*ppNewVert = newVert;
		}

		return TRUE;
	}
	return FALSE;
}
