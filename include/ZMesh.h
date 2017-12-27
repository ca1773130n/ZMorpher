#pragma once

#include "ZPublic.h"
#include <set>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief (서브)메시 클래스. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZMesh
{
public:
	DWORD					m_dwNumSubset;							///< 메시의 서브셋 개수
	DWORD					m_dwNumVerts, m_dwNumFaces;				///< vertex, face 수
	DWORD					m_dwVertStart, m_dwFaceStart;			///< VB, IB에서 이 메시의 vertex, face가 시작되는 인덱스

	D3DXVECTOR3				m_vCMass;								///< 메시의 질량 중심

	CGrowableArray<ZVert*>	m_verts;								///< 버텍스 배열
	CGrowableArray<ZEdge*>	m_edges;								///< 엣지 배열
	CGrowableArray<ZFace*>*	m_pFaces;								///< 서브셋별 face 배열

	BOOL					m_bHasTexture;							///< 텍스쳐를 갖는지 여부
	ZMaterial*				m_pMaterials;							///< 머티리얼 배열

	ZMesh*					m_pCorrespond;							///< 대응하는 상대 메시 포인터. 1:1 매핑된 대응 메시들은 서로 merge 됨

	VOID*					m_pUserData;							///< 차후를 위해 준비함

public:
	ZMesh( VOID );

	/// VB, IB 배열을 넘겨받아 지정한 서브셋 ID들의 face를 만들어 서브메시를 생성.
	VOID					Create( CGrowableArray<DWORD>& subsetIDs, MESHVERTEX* pVertices, WORD* pIndices, D3DXATTRIBUTERANGE* pAttrTable, D3DMATERIAL9* pMat, ZType type );

	/// 파괴 함수
	VOID					Destroy( VOID );

	/// 질량 중심 벡터를 반환
	D3DXVECTOR3				GetCenterOfMass( VOID );

	/// 주어진 질량 중심을 기준으로 vertex coordination을 재 조정
	FLOAT					ChangeCoordsByCMass( D3DXVECTOR3 vCMass );

	/************************************************************************/
	/* Spheircal Parameterization 관련                                      */
	/************************************************************************/

	/// 임의의 정사면체를 만들어 네 곡지점을 배열에 넣음
	VOID					MakeRandomRegularTetraHedron( D3DXVECTOR3 vArray[4] );

	/// vertex 집합 중에서 가장 거리가 가까운 두 vertex의 거리를 반환
	FLOAT					GetMinDistanceOfVertexPairs( CGrowableArray<ZVert*>& verts );

	/// 주어진 앵커 vertex들 중 주어진 정사면체 네 꼭지점과 가까운 네 vertex를 고정시킴
	VOID					FixVertices( D3DXVECTOR3 vTetraVerts[4], CGrowableArray<ZVert*>& Anchors );
	
	/// 고정 정점들의 고정을 해제
	VOID					ResetFixedVertices( CGrowableArray<ZVert*>& Anchors );

	/// 메시의 모든 vertex들을 릴랙스시켜 구형으로 만듬
	DOUBLE					RelaxVertices( DOUBLE fEpsilon );

	/// embedding이 collapse 되었는지 판별
	BOOL					CheckCollapsed( D3DXVECTOR3 vTetraVerts[4], FLOAT fBetweenAnchors );

	/// embedding의 모든 face에 대해 face를 이루는 vertex들이 올바른 (시계)방향으로 면을 이루고 있는지 검사
	BOOL					CheckVertexOrientation( VOID );

	/// 메시를 sphere에 embed(parameterize)하는 메인 함수
	VOID					EmbedToSphere( VOID );

	/// 주어진 vContain을 포함하는 face의 boundary edge 중에서 v1이 vCorner인 엣지를 찾아 반환
	ZEdge*					FindEdgeOfContainFace( ZVert* vCorner, ZVert* vContain );

	/// 주어진 face f를 vertex pSrcV로 분할
	VOID					DivideTri( ZFace* f, ZVert* pSrcV );

	//VOID					EnqueueUniqueEdges( pq_pState pq, ZMesh& oppositeMesh );

	/// 메시의 노멀을 계산
	VOID					CalcNormals( VOID );

	/// 주어진 타겟 메시로 이 메시를 merge함. 서로 교차되는 엣지들을 cut하게 됨
	VOID					MergeWith( ZMesh* pTgtMesh );

	/// 보간의 시작 값을 현재 값으로 지정
	VOID					SetMin( VOID );

	/// 보간 함수. min, max 사이의 in-between 메시를 만들어 현재 값으로 지정
	VOID					Interpolate( FLOAT alpha );

	/// 주어진 타겟 메시로 이 메시를 분할. 타겟 메시의 vertex topology를 이 메시에 추가시키게 됨.
	VOID					DivideMeshFrom( ZMesh* pTgtMesh );

	/// 주어진 vertex를 구면상에서 포함하는 face를 이 메시에서 찾아 반환
	ZFace*					FindFaceContainsVertex( ZVert* pV, ZType type );

	/// 주어진 머티리얼 서브셋 ID에 해당하는 face들이 face 배열의 몇번째 인덱스에 존재하는지를 반환
	DWORD					GetIndexOfSubsetID( DWORD id );

	/// vertex, face 수 등 통계치를 업데이트함
	VOID					CalcMeshInfo( VOID );

	/// 주어진 엣지를 주어진 vertex를 기준으로 cut함. 
	ZEdge*					CutEdge( ZEdge* e, ZVert* v );

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시 서브셋 정보 클래스. 

 이 클래스는 ZMorpher::MapSubMeshes() 함수에서 소스와 타겟 메시의 각 서브메시들을 매핑시킬때 사용된다.
 메시의 서브셋(서브메시)에 대한 정보를 노드로 나타내고 그래프를 순회하면서 인접 서브셋들을 검사하여 
 서로 다른 서브셋이지만 geometry 관점에서는 하나의 서브메시로 판단하여야 할 것들을 알아내기 위해 서브셋 및 노드 정보를 담고 있다.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZMeshSubsetInfo
{
public:
	DWORD					m_dwSubsetID;						///< 서브셋 ID
	ZSubsetType				m_enSubsetType;						///< 서브셋 종류
	BOOL					m_bVisited;							///< 그래프 순회시 방문된 노드인지를 나타내기 위함

	set<DWORD>				m_dwAdjSubsetIDs;					///< 인접 서브셋들의 ID 집합
	CGrowableArray<DWORD>	m_dwComposedSubsetIDs;				///< 하나의 geometry를 구성할 수 있는 서브셋들의 ID들
	BOOL					m_bClosed;							///< 메시가 closed 메시인지 나타냄

	CGrowableArray
	<ZMeshSubsetInfo*>		m_pAdjNodes;						///< 인접 노드들의 배열

public:
	/// 생성자
	ZMeshSubsetInfo( VOID );
};