#include "stdafx.h"
#include "ZVert.h"
#include "ZEdge.h"
#include "ZFace.h"
#include "ZMesh.h"
#include "ZMaterial.h"

#include "D3DEx.h"
#include <stack>

ZMesh::ZMesh( VOID )
{
	m_dwNumVerts = m_dwNumFaces = m_dwVertStart = m_dwFaceStart = 0;
	m_bHasTexture = FALSE;
	m_vCMass = D3DXVECTOR3( 0,0,0 );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief ZMesh 생성 함수

 이 함수에서는 주어진 서브셋 ID들에 해당하는 face들을 구성하여 하나의 서브메시로 만든다.
 여러 서브셋들이라 하더라도, geometry 측면에서는 하나의 서브메시로 간주할 수 있는 경우가 있다. 예를 들어 

 * \param subsetIDs		이 가변배열 subsetIDs 에는 통합해햐 할 서브셋 ID들이 들어있다. 이 배열의 크기만큼 메인 루프를 돌면서 과정을 수행한다. 이 서브셋 ID들에 해당하는 face들을 전부 하나의 서브메시로 만든다.
 * \param pVertices		전체 DXUT메시의 버텍스 버퍼 포인터이다. 이 중에서 해당 서브셋의 vertex들만 취한다.
 * \param pIndices		전체 DXUT메시의 인덱스 버퍼 포인터이다. 이 중에서 해당 서브셋의 face 인덱스들만 참고한다.
 * \param pAttrTable	DXUT 메시의 attribute table 포인터이다. 이 테이블에는 서브셋들에 대한 정보가 있다. 해당 ID의 서브셋이 VB의 어디부터 어디까지인지 등을 이걸 참고하게 된다
 * \param pMat			머티리얼 배열 포인터
 * \param type			소스 메시인지 타겟 메시인지를 나타냄. ZType 참조.
 * \return				없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID 
ZMesh::Create( CGrowableArray<DWORD>& subsetIDs, MESHVERTEX* pVertices, WORD* pIndices, D3DXATTRIBUTERANGE* pAttrTable, D3DMATERIAL9* pMat, ZType type )
{	
	m_dwNumSubset = subsetIDs.GetSize();
	DWORD* pSubsetIDs = subsetIDs.GetData();
	DWORD subsetID;
	
	// m_pFaces은 2차원 배열이다. 서브셋 ID별로 배열들이 있고 해당 ID에 해당하는 face들을 그 배열에 넣게 된다.
	m_pFaces = new CGrowableArray<ZFace*>[m_dwNumSubset];
	m_pMaterials = new ZMaterial[m_dwNumSubset];

	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		subsetID = pSubsetIDs[i];

		//////////////////////////////////////////////////////////////////////////
		// 정점을 생성한다. att table을 참고하여 pSubsetIDs의 모든 서브셋에 대한 정점을 만들어 배열에 넣는다.
		// 이때 중복 vertex들이 있을 수 있다. D3D API를 사용하여 weld를 수행하였더라도, 서브셋이 달라 같은 위치의 정점인데
		// 중복 표현된 것들이 있을 수 있다. 이것들의 정보를 저장해 두었다가 메시를 만들고 나서 중복 정점들을 제거한다.
		ZVert **pVerts, *newvert;
		BOOL bFound = FALSE;
		for( DWORD j=pAttrTable[subsetID].VertexStart; j < pAttrTable[subsetID].VertexCount; j++ )
		{
			newvert					= new ZVert( type, pVertices[j].pos );
			newvert->index			= m_dwNumVerts++;

			bFound = FALSE;		
			pVerts = m_verts.GetData();
			for( DWORD k=0; k < m_verts.GetSize(); k++ )
				// 중복 정점이 발견되면 대표 vertex의 dummies에 저장한다. 대표 vertex는 중복 정점들 중에서 인덱스가 가장 작은 정점이다.
				if( pVerts[k]->pos == newvert->pos )
				{
					pVerts[k]->dummies.Add( newvert );
					newvert->dummy_parent = pVerts[k];
					newvert->fixed = TRUE;
					bFound = TRUE;
					break;
				}
			m_verts.Add( newvert );

			// 질량 중심을 계산하기 위해 모든 vertex의 포지션을 더한다. 따로 또 for 돌것 없이 여기서 한다. 나중에 vertex 수로 나누면 그것이 질량 중심이다.
			if( !newvert->fixed ) m_vCMass += newvert->pos;
		}
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// face 정보로부터 face와 엣지들을 생성한다
		ZVert* v[4];
		ZVert* r[4];
		ZEdge* e[3];	
		ZEdge* twin;		
		ZFace* newface;
		ZVert** pDum;
		DWORD mvIndex[4];

		pVerts = m_verts.GetData();
		for( DWORD j=pAttrTable[subsetID].FaceStart; j < pAttrTable[subsetID].FaceCount * 3; j += 3 )
		{
			mvIndex[0] = pIndices[j];
			mvIndex[1] = pIndices[j+1];
			mvIndex[2] = pIndices[j+2];
			mvIndex[3] = pIndices[j];

			// 일단 새 face를 만들어둔다
			newface = new ZFace( subsetID, type );		

			// 배열로 처리하기 위한 인덱스 설정
			for( int k=0; k < 3; k++ ) 
				v[k] = r[k] = pVerts[mvIndex[k]];		
			v[3] = pVerts[mvIndex[0]];
			r[3] = pVerts[mvIndex[0]];

			//////////////////////////////////////////////////////////////////////////
			// IB 내의 실제 인덱스와 앞에서 생성한 ZVert 인덱스를 맞춰 준다.
			// 만약 IB에서 참조한 인덱스의 버텍스가 중복 정점을 가리키면, 그것의 대표 버텍스의 인덱스로 바꿔 준다.
			//
			// 이 과정을 하는 이유는 새로 생성할 face의 인덱스들을 모두 유일한 버텍스의 인덱스로 하여
			// 중복 정점을 모핑에 사용하지 않도록 하기 위함이다.
			// 중복 정점을 사용하면 안되는 이유는 모핑을 할때 embed시킬 메시는 closed 메시이어야 하기 때문이다. 
			// 중복 정점이 있다는 것은 face들이 연결되지 않고 떨어져 있다는 것을 의미하므로.
			//////////////////////////////////////////////////////////////////////////
			for( int k=0; k < 4; k++ ) 
			{
				bFound = FALSE;
				for( int m=0; m < m_verts.GetSize() && bFound == FALSE; m++ )
				{
					pDum = pVerts[m]->dummies.GetData();
					for( int n=0; n < pVerts[m]->dummies.GetSize(); n++ )
						// 중복 정점을 가리키면 대표 버텍스의 인덱스로 바꾼다
						if( mvIndex[k] == pDum[n]->index ) 
						{
							r[k] = pDum[n];
							v[k] = pVerts[m];						
							bFound = TRUE;
							break;
						}
				}
			}

			// face의 세 정점으로부터 edge 생성
			for( int k=0; k < 3; k++ )
			{	
				e[k] = new ZEdge( v[k], v[k+1], newface, pVertices[r[k]->index].normal, pVertices[r[k]->index].texcoord, type );

				e[k]->f = newface;	

				v[k]->edges.Add( e[k] );			
				m_edges.Add( e[k] );			
			}
			e[0]->next = e[1];
			e[1]->next = e[2];
			e[2]->next = e[0];

			newface->edge = e[0];

			m_pFaces[i].Add( newface );
			m_dwNumFaces++;
		}

		// dummy들을 제거. 이제는 필요 없다.
		DWORD index = 0;
		while( index < m_verts.GetSize() )
		{
			while( pVerts[index]->fixed ) 
			{
				if( index < m_verts.GetSize() )
				{
					m_verts.Remove( index );
				}
				else break; 
			}
			index++;
		}

		// edge들의 twin 설정
		ZEdge** pE = m_edges.GetData();
		for( DWORD j=0; j < m_edges.GetSize(); j++ )
		{
			if( pE[j]->twin == NULL )
			{
				pE[j]->twin = pE[j]->v2->GetEdgeToVertex( pE[j]->v1 );
				if( pE[j]->twin ) pE[j]->twin->twin = pE[j];
				else 
				{
					pE[j]->twin = new ZEdge( pE[j]->v2, pE[j]->v1, NULL, pE[j]->normal.org, pE[j]->texcoord.org, type );
					pE[j]->twin->next = NULL;
				}
			}
		}

		// 머티리얼 설정
		m_pMaterials[i].SetValAll( pMat );

		//////////////////////////////////////////////////////////////////////////
	}

	m_vCMass /= m_verts.GetSize();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 파괴 함수.

 메시를 제거할때 필요한 작업들을 수행한다. 명시적 생성, 소멸 함수를 갖는 이유는 ZObject가 메시 객체들을 포인터로 갖고 있기 때문
 * \param VOID 
 * \return 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::Destroy( VOID )
{
	ZVert** pV; 
	ZEdge** pE; 
	ZFace** pF;

	m_verts.RemoveAll();
	m_edges.RemoveAll();
	for( int i=0; i < m_dwNumSubset; i++ ) m_pFaces[i].RemoveAll();
	m_dwNumVerts = m_dwNumFaces = m_dwVertStart = m_dwFaceStart = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 ID의 서브셋 face들이 실제 m_pFaces의 몇번째 인덱스의 배열에 속해있는지 그 인덱스를 돌려준다

 이것이 필요한 이유는 m_pFaces 배열의 1차원 인덱스는 실제 서브셋 ID와 다르기 때문에 특정 ID의 서브셋에 해당하는 face들을 참조하고자 할때
 몇번째 배열이 그것인지 알 수 없기 때문이다. 이 함수가 그것을 도와준다.

 * \param id	서브셋 ID
 * \return		배열 인덱스
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD
ZMesh::GetIndexOfSubsetID( DWORD id )
{
	ZFace** pF;
	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		pF = m_pFaces[i].GetData();
		if( pF[0]->m_dwSubsetID == id ) return i;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 face f를 주어진 vertex pSrcV로 분할한다

 이 함수를 통해 face f는 pSrcV를 기준으로 3개의 face로 분할된다. 자세한 것은 알고리즘을 참조.

 * \note
 코드가 매우 복잡하므로 알고리즘과 함께 보아야 한다. 또한 이 함수에서는 분할 과정에서
 분할을 통해 생성되는 vertex 및 edge들의 min, max 값 등 속성을 결정하게 된다.

 * \param f			분할할 face 포인터
 * \param pSrcV		분할에 사용할 vertex 포인터
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID 
ZMesh::DivideTri( ZFace* f, ZVert* pSrcV )
{
	ZEdge* e = f->edge;

	// 분할을 통해 6개의 새로운 엣지와 2개의 face(생성되는 face는 3개가 아니라 2개이다. 하나는 분할 전의 원래 face), 1개의 vertex가 생성된다.
	// ne = new edge, nf = new face
	ZEdge* ne = new ZEdge[6];
	ZFace* nf = new ZFace[2];
	ZVert* pNewVert = new ZVert( pSrcV->type, pSrcV->pos );

	// face f의 세 vertex를 v 배열에 넣음
	ZVert* v[3];
	v[0] = e->v1;
	v[1] = e->next->v1;
	v[2] = e->next->next->v1;

	// face의 세 vertex를 각각 끝점으로 갖도록 엣지들을 설정한다.
	for( int i=0; i < 5; i += 2 ) 
	{
		ne[i].v2 = ne[i+1].v1 = v[i/2];
		ne[i].v1 = ne[i+1].v2 = pNewVert;
	}

	// 엣지들의 face를 설정한다
	ne[0].f = ne[3].f = e->f = f;
	ne[1].f = ne[4].f = e->next->next->f = &nf[1];
	ne[2].f = ne[5].f = e->next->f = &nf[0];

	// 엣지들의 twin 설정. 
	for( int i=0; i < 5; i += 2 ) 
	{
		ne[i].twin = &ne[i+1];
		ne[i+1].twin = &ne[i];
	}	

	// boundary next 엣지들을 설정. 삼각형 f가 e1, e2, e3로 구성된다면 e1의 next는 e2이다.
	ne[0].next = e;
	ne[1].next = &ne[4];
	ne[2].next = e->next;
	ne[3].next = &ne[0];
	ne[4].next = e->next->next;
	ne[5].next = &ne[2];

	e->next->next->next = &ne[1];
	e->next->next = &ne[5];
	e->next = &ne[3];

	// 새로운 vertex에 그것으로부터 뻗어나가는 3개의 엣지를 추가한다
	// 또 삼각형의 세 꼭지점에 그것으로부터 뻗어나가는 1개씩의 엣지를 추가
	for( int i=0; i < 5; i += 2 ) 
	{
		pNewVert->edges.Add( &ne[i] );
		v[i/2]->edges.Add( &ne[i+1] );
	}

	// vertex, face를 배열에 추가
	m_pFaces[ GetIndexOfSubsetID(f->m_dwSubsetID) ].Add( &nf[0] );
	m_pFaces[ GetIndexOfSubsetID(e->twin->f->m_dwSubsetID) ].Add( &nf[1] );
	m_verts.Add( pNewVert );

	// edge를 배열에 추가
	for( int i=0; i < 6; i++ ) 
	{
		ne[i].type = pSrcV->type == ZTYPE_SRC ? ZTYPE_TGT : ZTYPE_SRC;		
		m_edges.Add( &ne[i] );
	}

	f->edge = e;
	nf[0].edge = ne[2].next;
	nf[1].edge = ne[4].next;

	// pSrcV와 pNewVert는 서로 대응 관계이다. 둘은 서로 포지션이 동일하며, 하나는 소스 메시에, 하나는 타겟 메시에 존재한다.
	// 대응 vertex 포인터를 저장해 두어야 나중에 다른 계산에 사용할 수 있다
	pSrcV->correspond = pNewVert;
	pNewVert->correspond = pSrcV;

	// UV 보간을 위한 사전 작업
	ZEdge* boundE[3];
	boundE[0] = e;
	boundE[1] = e->next->twin->next;
	boundE[2] = e->next->twin->next->next->twin->next;

	// face f에 생성된 vertex의 포지션 벡터를 쏴서 barycentric 보간에 사용할 UV값을 얻는다.
	FLOAT baryU, baryV, d;	
	D3DXIntersectTri( &v[0]->pos, &v[1]->pos, &v[2]->pos, &D3DXVECTOR3(0,0,0), &pNewVert->pos, &baryU, &baryV, &d );

	// 얻은 UV값을 가지고 barycentric 보간을 하여 f 내에 포함될 pNewVert의 포지션을 정한다.
	// 결과적으로 pNewVert는 face f와 동일 평면상에 놓이게 된다.
	D3DXVECTOR3 pos;
	D3DXVec3BaryCentric( &pos, &v[0]->pos.org, &v[1]->pos.org, &v[2]->pos.org, baryU, baryV );
	pNewVert->pos.org = pos;
	pNewVert->pos.max = pSrcV->pos.org;	
	pSrcV->pos.max = pos;	

	// 마찬가지로 얻은 UV값을 가지고 매핑좌표와 노멀 벡터 보간을 수행한다.
	// pNewVert의 포지션에서 어떤 노멀과 매핑좌표 값을 가질지를 보간을 통해 계산하게 된다.
	D3DXVECTOR2 uv;
	D3DXVECTOR3 normal;
	D3DXVec2BaryCentric( &uv, &boundE[0]->texcoord.org, &boundE[1]->texcoord.org, &boundE[2]->texcoord.org, baryU, baryV );	
	D3DXVec3BaryCentric( &normal, &boundE[0]->normal.org, &boundE[1]->normal.org, &boundE[2]->normal.org, baryU, baryV );

	// 계산한 것들을 가지고 엣지들의 노멀 및 매핑좌표를 설정한다
	for( int i=0; i < 6; i += 2 )
	{
		ne[i].texcoord.org = uv;
		ne[i].normal.org = normal;
		ne[i+1].texcoord.org = boundE[i/2]->texcoord.org;
		ne[i+1].normal.org = boundE[i/2]->normal.org;
	}

	// 이건 디버깅용 //////////////
	if( !(m_bHasTexture && m_pCorrespond->m_bHasTexture) ) return;

	/************************************************************************/
	/* 어려운 부분                                                          */
	/************************************************************************/

	//////////////////////////////////////////////////////////////////////////
	// divide 과정에서 반드시 <새로 생성되는 모든 엣지>의 노멀 및 매핑좌표를 결정해야 한다.
	// pNewVert로부터 뻗어나가는 세 엣지의 경우 barycentric 보간된 값들을 사용하면 간단하므로 문제될 것이 없으나
	// 삼각형의 세 꼭지점으로부터 pNewVert로 향하는 세 엣지의 경우 그 값을 결정하기가 상당히 까다롭다
	// 여기서 사용하는 방법은 완전하지 않다. 알고리즘 참조.
	//////////////////////////////////////////////////////////////////////////
	ZEdge *pCCW1, *pCCW2, *pCCW3;
	pCCW1 = ne[0].FindClosestCCWEdge( &ne[4], pSrcV );
	pCCW2 = ne[2].FindClosestCCWEdge( &ne[0], pSrcV );
	pCCW3 = ne[4].FindClosestCCWEdge( &ne[2], pSrcV );
	ne[0].normal.max = pCCW1 ? pCCW1->normal.org : pCCW3->normal.org;
	ne[2].normal.max = pCCW2 ? pCCW2->normal.org : pCCW1->normal.org;
	ne[4].normal.max = pCCW3 ? pCCW3->normal.org : pCCW2->normal.org;
	ne[0].texcoord.max = pCCW1 ? pCCW1->texcoord.org : pCCW3->texcoord.org;
	ne[2].texcoord.max = pCCW2 ? pCCW2->texcoord.org : pCCW1->texcoord.org;
	ne[4].texcoord.max = pCCW3 ? pCCW3->texcoord.org : pCCW2->texcoord.org;

	ZEdge* pEdge = NULL;
	ZVert vTemp;

	for( int i=0; i < 5; i += 2 )
	{
		if( (pEdge = pSrcV->GetEdgeToVertex(v[i/2])) != NULL ) 
		{
			ne[i].normal.max = pEdge->normal.org;
			ne[i].texcoord.max = pEdge->texcoord.org;
			ne[i+1].normal.max = pEdge->twin->normal.org;
			ne[i+1].texcoord.max = pEdge->twin->texcoord.org;
		}
		else
		{		
			vTemp.pos = pNewVert->pos + 0.9 * (v[i/2]->pos - pNewVert->pos);
			pEdge = FindEdgeOfContainFace( v[i/2], &vTemp );
			if( pEdge ) 
			{
				ne[i+1].normal.max = pEdge->normal.org;
				ne[i+1].texcoord.max = pEdge->texcoord.org;		
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 vContain을 포함하는 face의 바운더리 엣지들 중에서 그것의 v1이 vCorner인 엣지를 찾아 반환

 이 함수는 DivideTri에서 노멀 및 매핑좌표를 결정하는데 쓰인다.
 내용은 그림을 참조해야 하므로 알고리즘을 참조한다.

 * \param vCorner	분할되는 소스 face의 세 꼭지점 중 하나의 포지션 벡터. 타겟 face 중에서 이것을 포함하는 face가 있으면 그 face의 엣지의 uv, normal 값을 이용하여 새로 생성된 edge의 값을 결정할 수 있다.
 * \param vContain	포함 face를 찾기 위한 vertex 포지션 벡터. 상대 메시의 face중에서 이것을 포함하는 face를 찾는다.
 * \return			찾은 엣지의 포인터
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZEdge* 
ZMesh::FindEdgeOfContainFace( ZVert* vCorner, ZVert* vContain )
{
	ZFace* pF = m_pCorrespond->FindFaceContainsVertex( vContain, ZTYPE_ANY );
	if( !pF ) return NULL;

	ZEdge* pEdge = pF->edge;
	for( int i=0; i < 3; i++, pEdge = pEdge->next )
		if( pEdge->v1->pos == vCorner->pos ) return pEdge;

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 단순히 질량 중심 벡터를 반환한다. 계산은 Create 함수에서 이미 해 놓았음.

 * \param VOID	없음
 * \return		질량 중심 벡터
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DXVECTOR3
ZMesh::GetCenterOfMass( VOID )
{
	return m_vCMass;	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 질량 중심을 기준으로 모든 vertex 코디네이션을 재조정하는 함수
 
 메시의 모든 vertex들은 원본 메시의 좌표값들을 가진다. 이 벡터들을 질량 중심을 기준으로 바꾼다.
 이것을 하는 이유는 메시의 모핑 시에 대응되는 상대 메시로 자연스럽게 이동하도록 하기 위해서 등의 이유이다.

 또 내접 구의 반지름을 반환하여 이 값을 통해 메시를 노멀라이즈 할 수 있도록 한다.

 * \param vCMass	질량 중심 벡터
 * \return			변환된 코디네이션의 vertex들이 이루는 이 메시의 내접 구의 반지름
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT
ZMesh::ChangeCoordsByCMass( D3DXVECTOR3 vCMass )
{
	FLOAT radius = 0, temp = 0;

	ZVert** pV = m_verts.GetData();
	for( DWORD i=0; i < m_verts.GetSize(); i++ )
	{	
		pV[i]->pos -= vCMass;
		pV[i]->pos.org -= vCMass;
		D3DXVec3Normalize( &pV[i]->pos, &pV[i]->pos );
		pV[i]->pos.min = pV[i]->pos;
		temp = D3DXVec3Length( &pV[i]->pos );
		if( temp > radius ) radius = temp;
	}	
	return radius;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 정사면체 꼭지점들을 이용하여 앵커 vertex들 중 4개를 고정시키는 함수

 알고리즘은 Alexa의 논문대로이다. 정사면체의 네 꼭지점과 가장 가까운 네 vertex를 고정시켜 놓고
 relax를 수행하게 된다. 고정된 정점 없이 relax를 하게 되면 필연적으로 collapse 되며, relax 알고리즘 자체가 인접 정점들의 포지션의 평균값으로 vertex를 이동시키는 
 것이기 때문에 고정 정점이 없이 하면 말이 되지 않는다.(무한히 이동하게 됨)

 * \param vTetraVerts[4]	정사면체의 네 꼭지점 벡터 배열
 * \param Anchors			앵커가 될 vertex 배열 객체. 여기에 fix된 vertex들을 넣는다.
 * \return					없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::FixVertices( D3DXVECTOR3 vTetraVerts[4], CGrowableArray<ZVert*>& Anchors )
{
	FLOAT fMin, fDistance = 0;
	DWORD minIndex = 0;	

	ZVert** pV = m_verts.GetData();
	for( int i=0; i < 4; i++ )
	{
		fMin = 1000000;
		for( DWORD j=0; j < m_verts.GetSize(); j++ )
		{
			// 정사면체 네 꼭지점에서 가장 가까운 vertex를 찾는다
			if( (fDistance = D3DXVec3Length( &(vTetraVerts[i] - pV[j]->pos) )) < fMin ) 
			{
				fMin = fDistance;
				minIndex = j;
			}
		}			

		// 찾은 vertex를 fix시키고 앵커 배열에 넣음
		pV[minIndex]->fixed = TRUE;
		Anchors.Add( pV[minIndex] );		
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief Embedding이 collapse 되었는지 여부를 판별하는 함수

 relax 과정을 보면 임의의 정사면체를 사용하여 vertex 4개를 고정시키고 relax를 되풀이하는데, 이 고정 점을 잘못 잡으면 embedding이 collapse되게 된다.
 논문의 알고리즘을 참조하도록 한다. 논문에서는 collapse 되었다고 판단되면 알고리즘을 재시작하여 새로운 랜덤 정사면체를 잡고 다시 릴렉스를 수행하게 된다.
 relax 알고리즘상 collapse를 원천적으로 방지할 방법이 마땅치 않음. 따라서 논문에서 이와 같은 휴리스틱 판별 알고리즘을 통해 collapse 판단시 알고리즘을 재시작함.

 판별 기준은 정사면체 네 꼭지점의 diametric vertex 즉 정사면체 중심을 기준으로 꼭지점의 반대편 지점과 가장 가까운 embedding의 vertex 사이의 거리가
 정사면체 형태로 고정한 앵커 vertex들 중에서 가장 가까운 두 앵커 vertex 사이의 거리의 1/2 보다 크거나 같으면 collapse 되었다고 판단한다.

 쉽게말해, 네 고정 정점 주위에 적당히 인접한 vertex가 하나도 없다는것은 vertex들이 한쪽으로 쏠렸다는 것이므로 collapse 될것으로 판단.

 * \param vTetraVerts[4]	정사면체 꼭지점 벡터 배열
 * \param fBetweenAnchors	앵커 vertex들 중 가장 가까운 pair 간의 거리
 * \return					TRUE면 collapse됨, FALSE면 반대
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL
ZMesh::CheckCollapsed( D3DXVECTOR3 vTetraVerts[4], FLOAT fBetweenAnchors )
{
	FLOAT fMin;
	FLOAT fDistance = 0;
	DWORD minIndex = 0;

	ZEdge** pEdges;	
	ZVert** pV = m_verts.GetData();

	for( int i=0; i < 4; i ++ )
	{
		fMin = 1000000;		
		for( DWORD j=0; j < m_verts.GetSize(); j++ )
		{		
			if( pV[j]->fixed == FALSE && (fDistance = D3DXVec3Length( &(-vTetraVerts[i] - pV[j]->pos))) < fMin )
			{
				fMin = fDistance;
				minIndex = j;
			}
		}

		// diametric vertex의 인접 정점들을 검사하여 거리가 충분히 짧은지 확인
		pEdges = pV[minIndex]->edges.GetData();
		for( DWORD j=0; j < pV[minIndex]->edges.GetSize(); j++ )
		{
			if( D3DXVec3Length( &(pEdges[j]->v2->pos - pEdges[j]->v1->pos) ) >= fBetweenAnchors / 2.f ) return TRUE;			
		}
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 릴랙스 함수

 이 함수는 모든 vertex들의 포지션을 해당 vertex와 엣지로 연결된 인접 vertex들의 포지션의 평균으로 이동시킨다.
 이 과정을 인자 epsilon을 기준으로 반복하는데, 한번의 릴렉스 과정을 통해 이동한 모든 vertex의 이동 거리 중 가장 큰 값이 epsilon보다 작아질때까지 반복한다.
 즉 어느정도 에너지 평형상태에 놓이면 루프를 그만두는데 이것을 epsilon을 통해 한계점을 지정하는 것이다.

 * \note
 Alexa의 이 알고리즘은 완전하지 않다. epsilon을 사용하는 것 자체가 무한루프에 빠질 수 있는 빌미를 제공한다.
 epsilon 값은 임의로 지정을 해야 하는데, 이 값을 결정하기가 쉽지 않다. 예를 들어 vertex가 많은 매우 복잡한 메시의 경우 vertex들이 밀집되어 있는 지역에서는
 relax를 통해 아주 작은 이동을 하였더라도 아직 완전히 relax 과정이 끝난 것이 아닐 수 있기 때문에 충분히 작은 epsilon 값을 설정하여야 한다. 따라서 알고리즘의
 무한 반복을 막기 위해서는 신중한 epsilon 값의 추정(신중하게 하여도 완벽할수는 없을것임)과 적절히 루프 수에 제한을 두는 등의 장치가 필요하다.

 어쨌든 Alexa의 relax 기법을 사용한 parameterization 알고리즘은 근본적으로 문제가 있으므로 다른 parameterization 방법이 있으면 그것으로 대체하는 것이 좋다.
 성능 측면에서도 이 relax 방식은 매우 큰 비용을 요구한다. 

 * \param fEpsilon	한계치 값. 이 값보다 vertex의 최대 이동거리가 작아지면 수행을 멈춘다.
 * \return			최대로 많이 이동한 vertex의 이동 거리
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DOUBLE
ZMesh::RelaxVertices( DOUBLE fEpsilon )
{
	ZVert** pV = m_verts.GetData();
	ZEdge** pE;

	DOUBLE fMaxMovement = 0, fPrevMaxMovement = -1, fMovement;
	int numNeighbors = 0;	
	int limit = 0;
	D3DXVECTOR3 vTemp;

	do
	{
		fMaxMovement = 0;
		for( DWORD i=0; i < m_verts.GetSize(); i++ )
		{
			vTemp.x = vTemp.y = vTemp.z = 0;
			numNeighbors = pV[i]->edges.GetSize();
			pE = pV[i]->edges.GetData();

			// 인접 vertex들의 포지션 평균값을 구한다
			for( int j=0; j < numNeighbors; j++ )
			{
				vTemp += pE[j]->v2->pos;
			}					
			D3DXVec3Normalize( &vTemp, &vTemp );

			// 고정 정점은 relax하지 않는다. 고정되지 않은것만 이동시킴
			if( pV[i]->fixed == FALSE )
			{
				// 최대 이동량을 갱신
				fMovement = D3DXVec3Length( &(vTemp - pV[i]->pos) );
				if( fMovement > fMaxMovement ) fMaxMovement = fMovement; 

				// vertex의 포지션을 구한 평균 위치로 이동시킨다
				pV[i]->pos = vTemp;			
			}					
		}	
		//if( ++limit > 10000 ) return -1;

		//GetCTmain()->ResetBuffersFromMorphMesh( (CtObjectBase*)m_pUserData );
		//GetCTmain()->Render();
		
		if( fPrevMaxMovement == fMaxMovement ) break;
		fPrevMaxMovement = fMaxMovement;

	} while( fMaxMovement > fEpsilon );
	
	return fMaxMovement;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시의 모든 face들이 모두 제대로 된 방향을 가진 vertex로 구성되었는지 여부를 판별하는 함수

 relax를 반복하면서 이 함수를 통해 계속 검사를 하게 된다. 이 함수를 통과하면 비로소 embed가 완료된다.
 적당히 relax되어 구형의 embedding이 형성되었더라도, face를 이루는 vertex들이 제대로 된 위치에 있지 않으면 나중에 merge시킬때 제대로 되지 않는다.
 제대로 된 위치라 함은, 간단히 설명해서 구형을 이루는 모든 face들의 노멀 방향이 모두 구의 바깥쪽 방향을 향하도록 vertex들이 위치하여야 한다는 것이다.
 그렇지 않은 경우는 face가 일부 겹쳐져 있거나 접혀있는 형태를 띄게 된다.

 * \param VOID	없음
 * \return		TRUE면 통과, FALSE면 실패
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL
ZMesh::CheckVertexOrientation( VOID )
{
	ZFace** pF;
	ZEdge** pE;
	ZEdge* pEdge;

	D3DXVECTOR3 vFaceVerts[3], vTemp;
	int sgn = 0, sgnFirst = 0, sgnSum = 0;
	FLOAT fTemp = 0;
	BOOL result = TRUE;

	for( int i=0; i < m_dwNumSubset; i++ )
	{
		pF = m_pFaces[i].GetData();	
		for( DWORD j=0; j < m_pFaces[i].GetSize(); j++ )
		{
			pEdge = pF[j]->edge;
			for( int k=0; k < 3; k++, pEdge = pEdge->next )
				vFaceVerts[k] = pEdge->v1->pos;

			// face의 orientation 적합 여부를 판단하는 알고리즘
			// Alexa 논문을 참조. 내적과 외적을 사용하여 판단한다.
			D3DXVec3Cross( &vTemp, &vFaceVerts[0], &vFaceVerts[1] );
			fTemp = D3DXVec3Dot( &vTemp, &vFaceVerts[2] );

			if( fTemp > 0 ) sgn = 1;
			else if( fTemp == 0 ) sgn = 0;
			else sgn = -1;				

			sgnSum += sgn;

			if( j == 0 ) sgnFirst = sgn;
			else if( sgn != sgnFirst ) result = FALSE;
		}
	}
	
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief Spherical Parameterization 메인 함수

 embed 알고리즘 메인 함수이다. 이 메인 알고리즘은 Alexa의 논문에 소개된 그대로이다. 
 원본 메시를 구형의 메시로 parameterize 시킨다. 논문 및 주석을 참조.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::EmbedToSphere( VOID )
{
	//////////////////////////////////////////////////////////////////////////
	//
	// * Alexa et al. Sphere parameterization 알고리즘 *
	//
	// 1. 바운더리 스피어를 구한다
	// 2. 질량중심을 중심으로 모든 정점 코디를 재조정한 뒤 
	// 3. 바운더리 스피어가 유닛 스피어가 되도록 하는만큼 모든 정점을 스케일링한다
	// 4. 스피어상에 랜덤 레귤러 테트라히드론을 만들고 그것과 가장 가까운 정점들을 고정한다
	// 5. 메쉬의 모든 정점들을 정점의 이웃정점들을 평균한 값으로 릴랙스시킨다
	//    릴랙스 과정은 정점들의 이동거리의 중 최대치가 입실론보다 작아질때까지 한다
	// 6. 만약 임베딩이 콜랩스되면 4번과정으로 돌아가는데 콜랩스 여부는 테트라히드론을 잇는 다이아메트릭 모델까지의
	//    가장 가까운 거리를 재서 이것이 앵커들 사이의 거리의 반보다 작은 경우 콜랩스되었다고 판단
	// 7. 4번과정에서와 같이 테트라히드론의 다이아메트릭 까지 가장 가까운 정점들을 고정한다
	// 8. 5번과정을 반복하여 릴랙스.
	// 9. 모든 face의 세 정점들을 검사해서 sgn(v0 외적 v1 을 v2와 내적한 값)이 모두 같지 않으면 입실론을 줄이고 
	//    8번과정을 반복한다. 모두 같아질때까지 계속한다.
	//
	//////////////////////////////////////////////////////////////////////////

	srand( time(NULL) );

	//////////////////////////////////////////////////////////////////////////
	//
	// Step 1, 2, 3
	// 
	// 메쉬를 단위 스피어 내부에 내접시킨다.
	// 바운더리 스피어를 구하고 메쉬의 중심을 원점으로 모든 정점 위치를 재조정한 뒤
	// 바운더리 스피어가 유닛 스피어가 되도록 하는 만큼 모든 정점을 uniform 스케일링한다
	// 
	//////////////////////////////////////////////////////////////////////////
	CGrowableArray<ZVert*> Anchors;

	D3DXVECTOR3 vCMass = GetCenterOfMass();
	FLOAT fRadius = ChangeCoordsByCMass( vCMass );	

	//////////////////////////////////////////////////////////////////////////
	//
	// Step 4, 5, 6
	// 
	// 바운더리 스피어상에 랜덤 레귤러 테트라히드론을 만들고 그것과 가장 가까운 정점들을 고정한다
	//
	// 메쉬의 모든 정점들을 정점의 이웃정점들을 평균한 값으로 릴랙스시킨다
	// 릴랙스 과정은 정점들의 이동거리의 중 최대치가 입실론보다 작아질때까지 한다
	//
	// 만약 임베딩이 콜랩스되면		4번과정으로 돌아가는데, 콜랩스 여부는 테트라히드론을 잇는 다이아메트릭 모델까지의
	// 가장 가까운 거리를 재	서 이것이 앵커들 사이의 거리의 반보다 작은 경우 콜랩스되었다고 판단한다
	//
	//////////////////////////////////////////////////////////////////////////

	// 정사면체를 스피어상에 만든다
	D3DXVECTOR3 vTetraVerts[4], vTetraInverse[4];
	DOUBLE fEpsilon = EPSILON;	
	DOUBLE fPrevEpsilon;
	FLOAT fBetweenAnchors = 0;

	ZVert** pV = m_verts.GetData();

restart:
	while( 1 )
	{
		MakeRandomRegularTetraHedron( vTetraVerts );
		for( int i=0; i < 4; i++ ) vTetraInverse[i] = -vTetraVerts[i];

		// 정사면체와 가까운 정점을 고정시킨다	
		ResetFixedVertices( Anchors );
		FixVertices( vTetraVerts, Anchors );	

		// Anchor들간의 거리중 가장 짧은것을 구해둔다		
		fBetweenAnchors = GetMinDistanceOfVertexPairs( Anchors );

		RelaxVertices( fEpsilon ); 

		if( CheckCollapsed( vTetraVerts, fBetweenAnchors ) == TRUE )
		{
			for( DWORD i=0; i < m_verts.GetSize(); i++ ) 
				pV[i]->pos = pV[i]->pos.min;						
		}
		else break;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// Step 7
	//
	// 4번 과정에서의 정사면체의 정점들의 diametric에 가까운 정점들을 고정시킨다
	//
	//////////////////////////////////////////////////////////////////////////

	ResetFixedVertices( Anchors );
	FixVertices( vTetraInverse, Anchors );
	fBetweenAnchors = GetMinDistanceOfVertexPairs( Anchors );

	//////////////////////////////////////////////////////////////////////////
	//
	// Step 8, 9
	//
	// 릴랙스를 한 뒤 
	// 모든 face의 세 정점들을 검사해서 sgn(v0 외적 v1 을 v2와 내적한 값)이 모두 같지 않으면 입실론을 줄이고 
	// 반복한다. 모두 같아질때까지 계속한다.
	//
	//////////////////////////////////////////////////////////////////////////

	do {		
		fPrevEpsilon = fEpsilon;
		fEpsilon = RelaxVertices( fEpsilon ); 
		if( CheckCollapsed(vTetraVerts, fBetweenAnchors) == TRUE )
		{
			goto restart;
		}
	} while( CheckVertexOrientation() == FALSE );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시 노멀 계산 함수

 이 함수는 사용하지 않으나 준비해 둠. 메시의 노멀을 계산하는 것은, 메시를 이루는 vertex들의 노멀을 결정하는 것이다.(ZMesh의 경우 엣지에 노멀을 할당하므로 엣지의 노멀을 계산한다고 해야하나)
 vertex들의 노멀은, 그것을 공유하는 모든 인접 face들의 노멀을 평균한 값으로 한다. face의 노멀은 그것을 포함하는 평면의 노멀과 같다.

 이렇게 노멀을 계산하는 것이 일반적이지만, 모핑 과정에서는 노멀 벡터가 계산에 의한 것이 아니라 보간되는 것이므로 따로 노멀을 계산하지는 않는다.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::CalcNormals( VOID )
{	
	D3DXVECTOR3 e1, e2, vNormal( 0,0,0 ), vTemp;
	DWORD numEdges = 0;

	ZEdge** pE;
	ZVert** pV = m_verts.GetData();
	for( DWORD i=0; i < m_verts.GetSize(); i++ )
	{
		pE = pV[i]->edges.GetData();
		numEdges = pV[i]->edges.GetSize();
		for( DWORD j=0; j < numEdges; j++ )
		{
			e1 = pE[j]->v2->pos - pE[j]->v1->pos;
			if( j == numEdges - 1 ) 
				e2 = pE[0]->v2->pos - pE[0]->v1->pos;
			else 
				e2 = pE[j+1]->v2->pos - pE[j+1]->v1->pos;
			D3DXVec3Cross( &vTemp, &e1, &e2 );
			D3DXVec3Normalize( &vTemp, &vTemp );
			vNormal += vTemp;
		}
		vNormal /= numEdges;
		D3DXVec3Normalize( &vNormal, &vNormal );
		for( DWORD j=0; j < numEdges; j++ ) pE[j]->normal = vNormal;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 임의의 정사면체를 만들어 네 꼭지점 벡터를 배열에 넣는 함수

 이 함수에서는 가장 간단한 정사면체를 지정한 다음 XYZ 축으로 랜덤하게 회전시킨다.

 * \param vArray[4]		정사면체 꼭지점 벡터들의 배열
 * \return				없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID 
ZMesh::MakeRandomRegularTetraHedron( D3DXVECTOR3 vArray[4] )
{
	// 가장 간단한 정사면체
	D3DXVECTOR3 vTetra[4];
	vTetra[0] = D3DXVECTOR3( 0,0,0 );
	vTetra[1] = D3DXVECTOR3( 0,1,1 );
	vTetra[2] = D3DXVECTOR3( 1,0,1 );
	vTetra[3] = D3DXVECTOR3( 1,1,0 );

	// 랜덤하게 회전한다	
	D3DXVECTOR3 vAxis[4];
	FLOAT fRandAngle;

	for( int i=0; i < 4; i++ )
	{
		vTetra[i] -= D3DXVECTOR3( 0.5f, 0.5f, 0.5f );		
		D3DXVec3Normalize( &vTetra[i], &vTetra[i] );		
		vAxis[i] = vTetra[i];
	}

	for( int i=0; i < 4; i++ )
	{
		fRandAngle = DEGREE_TO_RADIAN( rand() % 360 );
		for( int j=0; j < 4; j++ ) RotateVectorByAxis( &vTetra[j], &vTetra[j], vAxis[i], fRandAngle );		
	}

	for( int i=0; i < 4; i++ ) vArray[i] = vTetra[i];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 고정 정점들의 고정을 해제

 * \param Anchors	고정 정점들의 배열
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID 
ZMesh::ResetFixedVertices( CGrowableArray<ZVert*>& Anchors )
{
	ZVert** pV = Anchors.GetData();
	for( DWORD i=0; i < Anchors.GetSize(); i++ )
	{
		pV[i]->fixed = FALSE;
	}
	Anchors.RemoveAll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 vertex 집합에서, 가장 가까운 vertex pair를 얻어 그 사이 거리를 반환한다

 이 값은 collapse 여부를 판별하는 CheckCollapsed() 함수에서 쓴다.

 * \param verts		vertex 배열
 * \return			최근접 거리 값
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT
ZMesh::GetMinDistanceOfVertexPairs( CGrowableArray<ZVert*>& verts )
{
	ZVert** pAnchors = verts.GetData();
	FLOAT fMin = 1000000;
	FLOAT fDistance = 0;
	for( DWORD i=0; i < verts.GetSize(); i++ )
	{
		for( DWORD j=i+1; j < verts.GetSize() - 1; j++ )
		{
			if( (fDistance = D3DXVec3Length( &(pAnchors[i]->pos - pAnchors[j]->pos) )) < fMin )
			{
				fMin = fDistance;
			}
		}
	}
	return fMin;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 타겟 메시를 통해 소스 메시와 합병하는 함수

 타겟 메시의 모든 엣지들을 소스 메시의 엣지들과 교차 판정하여 교차하는 모든 소스 엣지들을 cut시킨다
 결과적으로 이미 Divide 과정을 거쳤던 소스 메시는 이 과정을 끝으로 소스 및 타겟 어느쪽으로든 옮겨갈 수 있는 토폴로지를 갖게 된다.

 * \param pTgtMesh	타겟 메시 포인터
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::MergeWith( ZMesh* pTgtMesh )
{
	//////////////////////////////////////////////////////////////////////////
	//
	// intersection find 일주
	//
	//////////////////////////////////////////////////////////////////////////

	stack<ZEdge*> EdgeStack;

	ZVert **pV, *pNewVert;
	ZEdge *pE, **pEdges, *pBoundEdge;
	ZEdge *pLastIntersectedTgtEdge = NULL;
	ZFace *pF, *pStartFace, *pNewFaces;

	// 스택에 검사 edge와 face들을 push (너비우선탐색) : 첫번째 v에 대해서 시작
	pV = pTgtMesh->m_verts.GetData();
	for( DWORD i=0; i < pTgtMesh->m_verts.GetSize(); i++ )
	{
		pEdges = pV[i]->edges.GetData();
		if( pV[i]->correspond->FindEdgeIntersect( pEdges[0] ) != NULL )
		{
			pEdges[0]->marked = pEdges[0]->twin->marked = TRUE;
			EdgeStack.push( pEdges[0] );
			break;
		}
	}

	while( EdgeStack.empty() == FALSE )
	{
		// 스택에서 검사 edge, face를 pop
		pE = EdgeStack.top();
		EdgeStack.pop();

		// 디버깅
		//ZFeature* feature = new ZFeature;
		//feature->vf[0] = pE->v1;
		//feature->vf[1] = pE->v2;

		// 검사 엣지의 시작점으로부터 처음 교차하는 엣지를 구함 
		pBoundEdge = pE->v1->correspond->FindEdgeIntersect( pE );

		// 엣지의 끝점에 이를때까지 교차하는 엣지들을 구해 컷트한다
		while( pBoundEdge )
		{
			if( pE->CheckIntersect( pBoundEdge, &pNewVert ) == TRUE )
			{
				// 디버깅 //////////////////////////////////////////////////
				//GetCTmain()->m_pMorphingSrcObj->m_pFeatures.Add( pNewVert );

				// 교점을 저장한다		
				m_verts.Add( pNewVert );		

				// 교차하는 엣지를 컷트한다
				pBoundEdge = CutEdge( pBoundEdge, pNewVert );
			}
			else break;
		}

		// 이제 검사 face는 검사 edge의 끝점을 포함한다. edge의 끝점으로부터 시작하는 표시 안된 엣지들과 그것들의 시작점을
		// 포함하는 face들을 스택에 넣는다
		pEdges = pE->v2->edges.GetData();
		for( DWORD i=0; i < pE->v2->edges.GetSize(); i++ )
		{			
			if( pEdges[i]->marked == FALSE )
			{
				// 검사 엣지 e와 그 twin에 표시를 해둠
				pEdges[i]->marked = pEdges[i]->twin->marked = TRUE;
				EdgeStack.push( pEdges[i] );				
			}
		}		
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 현재 메시 값을 min 값으로 지정하는 함수

 모핑을 위한 초기값 min 값을 현재 값을 사용하여 지정한다. 모든 vertex 및 edge 속성을 min 값에 지정한다.

 * \param VOID	없음	
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::SetMin( VOID )
{
	ZVert** pV = m_verts.GetData();
	for( DWORD i=0; i < m_verts.GetSize(); i++ )
		pV[i]->pos = pV[i]->pos.min = pV[i]->pos.org;

	ZEdge** pE = m_edges.GetData();
	for( DWORD i=0; i < m_edges.GetSize(); i++ )
	{
		pE[i]->normal = pE[i]->normal.org;
		pE[i]->texcoord = pE[i]->texcoord.org;
		pE[i]->SetMin();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 보간 함수

 min, max 값 사이를 alpha 값을 사용하여 보간하여 현재 값에 지정한다

 * \param alpha		보간 값. 0~1 사이
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::Interpolate( FLOAT alpha )
{
	// vertex 포지션, 노멀 및 텍스쳐 좌표를 보간한다
	ZEdge** pE = m_edges.GetData();
	for( DWORD i=0; i < m_edges.GetSize(); i++ )
	{
		pE[i]->v1->pos.Interpolate( alpha );
		pE[i]->normal.Interpolate( alpha );
		pE[i]->texcoord.Interpolate( alpha );
	}
	
	// 머티리얼 값 보간
	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		ZMaterial* pM = &m_pMaterials[i];
		pM[i].Interpolate( alpha );
	}	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/** 
 * \brief 타겟 메시를 통해 소스 메시를 분할한다

 이 함수에서는 소스 메시의 모든 face를 타겟 메시의 모든 vertex로 분할한다.

 * \param pTgtMesh	타겟 메시 포인터
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::DivideMeshFrom( ZMesh* pTgtMesh )
{
	ZVert** pV;
	ZFace* pF;

	pV = m_pCorrespond->m_verts.GetData();
	for( DWORD i=0; i < m_pCorrespond->m_verts.GetSize(); i++ )
	{
		// 타겟 vertex를 구면상에서 포함하는 소스 face를 찾아 분할한다		
		pF = FindFaceContainsVertex(pV[i], ZTYPE_ANY);
		if( !pF ) 
		{
			pV[i]->pos.max = pV[i]->correspond->pos.max;
		}
		else DivideTri( pF, pV[i] );
	}
	
	// 반대로 소스 vertex들을 포함하는 타겟 메시의 face들을 찾는다. 단 분할하지는 않는다.
	// 어떤 vertex에 대한 포함 face 정보를 가지고 있어야 나중에 계산할 때 쓸수 있음
	pV = m_verts.GetData();
	for( DWORD i=0; i < m_verts.GetSize(); i++ )
		if( pV[i]->correspond == NULL ) pTgtMesh->FindFaceContainsVertex( pV[i], ZTYPE_ANY );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 vertex를 구면상에서 포함하는 face를 상대 메시에서 찾아 반환한다

 구면상에서 포함한다는 것은, 쉽게말해 해당 vertex의 포지션을 향하는 ray를 쏴서 교차하는 face가 있으면 그것이 포함한다고 보면 된다.
 만약 embedding이 제대로 되었다면, 어떠한 vertex이건 간에 그것을 포함하는 상대 embedding의 face가 존재할수밖에 없다.(완전히 구형 폐곡면을 이루는 메시이므로 구의 중심에서 ray를 쏘면 어떤 face든 맞게 된다)

 * \note
 face를 찾지 못하는 경우가 한가지 존재한다. embed가 제대로 되지 않았을 경우를 제외하면,
 주어진 vertex의 포지션이 상대 메시의 face의 세 꼭지점 중 하나와 완전히 일치하는 경우이다. 이 경우 그 사실을 알아낼 수는 있으나
 face를 분할할수는 없다.(분할하려면 face에 포함되어야지 꼭지점 중 하나와 일치하면 분할이 불가능)

 이러한 경우는 흔치 않으나 충분히 존재할 수 있다. 따라서 이런 경우는 따로 처리해 주어 분할을 하지 않도록 해 주어야만 하고
 추가적인 처리를 해야 한다.(분할 과정을 통해 엣지의 노멀, 텍스쳐 좌표가 결정되므로)

 이러한 추가 작업은 현재 구현되어 있지 않다!

 * \param pV		검사할 vertex 포인터
 * \param type		찾을 face 종류. 지정한 종류의 face들 중에서만 찾는다
 * \return			찾은 face 포인터. 없으면 NULL 반환
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZFace*		
ZMesh::FindFaceContainsVertex( ZVert* pV, ZType type )
{	
	D3DXVECTOR3 v1, v2, v3, cross1, cross2, cross3;
	D3DXVECTOR3 vO( 0,0,0 );
	FLOAT fd;

	ZFace** pF;
	ZEdge** pE;
	ZEdge *e1, *e2, *e3;

	for( int i=0; i < m_dwNumSubset; i++ )
	{
		pF = m_pFaces[i].GetData();
		for( DWORD j=0; j < m_pFaces[i].GetSize(); j++ )
		{
			if( type != ZTYPE_ANY && pF[j]->type != type ) continue;

			// v1, v2, v3를 구한다
			e1 = pF[j]->edge;
			e2 = e1->next;
			e3 = e2->next;

			v1 = e1->v1->pos;
			v2 = e2->v1->pos;
			v3 = e3->v1->pos;

			if( pV->pos == v1 ) 
			{
				pV->correspond = e1->v1;
				return NULL;
			}
			if( pV->pos == v2 ) 
			{
				pV->correspond = e2->v1;
				return NULL;
			}
			if( pV->pos == v3 ) 
			{
				pV->correspond = e3->v1;
				return NULL;
			}

			// face가 p를 포함하는지 검사		
			D3DXVec3Cross( &cross1, &v1, &v2 );
			D3DXVec3Cross( &cross2, &v2, &v3 );
			D3DXVec3Cross( &cross3, &v3, &v1 );

			// Alexa 알고리즘은 내적을 통해 face가 vertex를 포함하는지 판별한다.
			// 그러나 IntersectTri를 통해 간단히 알 수 있다. 또 IntersectTri를 사용해야만 barycentric 보간을 위한 UV(f, g)값을 얻을 수 있다.(계산을 따로 해줘도 되겠지만)
			// 여기서는 두가지 모두를 사용한다.
			if( D3DXVec3Dot( &cross1, &pV->pos ) >= 0 && D3DXVec3Dot( &cross2, &pV->pos ) >= 0 && D3DXVec3Dot( &cross3, &pV->pos ) >= 0 )
			{
				if( D3DXIntersectTri(&v1, &v2, &v3, &vO, &pV->pos, &pV->barycentricUV.x, &pV->barycentricUV.y, &fd) == TRUE )
				{
					// 포함 face를 찾았으면 얻은 UV값을 통해 barycentric 보간을 하여 max 값 및 org 값들을 결정한다.
					
					// 포지션의 max값을 결정하는 것은, 소스 vertex가 궁극적으로 최종 이동할 목표 지점이 그것을 포함하는 타겟 face의 barycentric 보간 지점이기 때문
					D3DXVec3BaryCentric( &pV->pos.max, &e1->v1->pos.org, &e2->v1->pos.org, &e3->v1->pos.org, pV->barycentricUV.x, pV->barycentricUV.y );
					D3DXVECTOR3 no;
					D3DXVECTOR2 co;
					// 노멀 및 텍스쳐 좌표의 max 값을 결정하는 것은, 소스 face를 이루는 엣지들의 노멀 및 텍스쳐 값의 최종 max 값이 타겟 face에 포함된 엣지들의 그 값들을 barycentric 보간한 값이기 때문.
					// 즉 소스 vertex는 최종적으로 타겟 face에 숨게 되며, 그 지점의 노멀 및 매핑좌표는 그것을 포함하는 타겟 face의 세 엣지의 노멀 및 매핑좌표 값을 보간한 값으로 하는 것.
					D3DXVec3BaryCentric( &no, &e1->normal.org, &e2->normal.org, &e3->normal.org, pV->barycentricUV.x, pV->barycentricUV.y );
					D3DXVec2BaryCentric( &co, &e1->texcoord.org, &e2->texcoord.org, &e3->texcoord.org, pV->barycentricUV.x, pV->barycentricUV.y );

					pE = pV->edges.GetData();
					for( DWORD k=0; k < pV->edges.GetSize(); k++ )
					{
						pE[k]->normal.max = no;
						pE[k]->texcoord.max = co;
					}
				}
				pV->containFace = pF[j];
				return pF[j];
			}
		}

	}
		
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시의 통계 정보를 업데이트하는 함수

 vertex 및 face 수를 다시 세어 저장한다

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMesh::CalcMeshInfo( VOID )
{
	m_dwNumFaces = 0;	
	for( DWORD i=0; i < m_dwNumSubset; i++ )
		m_dwNumFaces += m_pFaces[i].GetSize();

	m_dwNumVerts = m_dwNumFaces * 3;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 주어진 엣지를 주어진 vertex로 컷트하는 함수

 미리 교차 판정을 통해 교점을 계산하고 교점을 통해 엣지를 컷트하는 함수이다.
 이 함수에서 새로 생성되는 edge 및 face들의 속성을 결정하게 된다.
 과정이 매우 복잡하므로 문서의 그림과 알고리즘을 참조한다.

 * \param e		컷트할 엣지 포인터
 * \param v		컷트하는 교점 vertex 포인터
 * \return		다음 컷트할 대상이 되는 엣지의 포인터를 반환한다. 알고리즘 참조.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZEdge*
ZMesh::CutEdge( ZEdge* e, ZVert* v )
{
	//////////////////////////////////////////////////////////////////////////
	//
	// * 교차 엣지를 컷트하는 알고리즘 *
	//
	// 여기서는 컷트와 함께 triangular 과정도 수행한다.
	// 타겟 face와 그것의 반대쪽 face, 소스 엣지에 바운드되는 두 face 모두 고려해서
	// 엣지를 생성하고 주변 정보를 업데이트한다.
	//
	//////////////////////////////////////////////////////////////////////////

	ZEdge* pNextBoundEdge = NULL;

	ZEdge *ne = new ZEdge[6];
	ZFace *nf = new ZFace[2];

	FLOAT sp = D3DXVec3Length( &D3DXVECTOR3(e->next->v2->pos - v->interedges[1]->v1->pos) ) / D3DXVec3Length( &D3DXVECTOR3(v->interedges[1]->v2->pos - v->interedges[1]->v1->pos) );
	FLOAT sq = D3DXVec3Length( &D3DXVECTOR3(e->twin->next->v2->pos - v->interedges[1]->v2->pos) ) / D3DXVec3Length( &D3DXVECTOR3(v->interedges[1]->v2->pos - v->interedges[1]->v1->pos) );

	//if( m_bHasTexture && m_pCorrespond->m_bHasTexture ) {
	
	
	ne[3].texcoord.org = e->twin->texcoord.org;
	ne[4].texcoord.org = e->next->next->texcoord.org;
	ne[5].texcoord.org = e->twin->next->next->texcoord.org;
	ne[0].texcoord.org = ne[1].texcoord.org = e->texcoord.org + v->barycentricUV.y * ( e->next->texcoord.org - e->texcoord.org );
	ne[2].texcoord.org = e->twin->texcoord.org = e->twin->texcoord.org + ( 1 - v->barycentricUV.y ) * ( e->twin->next->texcoord.org - e->twin->texcoord.org );

	
	ne[3].texcoord.max = e->twin->texcoord.max;
	ne[4].texcoord.max = v->interedges[1]->texcoord.org + sp * ( v->interedges[1]->next->texcoord.org - v->interedges[1]->texcoord.org );
	ne[5].texcoord.max = e->twin->next->next->texcoord.max;

	ne[0].texcoord.max = ne[2].texcoord.max = v->interedges[1]->texcoord.org + v->barycentricUV.x * ( v->interedges[1]->next->texcoord.org - v->interedges[1]->texcoord.org );
	ne[1].texcoord.max = e->twin->texcoord.max = v->interedges[1]->twin->texcoord.org + ( 1 - v->barycentricUV.x ) * ( v->interedges[1]->twin->next->texcoord.org - v->interedges[1]->twin->texcoord.org );
	//}

	ne[3].normal.org = e->twin->normal.org;
	ne[4].normal.org = e->next->next->normal.org;
	ne[5].normal.org = e->twin->next->next->normal.org;
	ne[0].normal.org = ne[1].normal.org = e->normal.org + v->barycentricUV.y * ( e->next->normal.org - e->normal.org );
	ne[2].normal.org = e->twin->normal.org = e->twin->normal.org + ( 1 - v->barycentricUV.y ) * ( e->twin->next->normal.org - e->twin->normal.org );

	ne[3].normal.max = e->twin->normal.max;
	ne[4].normal.max = (1 - sp) * v->interedges[1]->normal.org + sp * v->interedges[1]->next->normal.org;
	ne[5].normal.max = e->twin->next->next->normal.max;
	ne[0].normal.max = ne[2].normal.max = (1 - v->barycentricUV.x) * v->interedges[1]->normal.org + v->barycentricUV.x * v->interedges[1]->next->normal.org;
	ne[1].normal.max = e->twin->normal.max = v->barycentricUV.x * v->interedges[1]->twin->normal.org + (1 - v->barycentricUV.x) * v->interedges[1]->twin->next->normal.org;



	if( v->interedges[1]->CheckIntersect( e->twin->next, NULL ) == TRUE )
	{
		pNextBoundEdge = e->twin->next;
	}
	else if( v->interedges[1]->CheckIntersect( e->twin->next->next, NULL ) == TRUE )
	{
		pNextBoundEdge = e->twin->next->next;
		ne[2].normal.max = v->barycentricUV.x * v->interedges[1]->twin->normal.org + (1 - v->barycentricUV.x) * v->interedges[1]->twin->next->normal.org;
		ne[2].texcoord.max = v->interedges[1]->twin->texcoord.org + ( 1 - v->barycentricUV.x ) * ( v->interedges[1]->twin->next->texcoord.org - v->interedges[1]->twin->texcoord.org );
	}
	else
	{
		pNextBoundEdge = NULL;
		ne[5].normal.max = v->interedges[1]->twin->normal.org;
		ne[5].texcoord.max = v->interedges[1]->twin->texcoord.org;
	}


	// 대각 정점에 엣지 할당
	e->next->v2->edges.Add( &ne[4] );
	e->twin->next->v2->edges.Add( &ne[5] );  

	// e->twin의 시작점이 컷트의 중점 v로 바뀌어야 하므로 e-v2에서 e->twin을 제거
	e->v2->RemoveEdge( e->twin );
	e->v2->edges.Add( &ne[3] );

	for( int i=0; i < 3; i++ ) 
	{
		ne[i].v1 = ne[i+3].v2 = v;		
		ne[i].twin = &ne[i+3];
		ne[i+3].twin = &ne[i];
	}
	ne[0].v2 = ne[3].v1 = e->v2;
	ne[1].v2 = ne[4].v1 = e->next->v2;
	ne[2].v2 = ne[5].v1 = e->twin->next->v2;
	e->v2 = e->twin->v1 = v;

	ne[0].f = &nf[0];
	ne[1].f = e->f;
	ne[2].f = e->twin->f;
	ne[3].f = e->twin->f;
	ne[4].f = &nf[0];
	ne[5].f = &nf[1];
	e->twin->next->f = &nf[1];
	e->next->f = &nf[0];

	ne[0].next = e->next;
	ne[1].next = e->next->next;	
	ne[2].next = e->twin->next->next;
	ne[3].next = &ne[2];
	ne[4].next = &ne[0];
	ne[5].next = e->twin;

	e->next->next = &ne[4];
	e->next = &ne[1];	
	e->twin->next->next->next = &ne[3];
	e->twin->next->next = &ne[5];

	nf[0].edge = &ne[0];
	nf[1].edge = e->twin;
	e->twin->f->edge = &ne[3];
	e->twin->f = &nf[1];
	e->f->edge = e;

	v->edges.Add( e->twin );
	v->edges.Add( &ne[0] );
	v->edges.Add( &ne[1] );
	v->edges.Add( &ne[2] );

	for( int i=0; i < 6; i++ ) m_edges.Add( &ne[i] );
	m_pFaces[GetIndexOfSubsetID(e->f->m_dwSubsetID)].Add( &nf[0] );
	m_pFaces[GetIndexOfSubsetID(e->twin->f->m_dwSubsetID)].Add( &nf[1] );

	v->interverts[2] = e->v1;
	v->interverts[3] = ne[0].v2;		

	return pNextBoundEdge;
}


ZMeshSubsetInfo::ZMeshSubsetInfo( VOID )
{
	m_dwSubsetID	= 0;
	m_bVisited		= FALSE;
	m_bClosed		= TRUE;
}