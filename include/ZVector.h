#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모핑용 3차원 벡터

 D3DXVECTOR3로부터 상속받아 오리지날 값 및 min, max 값을 가짐으로써 보간될 수 있는 벡터
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZVector3 : public D3DXVECTOR3
{
public:
	D3DXVECTOR3				org, min, max;						///< 원본 값, min, max 값. min / max 값 사이를 보간하게 됨. 원본 값은 원래대로 돌아가기 위해 필요

public:
	ZVector3( VOID );

	/// 동등 연산자
	ZVector3				operator=( ZVector3 v );

	/// 동등 연산자
	ZVector3				operator=( D3DXVECTOR3 v );

	/// 현재 값을 반환
	D3DXVECTOR3				GetVal( VOID );

	/// 현재 값을 지정
	VOID					SetVal( D3DXVECTOR3 v );

	/// 모든 값을 주어진 벡터로 지정
	VOID					SetValAll( D3DXVECTOR3 v );

	/// 현재 값을 min 값으로 지정
	VOID					SetMin( VOID );

	/// 보간 함수. 주어진 알파 값 만큼 min, max 사이를 보간한 값으로 현재 값을 지정
	VOID					Interpolate( FLOAT alpha );

	/// 보간 함수. 주어진 알파 값 만큼 min 값과 주어진 벡터 v 사이를 보간한 값으로 현재 값을 지정
	VOID					Interpolate( D3DXVECTOR3 v, FLOAT alpha );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * \brief 모핑용 2차원 벡터

 D3DXVECTOR2로부터 상속받아 오리지날 값 및 min, max 값을 가짐으로써 보간될 수 있는 벡터
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ZVector2 : public D3DXVECTOR2
{
public:
	D3DXVECTOR2				org, min, max;						///< 원본 값, min, max 값. min / max 값 사이를 보간하게 됨. 원본 값은 원래대로 돌아가기 위해 필요

public:
	ZVector2( VOID );

	/// 동등 연산자
	ZVector2				operator=( ZVector2 v );

	/// 동등 연산자
	ZVector2				operator=( D3DXVECTOR2 v );

	/// 현재 값을 반환
	D3DXVECTOR2				GetVal( VOID );

	/// 현재 값을 지정
	VOID					SetVal( D3DXVECTOR2 v );

	/// 모든 값을 주어진 벡터로 지정
	VOID					SetValAll( D3DXVECTOR2 v );

	/// 현재 값을 min 값으로 지정
	VOID					SetMin( VOID );

	/// 보간 함수. 주어진 알파 값 만큼 min, max 사이를 보간한 값으로 현재 값을 지정
	VOID					Interpolate( FLOAT alpha );

	/// 보간 함수. 주어진 알파 값 만큼 min 값과 주어진 벡터 v 사이를 보간한 값으로 현재 값을 지정
	VOID					Interpolate( D3DXVECTOR2 v, FLOAT alpha );
};