#include "stdafx.h"
#include "ZMaterial.h"

ZMaterial::ZMaterial( VOID )
{

}

VOID
ZMaterial::Interpolate( FLOAT alpha )
{
	Ambient		= ( 1.f - alpha ) * min->Ambient + alpha * max->Ambient;
	Diffuse		= ( 1.f - alpha ) * min->Diffuse + alpha * max->Diffuse;
	Specular	= ( 1.f - alpha ) * min->Specular + alpha * max->Specular;
	Power		= ( 1.f - alpha ) * min->Power + alpha * max->Power;
	Emissive	= ( 1.f - alpha ) * min->Emissive + alpha * max->Emissive;
}

VOID
ZMaterial::SetValAll( D3DMATERIAL9* mat )
{
	Ambient = mat->Ambient;
	Diffuse = mat->Diffuse;
	Specular = mat->Specular;
	Power = mat->Power;
	Emissive = mat->Emissive;

	min = max = mat;
}
