// x2camera.cpp  
//
#include "x2camera.h"



X2Camera::X2Camera( const char* pszSelection, 
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface*	pTheSkyXForMounts,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*				pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*						pIOMutex,
					TickCountInterface*					pTickCount)
{   
	m_nPrivateISIndex				= nISIndex;
	m_pTheSkyXForMounts				= pTheSkyXForMounts;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;


    m_dwFin = 0;
	m_dCurTemp = -100.0;
	m_dCurPower = 0;

    mPixelSizeX = 0.0;
    mPixelSizeY = 0.0;

    // Read in settings
    if (m_pIniUtil) {
        m_pIniUtil->readString(KEY_X2CAM_ROOT, KEY_X2CAM_GUID, "0", m_szCameraSerial, 128);
        m_Camera.getCameraIdFromSerial(m_nCameraID, std::string(m_szCameraSerial));
        m_Camera.setCameraSerial(std::string(m_szCameraSerial));
        m_Camera.setCameraId(m_nCameraID);
        // read other camera settngs
    }

}

X2Camera::~X2Camera()
{
	//Delete objects used through composition
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;

}

#pragma mark DriverRootInterface
int	X2Camera::queryAbstraction(const char* pszName, void** ppVal)			
{
	X2MutexLocker ml(GetMutex());

	if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name))
			*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, PixelSizeInterface_Name))
        *ppVal = dynamic_cast<PixelSizeInterface*>(this);

	return SB_OK;
}

#pragma mark UI bindings
int X2Camera::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    int i;
    char tmpBuffer[128];
    int nCamIndex;

    if (NULL == ui)
        return ERR_POINTER;
    nErr = ui->loadUserInterface("SVBonyCamSelect.ui", deviceType(), m_nPrivateISIndex);
    if (nErr)
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    //Intialize the user interface
    m_Camera.listCamera(m_tCameraIdList);
    if(!m_tCameraIdList.size()) {
        snprintf(tmpBuffer,128,"No Camera found");
        dx->comboBoxAppendString("comboBox",tmpBuffer);
        dx->setCurrentIndex("comboBox",0);
    }
    else {
        nCamIndex = 0;
        for(i=0; i< m_tCameraIdList.size(); i++) {
            //Populate the camera combo box and set the current index (selection)
            snprintf(tmpBuffer, 128, "%s [%s]",m_tCameraIdList[i].model, m_tCameraIdList[i].Sn.id);
            dx->comboBoxAppendString("comboBox",tmpBuffer);
            if(m_tCameraIdList[i].cameraId == m_nCameraID)
                nCamIndex = i;
        }
        dx->setCurrentIndex("comboBox",nCamIndex);
    }
    //if(m_bLinked) {
        dx->setEnabled("pushButton", true);
    //}
    //else {
    //    dx->setEnabled("pushButton", false);
    //}
    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK)
    {
        int nCamera;
        std::string sCameraSerial;
        //Camera
        nCamera = dx->currentIndex("comboBox");
        m_Camera.setCameraId(m_tCameraIdList[nCamera].cameraId);
        m_nCameraID = m_tCameraIdList[nCamera].cameraId;
        m_Camera.getCameraSerialFromID(m_nCameraID, sCameraSerial);
        m_Camera.setCameraSerial(sCameraSerial);
        // store camera ID
        m_pIniUtil->writeString(KEY_X2CAM_ROOT, KEY_X2CAM_GUID, sCameraSerial.c_str());
    }

    return nErr;
}

int X2Camera::doSVBonyCAmFeatureConfig(bool& bPressedOK)
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;
    bPressedOK = false;

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("SVBonyCamera.ui", deviceType(), m_nPrivateISIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    // dx->setText("label_7","RoRo");

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;


    return nErr;
}


void X2Camera::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{

    //An example of showing another modal dialog
    if (!strcmp(pszEvent, "on_pushButton_clicked"))
    {
        int nErr=SB_OK;
        bool bPressedOK = false;

        nErr = doSVBonyCAmFeatureConfig( bPressedOK);

        if (bPressedOK)
        {
        }
    }
    else
        if (!strcmp(pszEvent, "on_timer"))
        {
        }

}


