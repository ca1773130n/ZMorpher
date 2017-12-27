#include "stdafx.h"
#include "ZVector.h"

ZVector2::ZVector2( VOID )
{
	x = y = min.x = min.y = org.x = org.y = 0;
}

ZVector2
ZVector2::operator=( D3DXVECTOR2 v )
{
	x = v.x;
	y = v.y;
	return *this;
}

ZVector2
ZVector2::operator=( ZVector2 v )
{
	x = v.x;
	y = v.y;
	org = v.org;
	min = v.min;
	max = v.max;
	return *this;
}

D3DXVECTOR2
ZVector2::GetVal( VOID )
{
	return D3DXVECTOR2( x, y );
}

VOID
ZVector2::SetVal( D3DXVECTOR2 v )
{
	x = v.x;
	y = v.y;
}

VOID
ZVector2::SetValAll( D3DXVECTOR2 v )
{
	x = v.x;
	y = v.y;

	min = max = org = v;
}

VOID
ZVector2::SetMin( VOID )
{
	min.x = x;
	min.y = y;
}

VOID
ZVector2::Interpolate( FLOAT alpha )
{
	x = ( 1.f - alpha ) * min.x + alpha * max.x;
	y = ( 1.f - alpha ) * min.y + alpha * max.y;
}

VOID
ZVector2::Interpolate( D3DXVECTOR2 v, FLOAT alpha )
{
	x = ( 1.f - alpha ) * min.x + alpha * v.x;
	y = ( 1.f - alpha ) * min.y + alpha * v.y;
}

ZVector3::ZVector3( VOID )
{
	x = y = z = min.x = min.y = min.z = org.x = org.y = org.z = 0;
}

ZVector3
ZVector3::operator=( D3DXVECTOR3 v )
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}

ZVector3
ZVector3::operator=( ZVector3 v )
{
	x = v.x;
	y = v.y;
	z = v.z;
	org = v.org;
	min = v.min;
	max = v.max;
	return *this;
}

D3DXVECTOR3 
ZVector3::GetVal( VOID )
{
	return D3DXVECTOR3( x, y, z );
}

VOID
ZVector3::SetVal( D3DXVECTOR3 v )
{
	x = v.x;
	y = v.y;
	z = v.z;
}

VOID
ZVector3::SetValAll( D3DXVECTOR3 v )
{
	x = v.x;
	y = v.y;
	z = v.z;

	min = max = org = v;
}

VOID
ZVector3::SetMin( VOID )
{
	min.x = x;
	min.y = y;
	min.z = z;
}

VOID
ZVector3::Interpolate( FLOAT alpha )
{
	x = ( 1.f - alpha ) * min.x + alpha * max.x;
	y = ( 1.f - alpha ) * min.y + alpha * max.y;
	z = ( 1.f - alpha ) * min.z + alpha * max.z;
}

VOID
ZVector3::Interpolate( D3DXVECTOR3 v, FLOAT alpha )
{
	x = ( 1.f - alpha ) * min.x + alpha * v.x;
	y = ( 1.f - alpha ) * min.y + alpha * v.y;
	z = ( 1.f - alpha ) * min.z + alpha * v.z;
}