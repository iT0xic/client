/*
	$Header: /Client/IO_Model.h 54    05-01-25 11:42a Choo0219 $
*/
#ifndef	__IO_MODEL_H
#define	__IO_MODEL_H
#include "IO_MESH.H"
#include "IO_MATERIAL.H"
#include "IO_Effect.h"
#include "Util//VFSManager.h"

//-------------------------------------------------------------------------------------------------

enum enumPOINTEFFECTTYPE {
  POINT_EFFECT_NORMAL = 0,
  POINT_EFFECT_DAYNNIGHT = 1,
  POINT_LIGHT_CONTAINER = 2,
};

class CPointPART {
public :
  t_HASHKEY m_uiEftKEY;
  short     m_nEffectType;
  short     m_nParent;

  D3DXQUATERNION m_Rotate;
  D3DVECTOR      m_Transform;
  D3DVECTOR      m_Scale;
  bool           Load(CFileSystem* pFilesystem);
};

class CBasicPART {
public :
  t_HASHKEY m_uiMeshKEY;
  t_HASHKEY m_uiMatKEY;

  CBasicPART() {
    m_uiMeshKEY = 0;
    m_uiMatKEY  = 0;
  }

  virtual bool Load(CFileSystem* pFilesystem, short nLinkBoneNo = -1);
  virtual bool IsRoot() { return false; }
  virtual void UnloadVisible(HNODE hNODE);
};

//-------------------------------------------------------------------------------------------------
class CCharPART : public CBasicPART {
private:
public :
  short      m_nLinkBoneIDX;
  short      m_nLinkDummyIDX;
  t_HASHKEY* m_pMeshAniFILE;

  CCharPART() { m_nLinkBoneIDX = m_nLinkDummyIDX = -1, m_pMeshAniFILE = nullptr; }

  ~CCharPART() {
    SAFE_DELETE_ARRAY( m_pMeshAniFILE );
  }

  bool Load(CFileSystem* pFilesystem, short nLinkBoneNo, short nLinkDummyNo);

  HNODE Load_ZMODEL(char* szName, HNODE hModelNode);
  void  Free_ZMODEL(HNODE hNODE);

  bool Is_LinkedMODEL(void);
};

//-------------------------------------------------------------------------------------------------
/// 고정 구조물 모델 부분.
class CFixedPART : public CBasicPART {
public :
  short          m_nParent;
  D3DXQUATERNION m_Rotate;
  D3DVECTOR      m_Transform;
  D3DXQUATERNION m_RotAXIS;
  D3DVECTOR      m_Scale;
  short          m_nCollisionLevel;
  t_HASHKEY      m_HashZMOFile;

  WORD m_wAniTYPE;

  /// 2004/3/17 Rangeset, buseLightmap 추가
  short m_nRangeSet;
  short m_nUseLightMap;

       CFixedPART();
  bool Load(CFileSystem* pFilesystem, short nLinkBoneNo, short nLinkDummyNo);

  bool IsRoot() override { return (m_nParent == -1); }

  HNODE LoadVisible(char*   szName, D3DVECTOR BasePOS, HNODE hParent);
  void  UnloadVisible(HNODE hNODE) override;
  //	void  ProcANI (HNODE hVisible);
};

//-------------------------------------------------------------------------------------------------
template <class CModelPart>
class CMODEL {
public :
  char* m_szName;

  int m_iCylinderRadius;
  int m_iCylinderX;
  int m_iCylinderY;

  short       m_nRootPART;
  short       m_nPartCNT;
  CModelPart* m_pParts;

  // 효과 붙는 위치...
  short       m_nDummyPointCNT;
  CPointPART* m_pDummyPoints;

  float m_BBMin[3];
  float m_BBMax[3];

  CMODEL() {
    m_pParts   = NULL, m_szName = nullptr, m_nRootPART = -1, m_nDummyPointCNT = 0, m_pDummyPoints = nullptr;
    m_BBMin[0] = m_BBMin[1]     = m_BBMin[2]           = 0.0f;
    m_BBMax[0] = m_BBMax[1]     = m_BBMax[2]           = 0.0f;

    m_iCylinderRadius = 0;

    m_iCylinderX = 0;
    m_iCylinderY = 0;
    m_nPartCNT   = 0;
  }