#pragma mark DriverInfoInterface
void X2Camera::driverInfoDetailedInfo(BasicStringInterface& str) const		
{
	X2MutexLocker ml(GetMutex());

    str = "SVBony camera X2 plugin by Rodolphe Pineau";
}

double X2Camera::driverInfoVersion(void) const								
{
	X2MutexLocker ml(GetMutex());

    return PLUGIN_VERSION;
}

#pragma mark HardwareInfoInterface
void X2Camera::deviceInfoNameShort(BasicStringInterface& str) const										
{
    X2Camera* pMe = (X2Camera*)this;
    X2MutexLocker ml(pMe->GetMutex());
    
    if(m_bLinked) {
        char cDevName[BUFFER_LEN];
        std::string sCameraSerial;
        std::string sCameraName;
        pMe->m_Camera.getCameraName(sCameraName);
        snprintf(cDevName, BUFFER_LEN, "%s", sCameraName.c_str());
        str = cDevName;
    }
    else {
        str = "";
    }
}

void X2Camera::deviceInfoNameLong(BasicStringInterface& str) const										
{
    X2Camera* pMe = (X2Camera*)this;
    X2MutexLocker ml(pMe->GetMutex());

    if(m_bLinked) {
        char cDevName[BUFFER_LEN];
        std::string sCameraSerial;
        std::string sCameraName;
        pMe->m_Camera.getCameraName(sCameraName);
        pMe->m_Camera.getCameraSerial(sCameraSerial);
        snprintf(cDevName, BUFFER_LEN, "%s (%s)", sCameraName.c_str(), sCameraSerial.c_str() );
        str = cDevName;
    }
    else {
        str = "";
    }
}

void X2Camera::deviceInfoDetailedDescription(BasicStringInterface& str) const								
{
	X2MutexLocker ml(GetMutex());

	str = "SVBony camera X2 plugin by Rodolphe Pineau";
}

void X2Camera::deviceInfoFirmwareVersion(BasicStringInterface& str)										
{
	X2MutexLocker ml(GetMutex());

	str = "Not available";
}

void X2Camera::deviceInfoModel(BasicStringInterface& str)													
{
	X2MutexLocker ml(GetMutex());

    if(m_bLinked) {
        char cDevName[BUFFER_LEN];
        std::string sCameraSerial;
        std::string sCameraName;
        m_Camera.getCameraName(sCameraName);
        snprintf(cDevName, BUFFER_LEN, "%s", sCameraName.c_str());
        str = cDevName;
    }
    else {
        str = "";
    }
}

#pragma mark Device Access
int X2Camera::CCEstablishLink(const enumLPTPort portLPT, const enumWhichCCD& CCD, enumCameraIndex DesiredCamera, enumCameraIndex& CameraFound, const int nDesiredCFW, int& nFoundCFW)
{
    int nErr = SB_OK;

    m_bLinked = false;
    if(!m_nCameraID)
        return ERR_NODEVICESELECTED;

    m_dCurTemp = -100.0;
    nErr = m_Camera.Connect(m_nCameraID);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

    return nErr;
}


int X2Camera::CCQueryTemperature(double& dCurTemp, double& dCurPower, char* lpszPower, const int nMaxLen, bool& bCurEnabled, double& dCurSetPoint)
{   
    int nErr = SB_OK;
    X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

    nErr = m_Camera.getTemperture(m_dCurTemp);
    dCurTemp = m_dCurTemp;
	dCurPower = 0;
    dCurSetPoint = 0;
    bCurEnabled = false;

    return nErr;
}

int X2Camera::CCRegulateTemp(const bool& bOn, const double& dTemp)
{ 
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

    return ERR_NOT_IMPL;
	// return SB_OK;
}

int X2Camera::CCGetRecommendedSetpoint(double& RecTemp)
{
	X2MutexLocker ml(GetMutex());

	RecTemp = 100;//Set to 100 if you cannot recommend a setpoint
    return ERR_NOT_IMPL;
	// return SB_OK;
}  


