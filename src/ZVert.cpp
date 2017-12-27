#include "stdafx.h"
#include "ZVector.h"
#include "ZVert.h"
#include "ZEdge.h"

ZVert::ZVert( VOID )
{
	type			= ZTYPE_ANY;
	index			= 0;
	fixed			= FALSE;

	correspond		= NULL;
	interedges[0]	= NULL;
	interedges[1]	= NULL;
	containFace		= NULL;

	dummy_parent	= this;
}

ZVert::ZVert( ZType typecode, D3DXVECTOR3 vPos )
{
	new (this)ZVert();
	type = typecode;
	pos.SetValAll( vPos );
}

ZVert&
ZVert::operator=( ZVert& v )
{
	type = v.type;
	pos = v.pos;
	correspond = v.correspond;

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 vertex까지 연결하는, 이 vertex로부터 뻗어나가는 엣지를 찾아 반환

 * \param v2	대상 vertex 포인터
 * \return		찾은 엣지 포인터. 못찾은 경우 NUL 반환
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZEdge* 
ZVert::GetEdgeToVertex( ZVert* v2 )
{
	ZEdge** pEtest = this->edges.GetData();
	for( DWORD i=0; i < this->edges.GetSize(); i++ )
		if( pEtest[i]->v2->pos == v2->pos ) 
			return pEtest[i];

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 이 vertex로부터 뻗어나가는 엣지들의 next boundary 엣지 중에서 주어진 엣지와 교차하는 것을 찾아 반환

 이 함수는 MergeWith() 함수에서 cut 과정을 수행하기 위해 필요하다. 타겟 엣지로부터 소스 엣지들을 cut하기 위해, 
 타겟 엣지를 인자로 넣어서 해당 타겟 엣지의 v1에 대한 correspond 소스 vertex로부터 뻗어나가는 엣지의 next boundary 엣지와
 교차하는 것을 찾고 그것으로부터 cut를 시작해 나가기 때문이다. 자세한 것을 알고리즘 참조.

 * \param pE	타겟 엣지 포인터
 * \return		교차하는 엣지 포인터. 없으면 NULL 반환
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZEdge* 
ZVert::FindEdgeIntersect( ZEdge* pE )
{
	ZEdge* result = NULL;

	ZEdge** pTempE = edges.GetData();
	for( DWORD i=0; i < edges.GetSize(); i++ )
	{
		if( pE->CheckIntersect( pTempE[i]->next, NULL ) == TRUE )
		{
			result = pTempE[i]->next;
			break;
		}		
	}
	
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief vertex로부터 뻗어나가는 엣지들 중 주어진 엣지를 제거

 * \param pE	제거할 엣지 포인터
 * \return		제거했으면 TRUE 반환, 못찾았으면 FALSE 반환
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL
ZVert::RemoveEdge( ZEdge* pE )
{
	ZEdge** pEdges = edges.GetData();
	for( DWORD i=0; i < edges.GetSize(); i++ )
		if( pEdges[i] == pE ) 
		{
			edges.Remove(i);
			return TRUE;
		}
		return FALSE;
}