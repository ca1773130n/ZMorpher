#pragma once

#include "ZVector.h"
#include "ZVert.h"
#include "ZEdge.h"
#include "ZFace.h"
#include "ZMesh.h"
#include "ZObject.h"
#include "ZMaterial.h"
#include "ZType.h"
#include "ZDebug.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모퍼 메인 클래스. 

 주어진 소스 및 타겟 DXUT 메시로부터 모핑 계산을 수행하고 보간된 in-between 메시를 구하도록 한다.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZMorpher
{
public:
	FLOAT					m_fAlpha;					///< 보간을 위한 알파값. 0~1 사이의 값

	ZObject*				m_SrcObj;					///< 소스 메시 오브젝트
	ZObject*				m_TgtObj;					///< 타겟 메시 오브젝트

public:
	ZMorpher( VOID );

	/// 초기화 함수. 이전 모핑 데이터를 삭제하고 초기화 수행
	VOID					Initialize( VOID );

	/// 소스 및 타겟 DXUT 메시를 입력받아 ZObject를 만들고 계산을 준비
	VOID					PrepareForMorph( CDXUTMesh* pSrcMesh, CDXUTMesh* pTgtMesh );

	/// 소스 서브메시들을 매핑된 타겟 서브메시들을 사용하여 분할
	VOID					DivideSrcMeshesByTgtMeshes( VOID );

	/// 서브메시들을 1:1 매핑시키는 함수
	VOID					MapSubmeshes( VOID );

	/// 매핑된 소스 및 타겟 서브메시들을 합병하여 cut하는 함수
	VOID					MergeEmbeddings( VOID );

	/// 모핑을 위해 embedding들을 소스 오브젝트 형태로 되돌리는 함수
	VOID					TransformBackToSrcMesh( VOID );

	/// 보간 함수. 주어진 알파 값 만큼 보간된 오브젝트를 현재값으로 지정
	VOID					SetInterpolatedMesh( FLOAT alpha );
};