int X2Camera::CCStartExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const double& dTime, enumPictureType Type, const int& nABGState, const bool& bLeaveShutterAlone)
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	bool bLight = true;
    int nErr = SB_OK;

	switch (Type)
	{
		case PT_FLAT:
		case PT_LIGHT:			bLight = true;	break;
		case PT_DARK:	
		case PT_AUTODARK:	
		case PT_BIAS:			bLight = false;	break;
		default:				return ERR_CMDFAILED;
	}

	if (m_pTickCount)
		m_dwFin = (unsigned long)(dTime*1000)+m_pTickCount->elapsed();
	else
		m_dwFin = 0;

    nErr = m_Camera.startCaputure(dTime);
	return nErr;
}   



int X2Camera::CCIsExposureComplete(const enumCameraIndex& Cam, const enumWhichCCD CCD, bool* pbComplete, unsigned int* pStatus)
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

    *pbComplete = false;

	if (m_pTickCount)
	{
        if (m_pTickCount->elapsed()>m_dwFin) {
            if(m_Camera.isFameAvailable())
                *pbComplete = true;
        }
	}
    else {
        if(m_Camera.isFameAvailable())
            *pbComplete = true;

    }

	return SB_OK;
}

int X2Camera::CCEndExposure(const enumCameraIndex& Cam, const enumWhichCCD CCD, const bool& bWasAborted, const bool& bLeaveShutterAlone)           
{   
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	int nErr = SB_OK;

	if (bWasAborted)
    {
        m_Camera.abortCapture();
        // cleanup ?
	}

    return nErr;
}

int X2Camera::CCGetChipSize(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nXBin, const int& nYBin, const bool& bOffChipBinning, int& nW, int& nH, int& nReadOut)
{
	X2MutexLocker ml(GetMutex());

	nW = m_Camera.getWidth()/nXBin;
    nH = m_Camera.getHeight()/nYBin;
    nReadOut = CameraDriverInterface::rm_Image;

    m_Camera.setBinSize(nXBin);
    
    return SB_OK;
}

int X2Camera::CCGetNumBins(const enumCameraIndex& Camera, const enumWhichCCD& CCD, int& nNumBins)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		nNumBins = 1;
	else
        nNumBins = m_Camera.getNumBins();

    return SB_OK;
}

int X2Camera::CCGetBinSizeFromIndex(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nIndex, long& nBincx, long& nBincy)
{
	X2MutexLocker ml(GetMutex());

    nBincx = m_Camera.getBinFromIndex(nIndex);
    nBincy = m_Camera.getBinFromIndex(nIndex);

	return SB_OK;
}

int X2Camera::CCUpdateClock(void)
{   
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int X2Camera::CCSetShutter(bool bOpen)           
{   
	X2MutexLocker ml(GetMutex());
    // SVBony camera don't have physical shutter.

	return SB_OK;;
}

int X2Camera::CCActivateRelays(const int& nXPlus, const int& nXMinus, const int& nYPlus, const int& nYMinus, const bool& bSynchronous, const bool& bAbort, const bool& bEndThread)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

int X2Camera::CCPulseOut(unsigned int nPulse, bool bAdjust, const enumCameraIndex& Cam)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

void X2Camera::CCBeforeDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
}

void X2Camera::CCAfterDownload(const enumCameraIndex& Cam, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());
	return;
}

int X2Camera::CCReadoutLine(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& pixelStart, const int& pixelLength, const int& nReadoutMode, unsigned char* pMem)
{   
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           

int X2Camera::CCDumpLines(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nReadoutMode, const unsigned int& lines)
{                                     
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}           


int X2Camera::CCReadoutImage(const enumCameraIndex& Cam, const enumWhichCCD& CCD, const int& nWidth, const int& nHeight, const int& nMemWidth, unsigned char* pMem)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

    m_Camera.getFrame(nHeight, nMemWidth, pMem);

    return SB_OK;
}

int X2Camera::CCDisconnect(const bool bShutDownTemp)
{
	X2MutexLocker ml(GetMutex());

	if (m_bLinked)
	{
        m_Camera.Disconnect();
		setLinked(false);
	}

	return SB_OK;
}