  ~CMODEL() { this->Free(); }

  bool Load(CFileSystem* pFileSystem, t_HASHKEY* pMeshKEY, t_HASHKEY* pMatKEY, t_HASHKEY* pEftKEY, short nLinkBoneNo, short nLinkDummyNo);
  void Free(void         );
};

template <class CModelPart>
bool CMODEL<CModelPart>::Load(CFileSystem* pFileSystem, t_HASHKEY* pMeshKEY, t_HASHKEY* pMatKEY, t_HASHKEY* pEftKEY, short nLinkBoneNo, short nLinkDummyNo) {
  /// Cylinder radius	
  pFileSystem->ReadInt32( &m_iCylinderRadius );
  pFileSystem->ReadInt32( &m_iCylinderX );
  pFileSystem->ReadInt32( &m_iCylinderY );

  pFileSystem->ReadInt16( &m_nPartCNT );
  if ( 0 == m_nPartCNT )
    return true;

  short nListIDX;

  //--------------------------------------------------------------------------------
  /// 각 모델 파트 로드
  //--------------------------------------------------------------------------------
  LogString(LOG_DEBUG, "the object model count is %d\n", m_nPartCNT);
  LogString(LOG_DEBUG, "offset is %d\n", pFileSystem->Tell());
  m_pParts       = new CModelPart[ m_nPartCNT ];
  for ( short nP = 0; nP < m_nPartCNT; nP++ ) {
    pFileSystem->ReadInt16( &nListIDX ); // mesh file
    m_pParts[nP].m_uiMeshKEY = pMeshKEY[nListIDX];

    pFileSystem->ReadInt16( &nListIDX ); // mat file

    // nListIDX < 0 인경우는 보이지 않는 충돌체크용...
    m_pParts[nP].m_uiMatKEY = (nListIDX >= 0) ? pMatKEY[nListIDX] : 0;

    m_pParts[nP].Load( pFileSystem, nLinkBoneNo, nLinkDummyNo );
    if ( m_pParts[nP].IsRoot() ) m_nRootPART = nP;
  }

  if ( m_nRootPART < 0 ) // 설정된 루트가 없으면 0번으로 강제 설정.
    m_nRootPART = 0;
  // need to return to here after object model count loop
  //--------------------------------------------------------------------------------
  /// 포인터 정보 로드
  //--------------------------------------------------------------------------------
  pFileSystem->ReadInt16( &m_nDummyPointCNT );
  if ( 0 != m_nDummyPointCNT ) {
    short nEffectType; // 밤낮 적용을 받는 이펙트 인가?
    m_pDummyPoints = new CPointPART[ m_nDummyPointCNT ];
    for ( short nP = 0; nP < m_nDummyPointCNT; nP++ ) {
      pFileSystem->ReadInt16( &nListIDX );
      /// Effect type
      pFileSystem->ReadInt16( &nEffectType );
      m_pDummyPoints[nP].m_nEffectType = nEffectType;

      m_pDummyPoints[nP].m_uiEftKEY = (nListIDX >= 0) ? pEftKEY[nListIDX] : 0;
      m_pDummyPoints[nP].Load( pFileSystem );
    }
  }

  //--------------------------------------------------------------------------------
  /// 모델 바운딩 박스 정보 로드
  //--------------------------------------------------------------------------------
  pFileSystem->ReadFloat( &m_BBMin[0] );
  pFileSystem->ReadFloat( &m_BBMin[1] );
  pFileSystem->ReadFloat( &m_BBMin[2] );

  pFileSystem->ReadFloat( &m_BBMax[0] );
  pFileSystem->ReadFloat( &m_BBMax[1] );
  pFileSystem->ReadFloat( &m_BBMax[2] );

  return true;
}

template <class CModelPart>
void CMODEL<CModelPart>::Free(void) {
  SAFE_DELETE_ARRAY(m_pParts);
  SAFE_DELETE_ARRAY(m_szName);
}

//-------------------------------------------------------------------------------------------------
template <class CModelPart>
class CModelDATA {
private :
  CStrVAR m_DataFileName;

