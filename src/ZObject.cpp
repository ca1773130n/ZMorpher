#include "stdafx.h"
#include "ZObject.h"
#include "ZMesh.h"
#include "ZMaterial.h"

#include <set>
#include <algorithm>
#include <stack>

using namespace std;

ZObject::ZObject( VOID )
{
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief ZObject 생성 함수

 DXUT 메시로부터 ZObject를 생성하는 함수. 
 
 이 함수에서는 특별히, DXUT 메시의 Attribute Table 및 Adjacency 정보를 이용하여
 해당 DXUT 메시 중에서 여러 서브셋으로 구성되어 있으나 geometry 측면에서는 하나의 단일 서브메시로 구성할 수 있는 것을 가려낸다.
 이러한 '통합 가능한' 서브셋들을 하나의 ZMesh로 생성하여 모핑에 사용하게 되며, 여러 서브셋으로 구성되어 있기 때문에 렌더링시
 각각의 서브셋 ID에 맞는 머티리얼을 face별로 그려주면서 보간하게 된다.

 이러한 일을 하는 이유는, 겉으로 보기에는 단일 폐곡면을 이루는 메시일지라도 각각의 face들이 다른 머티리얼 ID를 가질 경우 DXUT메시는
 서로 다른 서브메시(서브셋)으로 인식하기 때문에 실제로는 (그것이 closed 메시를 이룬다면)하나의 통합 메시로 보아 embed시키면 됨에도 불구하고,

 
 * \param pDXUTMesh		DXUT 메시 포인터
 * \param type			메시 종류
 * \return				에러 코드
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT 
ZObject::Create( CDXUTMesh* pDXUTMesh, ZType type )
{
	HRESULT hr = S_OK;

	m_dwNumVerts = m_dwNumFaces = m_dwNumSubset = 0;
	m_dwVBSize = m_dwIBSize = 0;
	m_pTextures = NULL;

	LPD3DXMESH pMesh = pDXUTMesh->m_pMesh;
	DWORD numTotalVerts = pMesh->GetNumVertices();
	DWORD numTotalFaces = pMesh->GetNumFaces();

	// 인접 정보 생성
	LPD3DXBUFFER pAdjacency = 0, pPoAdjcency = 0;
	D3DXCreateBuffer( numTotalFaces * sizeof(DWORD) * 3, &pAdjacency );
	D3DXCreateBuffer( numTotalFaces * sizeof(DWORD) * 3, &pPoAdjcency );
	pMesh->GenerateAdjacency( 0, (DWORD*)pAdjacency->GetBufferPointer() );

	// 메쉬를 weld 한다
	D3DXWELDEPSILONS we; 
	ZeroMemory( &we, sizeof( D3DXWELDEPSILONS)); 
	D3DXWeldVertices( pMesh, D3DXWELDEPSILONS_WELDPARTIALMATCHES, &we, (const DWORD*)pAdjacency->GetBufferPointer(), (DWORD*)pPoAdjcency->GetBufferPointer(), NULL, NULL); 

	// Attribute 테이블, 버퍼 생성
	DWORD* pAttrBuffer = 0;
	DWORD* pAdjBuffer = (DWORD*)pPoAdjcency->GetBufferPointer();

	pMesh->GetAttributeTable( 0, &m_dwNumSubset );
	D3DXATTRIBUTERANGE* attTable = new D3DXATTRIBUTERANGE[m_dwNumSubset];
	pMesh->GetAttributeTable( attTable, &m_dwNumSubset );
	pMesh->LockAttributeBuffer( D3DLOCK_READONLY, &pAttrBuffer );

	//////////////////////////////////////////////////////////////////////////
	// 서브셋 인접 정보 생성
	//
	// 인접 정보와 Attribute Table을 참고하여 서브셋 정보를 생성하여 subsetInfo 배열에 넣는다
	// 이때 인접 서브셋 ID 정보도 검사햐여 서브셋 정보 객체에 저장한다
	// i는 서브셋 ID, j는 face 인덱스, k는 vertex 인덱스이다.
	//////////////////////////////////////////////////////////////////////////
	ZMeshSubsetInfo* subsetInfo = new ZMeshSubsetInfo[m_dwNumSubset]();
	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		subsetInfo[i].m_dwSubsetID = i;
		for( DWORD j=attTable[i].FaceStart; j < attTable[i].FaceCount + attTable[i].FaceStart; j++ )
		{
			// Attribute Table의 face를 검사. pAdjBuffer를 참조하여 해당 인덱스의 face에 대한 인접 face 인덱스를 얻는다.
			// 이 인덱스를 가지고 AttrBuffer를 참조하여 해당 인접 face의 서브셋 ID를 얻는다.
			// ID를 얻어서 이 서브셋정보에 대한 인접 서브셋 ID 배열에 추가한다.
			for( int k=0; k < 3; k++ )
			{
				subsetInfo[i].m_dwAdjSubsetIDs.insert( pAttrBuffer[pAdjBuffer[j * 3 + k]] );

				// Attr 테이블의 값이 -1이라는 것은 해당 face와 인접하는 face가 없다는 뜻이다.
				// 따라서 이 경우 open 되어있다는 것을 의미하며, 플래그를 false로 놓는다.
				if( pAttrBuffer[pAdjBuffer[j * 3 + k]] == -1 ) subsetInfo[i].m_bClosed = FALSE;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 그래프 작성. 서브셋 정보 노드들을 인접 서브셋 정보를 이용하여 연결한다
	//////////////////////////////////////////////////////////////////////////
	BOOL bFound = FALSE;
	set<DWORD>::iterator iter;
	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		do {
			bFound = FALSE;
			for( iter=subsetInfo[i].m_dwAdjSubsetIDs.begin(); iter != subsetInfo[i].m_dwAdjSubsetIDs.end(); iter++ )
			{
				// 인접 노드 집합에에 검사 노드가 포함되지 않았고, 검사 노드의 서브셋이 폐곡면을 이루면 집합에 추가한다
				// 즉 단일 폐곡면을 이룰수 있는 서브셋의 노드들만 인접 노드 배열에 추가하여 노드를 서로 연결한다
				// 이 인접 노드 배열은 해당 노드와 연결된 노드들을 나타냄으로써 그래프를 구성하고 순회가 가능하게 한다.
				if( subsetInfo[*iter].m_bClosed == TRUE && subsetInfo[i].m_pAdjNodes.Contains(&subsetInfo[*iter]) == FALSE ) 
					subsetInfo[i].m_pAdjNodes.Add( &subsetInfo[*iter] );				
				// 아닌 경우에는 즉 집합에 포함하면서 closed가 아닌 경우에는 인접 서브셋 ID 집합에서 제거한다
				else
				{
					subsetInfo[i].m_dwAdjSubsetIDs.erase( iter );  
					bFound = TRUE;
					break;					
				}
			}
		}
		while( bFound == TRUE );
	}

	//////////////////////////////////////////////////////////////////////////
	// 그래프 순회, 통합 및 서브메쉬 생성	
	//////////////////////////////////////////////////////////////////////////
	stack<ZMeshSubsetInfo*> nodeStack;
	ZMesh* pZMesh = 0;

	ZMeshSubsetInfo* pPopped = 0;
	ZMeshSubsetInfo** pAdjs = 0;

	// VB, IB를 lock한다
	MESHVERTEX* pVertices = 0;
	WORD* pIndices = 0;	
	pMesh->LockVertexBuffer( D3DLOCK_READONLY, (VOID**)&pVertices );
	pMesh->LockIndexBuffer( D3DLOCK_READONLY, (VOID**)&pIndices );

	int numComposedSubset = 0;
	for( DWORD i=0; i < m_dwNumSubset; i++ )
	{
		numComposedSubset = 0;

		if( subsetInfo[i].m_bVisited == FALSE )
		{
			//////////////////////////////////////////////////////////////////////////
			// 노드 순회. 서브셋 정보 객체들의 연결 그래프를 순회한다
			//////////////////////////////////////////////////////////////////////////
			nodeStack.push( &subsetInfo[i] );
			while( !nodeStack.empty() )
			{
				pPopped = nodeStack.top();
				nodeStack.pop();
				pPopped->m_bVisited = TRUE;

				numComposedSubset++;

				// 순회하면서 지나가는 노드들의 서브셋 ID들은 모두 하나의 메시로 통합할수 있는 서브셋들의 ID이다. 
				// m_dwComposedSubsetIDs 배열에 추가한다. 이 배열의 ID들은 모두 geometry 측면에서 하나의 폐곡면 메시를 이룰 수 있는 것들이다
				subsetInfo[i].m_dwComposedSubsetIDs.Add( pPopped->m_dwSubsetID );

				// 방문하지 않은 노드들만 스택에 넣어 순회하도록 한다
				pAdjs = pPopped->m_pAdjNodes.GetData();
				for( int i=0; i < pPopped->m_pAdjNodes.GetSize(); i++ )
					if( pAdjs[i]->m_bVisited == FALSE ) nodeStack.push( pAdjs[i] );
			}

			// ZMesh 생성
			ZMesh* pNewZMesh = new ZMesh();
			pNewZMesh->Create( subsetInfo[i].m_dwComposedSubsetIDs, pVertices, pIndices, attTable, pDXUTMesh->m_pMaterials, type );
			if( pDXUTMesh->m_pTextures[0] ) pNewZMesh->m_bHasTexture = TRUE;
			m_pMeshes.Add( pNewZMesh ); 

			pNewZMesh->m_pUserData = m_pUserData;
		}		 		 
	}

	delete[] subsetInfo;
	pMesh->UnlockAttributeBuffer();
	pMesh->UnlockVertexBuffer();
	pMesh->UnlockIndexBuffer();

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 파괴 함수

 오브젝트 생성을 통해 동적으로 생성한 모든 mesh, vertex, edge, face 및 기타 객체를 해제한다.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
 ZObject::Destroy( VOID )
{
	ZMesh** pMeshes = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
		pMeshes[i]->Destroy();

	m_pMeshes.RemoveAll();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief Spherical Parameterization 함수

 서브메시들을 모두 sphere에 embed시킨다

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
 ZObject::EmbedToSphere( VOID )
{
	ZMesh** pMeshes = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
		pMeshes[i]->EmbedToSphere();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 보간 함수

 서브메시들을 모두 지정한 alpha 값 만큼 보간시킨다

 * \param alpha		보간 값. 0~1 사이
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZObject::Interpolate( FLOAT alpha )
{
	ZMesh** pMesh = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ ) pMesh[i]->Interpolate( alpha );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시 합변 함수

 타겟 서브메시들로부터 소스 메시를 합병하여 cut를 수행한다. 이때 각 소스 및 타겟 서브메시들은 1:1 매핑된 상태이다.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZObject::MergeEmbeddings( VOID )
{
	ZMesh** pMesh = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
		pMesh[i]->MergeWith( pMesh[i]->m_pCorrespond );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모핑을 위해 모든 서브메시들은 소스 메시 형태로 돌린다

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZObject::TransformBackToSrcMesh( VOID )
{
	ZMesh** pMesh = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
		pMesh[i]->SetMin();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 타겟 서브메시들로부터 소스 서브메시들은 분할한다.

 이 과정을 통해 소스 서브메시들은 모두 타겟 서브메시들의 vertex를 가지게 된다.

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZObject::DivideSrcMeshes( VOID )
{
	ZMesh** pMesh = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
		pMesh[i]->DivideMeshFrom( pMesh[i]->m_pCorrespond );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모든 서브메시의 통계 정보를 업데이트한다

 vertex 및 face 수를 갱신한다. 

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZObject::CalcObjectInfo( VOID )
{	
	m_dwNumVerts = m_dwNumFaces = 0;
	ZMesh** pMesh = m_pMeshes.GetData();
	for( DWORD i=0; i < m_pMeshes.GetSize(); i++ )
	{
		pMesh[i]->CalcMeshInfo();
		m_dwNumVerts += pMesh[i]->m_dwNumVerts;	
		m_dwNumFaces += pMesh[i]->m_dwNumFaces;
	}
}