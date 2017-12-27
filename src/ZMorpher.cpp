#include "stdafx.h"
#include "ZMorpher.h"

ZMorpher::ZMorpher( VOID )
{
	m_fAlpha = 0;
	m_SrcObj = new ZObject();
	m_TgtObj = new ZObject();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 초기화 함수

 모핑을 위한 소스 및 타겟 메시를 파괴하고 초기화한다.
 모퍼는 계속 존재하고 반복되는 모핑에 따라 소스 및 타겟 메시 정보를 제거하고 다시 생성하여야 한다

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::Initialize( VOID )
{	
	m_SrcObj->Destroy();
	m_TgtObj->Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모핑을 준비하는 함수

 이 함수에서는 DXUT 메시로부터 소스 및 타겟 ZObject를 생성하고 sphere에 embed시킨다.
 또 타겟 오브젝트로 소스 오브젝트를 분할한다.

 * \param pSrcMesh	소스 DXUT 메시 포인터
 * \param pTgtMesh	타겟 DXUT 메시 포인터
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::PrepareForMorph( CDXUTMesh* pSrcMesh, CDXUTMesh* pTgtMesh )
{
	if( FAILED( m_SrcObj->Create( pSrcMesh, ZTYPE_SRC ) ) ) DXUTOutputDebugString( L"Create Failed!" );
	if( FAILED( m_TgtObj->Create( pTgtMesh, ZTYPE_TGT ) ) ) DXUTOutputDebugString( L"Create Failed!" );

	// 서브메쉬 숫자를 맞추고 1:1 매핑
	MapSubmeshes();

	// Sphere에 Embed
	m_SrcObj->EmbedToSphere();
	m_TgtObj->EmbedToSphere();

	// 양쪽으로 Divide 한다
	DivideSrcMeshesByTgtMeshes();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 서브메시 매핑 함수

 소스와 타겟 서브메시들을 1:1 매핑 시킨다. 원본 메시의 서브메시 수는 소스와 타겟이 서로 다를수 있기 때문에 숫자를 맞춰주어야 한다.
 여기서는 sphere spawnning을 통해 양쪽 서브메시 수를 맞춰 준다. 즉 모자라는 쪽이 sphere를 생성하여 그것을 상대편 서브메시에 매핑시킨다

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::MapSubmeshes( VOID )
{
	FLOAT fD, fMinD;
	D3DXVECTOR3 vSrcCMass;

	ZMesh** pMeshes = m_SrcObj->m_pMeshes.GetData();
	ZMesh** pTgtMeshes;
	ZMesh*	pMinDistMesh;

	DWORD numSrcMesh = m_SrcObj->m_pMeshes.GetSize();
	DWORD numTgtMesh = m_TgtObj->m_pMeshes.GetSize();

	// 서브메쉬 수가 적은쪽은 스포닝 스피어를 생성
	//if( numSrcMesh > numTgtMesh ) m_TgtObj->SpawnSphere( numSrcMesh - numTgtMesh ); 

	// 모든 서브메시들에 대해서
	for( DWORD i=0; i < m_SrcObj->m_pMeshes.GetSize(); i++ )
	{
		fMinD = 1000000;
		vSrcCMass = pMeshes[i]->GetCenterOfMass();

		BOOL bAllTgtMeshesMapped = TRUE;
		pTgtMeshes = m_TgtObj->m_pMeshes.GetData();
		for( DWORD j=0; j < m_TgtObj->m_pMeshes.GetSize(); j++ )
		{
			// 질량 중심 거리가 가장 가까운 상대 서브메시를 찾는다
			fD = D3DXVec3Length( &D3DXVECTOR3(vSrcCMass - pTgtMeshes[j]->GetCenterOfMass()) );
			if( fD < fMinD ) 
			{
				fMinD = fD;
				pMinDistMesh = pTgtMeshes[j];
			}
			if( pTgtMeshes[j]->m_pCorrespond == NULL ) bAllTgtMeshesMapped = FALSE;
		}

		// 이미 모든 메시가 매핑이 되었거나, 그렇지 않으면서 가장 가까운 상대 서브메시가 매핑되지 않은 경우		
		if( bAllTgtMeshesMapped || (!bAllTgtMeshesMapped && !pMinDistMesh->m_pCorrespond) ) 
		{
			pMeshes[i]->m_pCorrespond = pMinDistMesh;
			pMinDistMesh->m_pCorrespond = pMeshes[i];

			for( DWORD k=0; k < pMeshes[i]->m_dwNumSubset; k++ )
			{
				pMeshes[i]->m_pMaterials[k].max = pMeshes[i]->m_pCorrespond->m_pMaterials[k].min;
			}
		}
	}	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 소스 메시들은 타겟 메시들로 분할하는 함수

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::DivideSrcMeshesByTgtMeshes( VOID )
{
	m_SrcObj->DivideSrcMeshes();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 소스와 타겟 메시들을 합병하는 함수

 * \param VOID	없음
 * \return		없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::MergeEmbeddings( VOID )
{
	m_SrcObj->MergeEmbeddings();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모핑을 시작하기 위해 소스 오브젝트 형태로 되돌리는 함수

 * \param VOID	없음
 * \return		없음 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::TransformBackToSrcMesh( VOID )
{
	m_SrcObj->TransformBackToSrcMesh();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 보간 함수.

 소스 오브젝트를 지정한 알파 값 만큼 보간한 in-between 오브젝트 형태를 갖도록 설정한다.

 * \param alpha		보간 값
 * \return			없음
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID
ZMorpher::SetInterpolatedMesh( FLOAT alpha )
{	
	m_SrcObj->Interpolate( alpha );
	m_fAlpha = alpha;
}