  short               m_nModelCNT;
  CMODEL<CModelPart>* m_pMODELS;
  short               m_nDefaultLinkBoneNo; // if m_nDefaultLinkBoneNo >=0 then 
  short               m_nDefaultLinkDummyNo;

public :
  CModelDATA() { m_pMODELS = NULL, m_nModelCNT = 0, m_nDefaultLinkBoneNo = -1, m_nDefaultLinkDummyNo = -1; }
  ~CModelDATA() { this->Free(); }

  short GetModelCNT() { return m_nModelCNT; }

  CMODEL<CModelPart>* GetMODEL(short nIndex);
  bool                Load(char*     szFileName, short nBoneIdx = -1, short nDummyIdx = -1);
  void                Free(void      );
};

//-------------------------------------------------------------------------------------------------
/// .ZSC 를 읽는 부분이다. 각 ModelPart 의 타입에 따라 내부에서 구체적으로 읽는 데이터는 틀려진다.
template <class CModelPart>
bool CModelDATA<CModelPart>::Load(char* szFileName, short nBoneIdx, short nDummyIdx) {
  m_nDefaultLinkBoneNo  = nBoneIdx;
  m_nDefaultLinkDummyNo = nDummyIdx;

  CFileSystem* pFileSystem = (CVFSManager::GetSingleton()).GetFileSystem();
  if ( pFileSystem->OpenFile( szFileName ) == false ) {
    char* szStr = CStr::Printf( "File [%s] open error ", szFileName );
    g_pCApp->ErrorBOX( szStr, "ERROR", MB_OK );
    return false;
  }

  m_DataFileName.Set( szFileName ); //datafile name reader

  t_HASHKEY *pMeshKEY = nullptr, *pMatKEY = nullptr, *pEftKEY = nullptr;

  /////////////////////////////////////////////////////////////////////////////////////
  /// Mesh file list
  char* pStr;
  short nI, nMeshFileCNT, nMatFileCNT;
  pFileSystem->ReadInt16( &nMeshFileCNT );

  pMeshKEY = new t_HASHKEY[ nMeshFileCNT ];
  for ( nI = 0; nI < nMeshFileCNT; nI++ ) {
    pStr   = CGameStr::ReadString( pFileSystem );
#ifdef	_DEBUG
		LogString (LOG_DEBUG, "Model MESH::[ %s ] \n", pStr);
#endif
    pMeshKEY[nI] = g_MeshFILE.Add_FILE( pStr );
    if ( !pMeshKEY[nI] ) {
      char* szMsg = CStr::Printf( "Mesh File [%s] not found ...", pStr );
      g_pCApp->ErrorBOX( szMsg, "ERROR", MB_OK );
    }
    //_ASSERT( pMeshKEY[ nI ] );
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /// Mat file list
  short nIsSkin,    nIsAlpha,  nIs2Side;
  short nAlphaTest, nAlphaRef, nZTest, nZWrite, nBlendType, nSpecular;
  float fAlphaValue;
  short nGlowType;
  float fGlowColor[3];

  pFileSystem->ReadInt16( &nMatFileCNT );
  pMatKEY  = new t_HASHKEY[ nMatFileCNT ];
  for ( nI = 0; nI < nMatFileCNT; nI++ ) {
    pStr   = CGameStr::ReadString( pFileSystem );

    pFileSystem->ReadInt16( &nIsSkin );
    pFileSystem->ReadInt16( &nIsAlpha );
    pFileSystem->ReadInt16( &nIs2Side );

    pFileSystem->ReadInt16( &nAlphaTest );
    pFileSystem->ReadInt16( &nAlphaRef );
    pFileSystem->ReadInt16( &nZTest );
    pFileSystem->ReadInt16( &nZWrite );
    pFileSystem->ReadInt16( &nBlendType );
    pFileSystem->ReadInt16( &nSpecular );
    pFileSystem->ReadFloat( &fAlphaValue );
    pFileSystem->ReadInt16( &nGlowType );
    pFileSystem->ReadFloat3( fGlowColor );

#ifdef	_DEBUG
		LogString (LOG_DEBUG, "Model MAT ::[ %s ], skin:%d, alpha:%d, 2side:%d, alphatest:%d, alpharef:%d, ztest:%d, zwrite:%d, blendtype:%d, specular:%d, alphavalue:%f, glowtype:%d, glowcolor:%f,%f,%f \n",
			pStr, nIsSkin, nIsAlpha, nIs2Side, nAlphaTest, nAlphaRef, nZTest, nZWrite, nBlendType, nSpecular, fAlphaValue, nGlowType, fGlowColor[0], fGlowColor[1], fGlowColor[2]);
#endif
    pMatKEY[nI] = g_MatFILE.Add_MATERIAL( pStr, nIsSkin, nIsAlpha, nIs2Side,
                                          false,   /* bTerrainMAT */
                                          nullptr, /* szMatNameUseLightMap */
                                          nAlphaTest, nAlphaRef, nZTest,
                                          nZWrite, nBlendType, nSpecular,
                                          fAlphaValue,
                                          nGlowType,
                                          fGlowColor
    );
    if ( !pMatKEY[nI] ) {
      char* szMsg = CStr::Printf( "Material File [%s] not found ...", pStr );
      g_pCApp->ErrorBOX( szMsg, "ERROR", MB_OK );
    }
    // _ASSERT( pMatKEY[ nI ] );
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /// Effect file list
  short nEftFileCNT;
  pFileSystem->ReadInt16( &nEftFileCNT );
  if ( nEftFileCNT > 0 ) {
    pEftKEY = new t_HASHKEY[ nEftFileCNT ]; /// Modify : 2004/2/12 :nAvy
    //		pEftKEY = new t_HASHKEY[ nMatFileCNT ];
    for ( nI = 0; nI < nEftFileCNT; nI++ ) {
      pStr   = CGameStr::ReadString( pFileSystem );

      //----------------------------------------------------------------------------------------------------			
      /// @todo 라이트를 이펙트와 같은 데이터로 사용하기때문에 현재는 구별 방법이 모호하다.
      ///        차후에 새로운 타입으로 생성하는 방법으로 전환요함!!
      //----------------------------------------------------------------------------------------------------
      if ( pStr[0] == '3' && (pStr[1] == 'D' || pStr[1] == 'd') ) {
        pEftKEY[nI] = g_pEffectLIST->Add_EffectFILE( pStr );
        if ( !pEftKEY[nI] ) {
          char* szMsg = CStr::Printf( "Effect File [%s] not found ...", pStr );
          LogString (LOG_DEBUG, "ERROR:: %s in %s \n", szMsg, szFileName);
        }
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  /// 실제  Part 타입의 데이터를 할당 하고 로딩..
  pFileSystem->ReadInt16( &m_nModelCNT );
  m_pMODELS = new CMODEL<CModelPart> [ m_nModelCNT ];
  //LogString(LOG_DEBUG, "im in %s", pStr);
  for ( short nM = 0; nM < m_nModelCNT; nM++ ) {
    m_pMODELS[nM].Load( pFileSystem, pMeshKEY, pMatKEY, pEftKEY, m_nDefaultLinkBoneNo, m_nDefaultLinkDummyNo );
  }

  SAFE_DELETE_ARRAY( pEftKEY );
  SAFE_DELETE_ARRAY( pMatKEY );
  SAFE_DELETE_ARRAY( pMeshKEY );

  pFileSystem->CloseFile();
  (CVFSManager::GetSingleton()).ReturnToManager( pFileSystem );

  return true;
}

template <class CModelPart>
void CModelDATA<CModelPart>::Free(void) {
  SAFE_DELETE_ARRAY( m_pMODELS );
}

template <class CModelPart>
CMODEL<CModelPart>* CModelDATA<CModelPart>::GetMODEL(short nIndex) {
  if ( nIndex < 0 || nIndex >= m_nModelCNT ) {
    LogString (LOG_DEBUG, "ERROR:: Invalid model data index[ %d / %d ] in %s \n", nIndex, m_nModelCNT, m_DataFileName.Get());
    return nullptr;
  }

  return &m_pMODELS[nIndex];
}

//-------------------------------------------------------------------------------------------------
#endif
