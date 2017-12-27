#include "stdafx.h"
#include "ZFace.h"

ZFace::ZFace( VOID )
{
	m_dwSubsetID = 0;
	edge = NULL;

}

ZFace::ZFace( DWORD subsetID, ZType typecode )
{
	new (this) ZFace();
	m_dwSubsetID = subsetID;
	type = typecode;
}