int X2Camera::CCSetImageProps(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nReadOut, void* pImage)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int X2Camera::CCGetFullDynamicRange(const enumCameraIndex& Camera, const enumWhichCCD& CCD, unsigned long& dwDynRg)
{
	X2MutexLocker ml(GetMutex());

    uint32_t nBitDepth;

    nBitDepth = m_Camera.getBitDepth();
    dwDynRg = (unsigned long)2^nBitDepth;

	return SB_OK;
}

void X2Camera::CCMakeExposureState(int* pnState, enumCameraIndex Cam, int nXBin, int nYBin, int abg, bool bRapidReadout)
{
	X2MutexLocker ml(GetMutex());

	return;
}

int X2Camera::CCSetBinnedSubFrame(const enumCameraIndex& Camera, const enumWhichCCD& CCD, const int& nLeft, const int& nTop, const int& nRight, const int& nBottom)
{
	X2MutexLocker ml(GetMutex());

    m_Camera.setROI(nLeft, nTop, nRight, nBottom);
	return SB_OK;
}


int X2Camera::CCSettings(const enumCameraIndex& Camera, const enumWhichCCD& CCD)
{
	X2MutexLocker ml(GetMutex());

	return ERR_NOT_IMPL;
}

int X2Camera::CCSetFan(const bool& bOn)
{
	X2MutexLocker ml(GetMutex());

	return SB_OK;
}

int	X2Camera::pathTo_rm_FitsOnDisk(char* lpszPath, const int& nPathSize)
{
	X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	//Just give a file path to a FITS and TheSkyX will load it
		
	return SB_OK;
}

CameraDriverInterface::ReadOutMode X2Camera::readoutMode(void)		
{
	X2MutexLocker ml(GetMutex());

	return CameraDriverInterface::rm_Image;
}


enumCameraIndex	X2Camera::cameraId()
{
	X2MutexLocker ml(GetMutex());
	return m_CameraIdx;
}

void X2Camera::setCameraId(enumCameraIndex Cam)	
{
    m_CameraIdx = Cam;
}

int X2Camera::PixelSize1x1InMicrons(const enumCameraIndex &Camera, const enumWhichCCD &CCD, double &x, double &y)
{
    int nErr = SB_OK;

    if(!m_bLinked) {
        x = 0.0;
        y = 0.0;
        return ERR_COMMNOLINK;
    }

    x = m_Camera.getPixelSize();
    y = m_Camera.getPixelSize();
    return nErr;
}

int X2Camera::countOfIntegerFields (int &nCount)
{
    int nErr = SB_OK;
    nCount = 0;
    return nErr;
}

int X2Camera::valueForIntegerField (int nIndex, BasicStringInterface &sFieldName, BasicStringInterface &sFieldComment, int &nFieldValue)
{
    int nErr = SB_OK;
    sFieldName = "";
    sFieldComment = "";
    nFieldValue = 0;
    return nErr;
}

int X2Camera::countOfDoubleFields (int &nCount)
{
    int nErr = SB_OK;
    nCount = 0;
    return nErr;
}

int X2Camera::valueForDoubleField (int nIndex, BasicStringInterface &sFieldName, BasicStringInterface &sFieldComment, double &dFieldValue)
{
    int nErr = SB_OK;
    sFieldName = "";
    sFieldComment = "";
    dFieldValue = 0;
    return nErr;
}

int X2Camera::countOfStringFields (int &nCount)
{
    int nErr = SB_OK;
    nCount = 0;
    if(m_Camera.isCameraColor())
        nCount = 1;
    return nErr;
}

int X2Camera::valueForStringField (int nIndex, BasicStringInterface &sFieldName, BasicStringInterface &sFieldComment, BasicStringInterface &sFieldValue)
{
    int nErr = SB_OK;
    std::string sBayerPattern;
    if(nIndex == 0) { // we only have one extra header, any other index is not valid
        if(m_Camera.isCameraColor()) {
            m_Camera.getBayerPattern(sBayerPattern);
            sFieldName = "DEBAYER";
            sFieldComment = "Bayer pattern to use to decode color image";
            sFieldValue = sBayerPattern.c_str();   // need to set the filed for Monochrome/Color.
        }
        else {
            sFieldName = "DEBAYER";
            sFieldComment = "Bayer pattern to use to decode color image";
            sFieldValue = "MONO";
        }
        
    }
    return nErr;
}

