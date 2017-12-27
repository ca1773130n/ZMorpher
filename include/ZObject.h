#pragma once

#include "ZPublic.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 메시 클래스. 서브메시 배열 및 메시 정보를 담고 있다

 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZObject
{
public:
	DWORD					m_dwNumSubset;						///< 총 서브셋 수
	DWORD					m_dwNumVerts;						///< 총 vertex 수	
	DWORD					m_dwNumFaces;						///< 총 face 수
	DWORD					m_dwVBSize;							///< 버텍스 버퍼 사이즈	
	DWORD					m_dwIBSize;							///< 인덱스 버퍼 사이즈
	
	CGrowableArray<ZMesh*>	m_pMeshes;							///< 서브메시 배열
	LPDIRECT3DTEXTURE9		m_pTextures;						///< 텍스쳐 배열

	VOID*					m_pUserData;						///< 차후를 위해 준비

public:
	ZObject( VOID );

	/// DXUT 메시로부터 ZObject를 생성
	HRESULT					Create( CDXUTMesh* pDXUTMesh, ZType type );

	/// 파괴 함수
	VOID					Destroy( VOID );

	/// 서브메시들 sphere에 embed시킴
	VOID					EmbedToSphere( VOID );

	/// 모든 전처리 계산 과정이 끝난 상태에서 모핑을 위해 소스 메시 형태로 돌아가도록 하는 함수
	VOID					TransformBackToSrcMesh( VOID );

	/// 1:1 매핑된 소스와 타겟 서브메시들을 merge하는 함수
	VOID					MergeEmbeddings( VOID );
	
	/// 보간 함수. min, max 값 사이의 in-between 메시(오브젝트)를 만들어 현재 값으로 지정
	VOID					Interpolate( FLOAT alpha );

	/// 소스 서브메시들을 매핑된 타겟 서브메시들을 사용하여 분할
	VOID					DivideSrcMeshes( VOID );

	/// 메시의 통계 정보를 업데이트
	VOID					CalcObjectInfo( VOID );
};