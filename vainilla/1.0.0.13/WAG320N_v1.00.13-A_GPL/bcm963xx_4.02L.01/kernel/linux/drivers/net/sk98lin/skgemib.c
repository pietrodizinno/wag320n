/*****************************************************************************
 *
 * Name:	skgemib.c
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1.1.1.1 $
 * Date:	$Date: 2009-01-05 09:00:54 $
 * Purpose:	Private Network Management Interface Management Database
 *
 ****************************************************************************/

/******************************************************************************
 *
 *	(C)Copyright 1998-2002 SysKonnect GmbH.
 *	(C)Copyright 2002-2003 Marvell.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	The information in this file is provided "AS IS" without warranty.
 *
 ******************************************************************************/

/*
 * PRIVATE OID handler function prototypes
 */
PNMI_STATIC int Addr(SK_AC *pAC, SK_IOC IoC, int action,
	SK_U32 Id, char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int CsumStat(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int General(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Mac8023Stat(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int MacPrivateConf(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int MacPrivateStat(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Monitor(SK_AC *pAC, SK_IOC IoC, int action,
	SK_U32 Id, char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int OidStruct(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Perform(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int* pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Rlmt(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int RlmtStat(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int SensorStat(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Vpd(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
PNMI_STATIC int Vct(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);

#ifdef SK_POWER_MGMT
PNMI_STATIC int PowerManagement(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
#endif /* SK_POWER_MGMT */

#ifdef SK_DIAG_SUPPORT
PNMI_STATIC int DiagActions(SK_AC *pAC, SK_IOC IoC, int action, SK_U32 Id,
	char *pBuf, unsigned int *pLen, SK_U32 Instance,
	unsigned int TableIndex, SK_U32 NetIndex);
#endif /* SK_DIAG_SUPPORT */


/* defines *******************************************************************/
#define ID_TABLE_SIZE (sizeof(IdTable)/sizeof(IdTable[0]))


/* global variables **********************************************************/

/*
 * Table to correlate OID with handler function and index to
 * hardware register stored in StatAddress if applicable.
 */
PNMI_STATIC const SK_PNMI_TAB_ENTRY IdTable[] = {
	{OID_GEN_XMIT_OK,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX},
	{OID_GEN_RCV_OK,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX},
	{OID_GEN_XMIT_ERROR,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
	{OID_GEN_RCV_ERROR,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
	{OID_GEN_RCV_NO_BUFFER,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
	{OID_GEN_DIRECTED_FRAMES_XMIT,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_UNICAST},
	{OID_GEN_MULTICAST_FRAMES_XMIT,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_MULTICAST},
	{OID_GEN_BROADCAST_FRAMES_XMIT,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_BROADCAST},
	{OID_GEN_DIRECTED_FRAMES_RCV,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_UNICAST},
	{OID_GEN_MULTICAST_FRAMES_RCV,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_MULTICAST},
	{OID_GEN_BROADCAST_FRAMES_RCV,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_BROADCAST},
	{OID_GEN_RCV_CRC_ERROR,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_FCS},
	{OID_GEN_TRANSMIT_QUEUE_LENGTH,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
	{OID_802_3_PERMANENT_ADDRESS,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, 0},
	{OID_802_3_CURRENT_ADDRESS,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, 0},
	{OID_802_3_RCV_ERROR_ALIGNMENT,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_FRAMING},
	{OID_802_3_XMIT_ONE_COLLISION,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_SINGLE_COL},
	{OID_802_3_XMIT_MORE_COLLISIONS,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_MULTI_COL},
	{OID_802_3_XMIT_DEFERRED,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_DEFFERAL},
	{OID_802_3_XMIT_MAX_COLLISIONS,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_EXCESS_COL},
	{OID_802_3_RCV_OVERRUN,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HRX_OVERFLOW},
	{OID_802_3_XMIT_UNDERRUN,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_UNDERRUN},
	{OID_802_3_XMIT_TIMES_CRS_LOST,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_CARRIER},
	{OID_802_3_XMIT_LATE_COLLISIONS,
		0,
		0,
		0,
		SK_PNMI_RO, Mac8023Stat, SK_PNMI_HTX_LATE_COL},
#ifdef SK_POWER_MGMT
	{OID_PNP_CAPABILITIES,
		0,
		0,
		0,
		SK_PNMI_RO, PowerManagement, 0},
	{OID_PNP_SET_POWER,
		0,
		0,
		0,
		SK_PNMI_WO, PowerManagement, 0},
	{OID_PNP_QUERY_POWER,
		0,
		0,
		0,
		SK_PNMI_RO, PowerManagement, 0},
	{OID_PNP_ADD_WAKE_UP_PATTERN,
		0,
		0,
		0,
		SK_PNMI_WO, PowerManagement, 0},
	{OID_PNP_REMOVE_WAKE_UP_PATTERN,
		0,
		0,
		0,
		SK_PNMI_WO, PowerManagement, 0},
	{OID_PNP_ENABLE_WAKE_UP,
		0,
		0,
		0,
		SK_PNMI_RW, PowerManagement, 0},
#endif /* SK_POWER_MGMT */
#ifdef SK_DIAG_SUPPORT
	{OID_SKGE_DIAG_MODE,
		0,
		0,
		0,
		SK_PNMI_RW, DiagActions, 0},
#endif /* SK_DIAG_SUPPORT */
	{OID_SKGE_MDB_VERSION,
		1,
		0,
		SK_PNMI_MAI_OFF(MgmtDBVersion),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_SUPPORTED_LIST,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
	{OID_SKGE_ALL_DATA,
		0,
		0,
		0,
		SK_PNMI_RW, OidStruct, 0},
	{OID_SKGE_VPD_FREE_BYTES,
		1,
		0,
		SK_PNMI_MAI_OFF(VpdFreeBytes),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_ENTRIES_LIST,
		1,
		0,
		SK_PNMI_MAI_OFF(VpdEntriesList),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_ENTRIES_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(VpdEntriesNumber),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_KEY,
		SK_PNMI_VPD_ENTRIES,
		sizeof(SK_PNMI_VPD),
		SK_PNMI_OFF(Vpd) + SK_PNMI_VPD_OFF(VpdKey),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_VALUE,
		SK_PNMI_VPD_ENTRIES,
		sizeof(SK_PNMI_VPD),
		SK_PNMI_OFF(Vpd) + SK_PNMI_VPD_OFF(VpdValue),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_ACCESS,
		SK_PNMI_VPD_ENTRIES,
		sizeof(SK_PNMI_VPD),
		SK_PNMI_OFF(Vpd) + SK_PNMI_VPD_OFF(VpdAccess),
		SK_PNMI_RO, Vpd, 0},
	{OID_SKGE_VPD_ACTION,
		SK_PNMI_VPD_ENTRIES,
		sizeof(SK_PNMI_VPD),
		SK_PNMI_OFF(Vpd) + SK_PNMI_VPD_OFF(VpdAction),
		SK_PNMI_RW, Vpd, 0},
	{OID_SKGE_PORT_NUMBER,		
		1,
		0,
		SK_PNMI_MAI_OFF(PortNumber),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_DEVICE_TYPE,
		1,
		0,
		SK_PNMI_MAI_OFF(DeviceType),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_DRIVER_DESCR,
		1,
		0,
		SK_PNMI_MAI_OFF(DriverDescr),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_DRIVER_VERSION,
		1,
		0,
		SK_PNMI_MAI_OFF(DriverVersion),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_DRIVER_RELDATE,
		1,
		0,
		SK_PNMI_MAI_OFF(DriverReleaseDate),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_DRIVER_FILENAME,
		1,
		0,
		SK_PNMI_MAI_OFF(DriverFileName),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_HW_DESCR,
		1,
		0,
		SK_PNMI_MAI_OFF(HwDescr),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_HW_VERSION,
		1,
		0,
		SK_PNMI_MAI_OFF(HwVersion),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_CHIPSET,
		1,
		0,
		SK_PNMI_MAI_OFF(Chipset),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_CHIPID,
		1,
		0,
		SK_PNMI_MAI_OFF(ChipId),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RAMSIZE,
		1,
		0,
		SK_PNMI_MAI_OFF(RamSize),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_VAUXAVAIL,
		1,
		0,
		SK_PNMI_MAI_OFF(VauxAvail),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_ACTION,
		1,
		0,
		SK_PNMI_MAI_OFF(Action),
		SK_PNMI_RW, Perform, 0},
	{OID_SKGE_RESULT,
		1,
		0,
		SK_PNMI_MAI_OFF(TestResult),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_BUS_TYPE,
		1,
		0,
		SK_PNMI_MAI_OFF(BusType),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_BUS_SPEED,
		1,
		0,
		SK_PNMI_MAI_OFF(BusSpeed),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_BUS_WIDTH,
		1,
		0,
		SK_PNMI_MAI_OFF(BusWidth),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_SW_QUEUE_LEN,
		1,
		0,
		SK_PNMI_MAI_OFF(TxSwQueueLen),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_SW_QUEUE_MAX,
		1,
		0,
		SK_PNMI_MAI_OFF(TxSwQueueMax),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_RETRY,
		1,
		0,
		SK_PNMI_MAI_OFF(TxRetryCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RX_INTR_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RxIntrCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_INTR_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(TxIntrCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RX_NO_BUF_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RxNoBufCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_NO_BUF_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(TxNoBufCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_USED_DESCR_NO,
		1,
		0,
		SK_PNMI_MAI_OFF(TxUsedDescrNo),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RX_DELIVERED_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RxDeliveredCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RX_OCTETS_DELIV_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RxOctetsDeliveredCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RX_HW_ERROR_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RxHwErrorsCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TX_HW_ERROR_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(TxHwErrorsCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_IN_ERRORS_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(InErrorsCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_OUT_ERROR_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(OutErrorsCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_ERR_RECOVERY_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(ErrRecoveryCts),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_SYSUPTIME,
		1,
		0,
		SK_PNMI_MAI_OFF(SysUpTime),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_SENSOR_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(SensorNumber),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_SENSOR_INDEX,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorIndex),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_DESCR,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorDescr),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_TYPE,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorType),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_VALUE,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorValue),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_WAR_THRES_LOW,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorWarningThresholdLow),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_WAR_THRES_UPP,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorWarningThresholdHigh),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_ERR_THRES_LOW,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorErrorThresholdLow),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_ERR_THRES_UPP,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorErrorThresholdHigh),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_STATUS,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorStatus),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_WAR_CTS,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorWarningCts),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_ERR_CTS,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorErrorCts),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_WAR_TIME,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorWarningTimestamp),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_SENSOR_ERR_TIME,
		SK_PNMI_SENSOR_ENTRIES,
		sizeof(SK_PNMI_SENSOR),
		SK_PNMI_OFF(Sensor) + SK_PNMI_SEN_OFF(SensorErrorTimestamp),
		SK_PNMI_RO, SensorStat, 0},
	{OID_SKGE_CHKSM_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(ChecksumNumber),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_CHKSM_RX_OK_CTS,
		SKCS_NUM_PROTOCOLS,
		sizeof(SK_PNMI_CHECKSUM),
		SK_PNMI_OFF(Checksum) + SK_PNMI_CHK_OFF(ChecksumRxOkCts),
		SK_PNMI_RO, CsumStat, 0},
	{OID_SKGE_CHKSM_RX_UNABLE_CTS,
		SKCS_NUM_PROTOCOLS,
		sizeof(SK_PNMI_CHECKSUM),
		SK_PNMI_OFF(Checksum) + SK_PNMI_CHK_OFF(ChecksumRxUnableCts),
		SK_PNMI_RO, CsumStat, 0},
	{OID_SKGE_CHKSM_RX_ERR_CTS,
		SKCS_NUM_PROTOCOLS,
		sizeof(SK_PNMI_CHECKSUM),
		SK_PNMI_OFF(Checksum) + SK_PNMI_CHK_OFF(ChecksumRxErrCts),
		SK_PNMI_RO, CsumStat, 0},
	{OID_SKGE_CHKSM_TX_OK_CTS,
		SKCS_NUM_PROTOCOLS,
		sizeof(SK_PNMI_CHECKSUM),
		SK_PNMI_OFF(Checksum) + SK_PNMI_CHK_OFF(ChecksumTxOkCts),
		SK_PNMI_RO, CsumStat, 0},
	{OID_SKGE_CHKSM_TX_UNABLE_CTS,
		SKCS_NUM_PROTOCOLS,
		sizeof(SK_PNMI_CHECKSUM),
		SK_PNMI_OFF(Checksum) + SK_PNMI_CHK_OFF(ChecksumTxUnableCts),
		SK_PNMI_RO, CsumStat, 0},
	{OID_SKGE_STAT_TX,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX},
	{OID_SKGE_STAT_TX_OCTETS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxOctetsOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_OCTET},
	{OID_SKGE_STAT_TX_BROADCAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxBroadcastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_BROADCAST},
	{OID_SKGE_STAT_TX_MULTICAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxMulticastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_MULTICAST},
	{OID_SKGE_STAT_TX_UNICAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxUnicastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_UNICAST},
	{OID_SKGE_STAT_TX_LONGFRAMES,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxLongFramesCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_LONGFRAMES},
	{OID_SKGE_STAT_TX_BURST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxBurstCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_BURST},
	{OID_SKGE_STAT_TX_PFLOWC,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxPauseMacCtrlCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_PMACC},
	{OID_SKGE_STAT_TX_FLOWC,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxMacCtrlCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_MACC},
	{OID_SKGE_STAT_TX_SINGLE_COL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxSingleCollisionCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_SINGLE_COL},
	{OID_SKGE_STAT_TX_MULTI_COL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxMultipleCollisionCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_MULTI_COL},
	{OID_SKGE_STAT_TX_EXCESS_COL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxExcessiveCollisionCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_EXCESS_COL},
	{OID_SKGE_STAT_TX_LATE_COL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxLateCollisionCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_LATE_COL},
	{OID_SKGE_STAT_TX_DEFFERAL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxDeferralCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_DEFFERAL},
	{OID_SKGE_STAT_TX_EXCESS_DEF,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxExcessiveDeferralCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_EXCESS_DEF},
	{OID_SKGE_STAT_TX_UNDERRUN,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxFifoUnderrunCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_UNDERRUN},
	{OID_SKGE_STAT_TX_CARRIER,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxCarrierCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_CARRIER},
/*	{OID_SKGE_STAT_TX_UTIL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxUtilization),
		SK_PNMI_RO, MacPrivateStat, (SK_U16)(-1)}, */
	{OID_SKGE_STAT_TX_64,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTx64Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_64},
	{OID_SKGE_STAT_TX_127,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTx127Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_127},
	{OID_SKGE_STAT_TX_255,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTx255Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_255},
	{OID_SKGE_STAT_TX_511,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTx511Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_511},
	{OID_SKGE_STAT_TX_1023,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTx1023Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_1023},
	{OID_SKGE_STAT_TX_MAX,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxMaxCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_MAX},
	{OID_SKGE_STAT_TX_SYNC,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxSyncCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_SYNC},
	{OID_SKGE_STAT_TX_SYNC_OCTETS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatTxSyncOctetsCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HTX_SYNC_OCTET},
	{OID_SKGE_STAT_RX,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX},
	{OID_SKGE_STAT_RX_OCTETS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxOctetsOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_OCTET},
	{OID_SKGE_STAT_RX_BROADCAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxBroadcastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_BROADCAST},
	{OID_SKGE_STAT_RX_MULTICAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxMulticastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_MULTICAST},
	{OID_SKGE_STAT_RX_UNICAST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxUnicastOkCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_UNICAST},
	{OID_SKGE_STAT_RX_LONGFRAMES,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxLongFramesCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_LONGFRAMES},
	{OID_SKGE_STAT_RX_PFLOWC,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxPauseMacCtrlCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_PMACC},
	{OID_SKGE_STAT_RX_FLOWC,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxMacCtrlCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_MACC},
	{OID_SKGE_STAT_RX_PFLOWC_ERR,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxPauseMacCtrlErrorCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_PMACC_ERR},
	{OID_SKGE_STAT_RX_FLOWC_UNKWN,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxMacCtrlUnknownCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_MACC_UNKWN},
	{OID_SKGE_STAT_RX_BURST,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxBurstCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_BURST},
	{OID_SKGE_STAT_RX_MISSED,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxMissedCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_MISSED},
	{OID_SKGE_STAT_RX_FRAMING,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxFramingCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_FRAMING},
	{OID_SKGE_STAT_RX_OVERFLOW,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxFifoOverflowCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_OVERFLOW},
	{OID_SKGE_STAT_RX_JABBER,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxJabberCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_JABBER},
	{OID_SKGE_STAT_RX_CARRIER,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxCarrierCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_CARRIER},
	{OID_SKGE_STAT_RX_IR_LENGTH,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxIRLengthCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_IRLENGTH},
	{OID_SKGE_STAT_RX_SYMBOL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxSymbolCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_SYMBOL},
	{OID_SKGE_STAT_RX_SHORTS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxShortsCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_SHORTS},
	{OID_SKGE_STAT_RX_RUNT,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxRuntCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_RUNT},
	{OID_SKGE_STAT_RX_CEXT,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxCextCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_CEXT},
	{OID_SKGE_STAT_RX_TOO_LONG,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxTooLongCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_TOO_LONG},
	{OID_SKGE_STAT_RX_FCS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxFcsCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_FCS},
/*	{OID_SKGE_STAT_RX_UTIL,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxUtilization),
		SK_PNMI_RO, MacPrivateStat, (SK_U16)(-1)}, */
	{OID_SKGE_STAT_RX_64,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRx64Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_64},
	{OID_SKGE_STAT_RX_127,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRx127Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_127},
	{OID_SKGE_STAT_RX_255,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRx255Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_255},
	{OID_SKGE_STAT_RX_511,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRx511Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_511},
	{OID_SKGE_STAT_RX_1023,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRx1023Cts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_1023},
	{OID_SKGE_STAT_RX_MAX,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_STAT),
		SK_PNMI_OFF(Stat) + SK_PNMI_STA_OFF(StatRxMaxCts),
		SK_PNMI_RO, MacPrivateStat, SK_PNMI_HRX_MAX},
	{OID_SKGE_PHYS_CUR_ADDR,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfMacCurrentAddr),
		SK_PNMI_RW, Addr, 0},
	{OID_SKGE_PHYS_FAC_ADDR,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfMacFactoryAddr),
		SK_PNMI_RO, Addr, 0},
	{OID_SKGE_PMD,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfPMD),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_CONNECTOR,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfConnector),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_PHY_TYPE,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfPhyType),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_LINK_CAP,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfLinkCapability),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_LINK_MODE,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfLinkMode),
		SK_PNMI_RW, MacPrivateConf, 0},
	{OID_SKGE_LINK_MODE_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfLinkModeStatus),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_LINK_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfLinkStatus),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_FLOWCTRL_CAP,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfFlowCtrlCapability),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_FLOWCTRL_MODE,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfFlowCtrlMode),
		SK_PNMI_RW, MacPrivateConf, 0},
	{OID_SKGE_FLOWCTRL_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfFlowCtrlStatus),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_PHY_OPERATION_CAP,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfPhyOperationCapability),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_PHY_OPERATION_MODE,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfPhyOperationMode),
		SK_PNMI_RW, MacPrivateConf, 0},
	{OID_SKGE_PHY_OPERATION_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfPhyOperationStatus),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_SPEED_CAP,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfSpeedCapability),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_SPEED_MODE,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfSpeedMode),
		SK_PNMI_RW, MacPrivateConf, 0},
	{OID_SKGE_SPEED_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_CONF),
		SK_PNMI_OFF(Conf) + SK_PNMI_CNF_OFF(ConfSpeedStatus),
		SK_PNMI_RO, MacPrivateConf, 0},
	{OID_SKGE_TRAP,
		1,
		0,
		SK_PNMI_MAI_OFF(Trap),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_TRAP_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(TrapNumber),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RLMT_MODE,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtMode),
		SK_PNMI_RW, Rlmt, 0},
	{OID_SKGE_RLMT_PORT_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtPortNumber),
		SK_PNMI_RO, Rlmt, 0},
	{OID_SKGE_RLMT_PORT_ACTIVE,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtPortActive),
		SK_PNMI_RO, Rlmt, 0},
	{OID_SKGE_RLMT_PORT_PREFERRED,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtPortPreferred),
		SK_PNMI_RW, Rlmt, 0},
	{OID_SKGE_RLMT_CHANGE_CTS,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtChangeCts),
		SK_PNMI_RO, Rlmt, 0},
	{OID_SKGE_RLMT_CHANGE_TIME,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtChangeTime),
		SK_PNMI_RO, Rlmt, 0},
	{OID_SKGE_RLMT_CHANGE_ESTIM,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtChangeEstimate),
		SK_PNMI_RO, Rlmt, 0},
	{OID_SKGE_RLMT_CHANGE_THRES,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtChangeThreshold),
		SK_PNMI_RW, Rlmt, 0},
	{OID_SKGE_RLMT_PORT_INDEX,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtIndex),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_STATUS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtStatus),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_TX_HELLO_CTS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtTxHelloCts),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_RX_HELLO_CTS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtRxHelloCts),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_TX_SP_REQ_CTS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtTxSpHelloReqCts),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_RX_SP_CTS,
		SK_PNMI_MAC_ENTRIES,
		sizeof(SK_PNMI_RLMT),
		SK_PNMI_OFF(Rlmt) + SK_PNMI_RLM_OFF(RlmtRxSpHelloCts),
		SK_PNMI_RO, RlmtStat, 0},
	{OID_SKGE_RLMT_MONITOR_NUMBER,
		1,
		0,
		SK_PNMI_MAI_OFF(RlmtMonitorNumber),
		SK_PNMI_RO, General, 0},
	{OID_SKGE_RLMT_MONITOR_INDEX,
		SK_PNMI_MONITOR_ENTRIES,
		sizeof(SK_PNMI_RLMT_MONITOR),
		SK_PNMI_OFF(RlmtMonitor) + SK_PNMI_MON_OFF(RlmtMonitorIndex),
		SK_PNMI_RO, Monitor, 0},
	{OID_SKGE_RLMT_MONITOR_ADDR,
		SK_PNMI_MONITOR_ENTRIES,
		sizeof(SK_PNMI_RLMT_MONITOR),
		SK_PNMI_OFF(RlmtMonitor) + SK_PNMI_MON_OFF(RlmtMonitorAddr),
		SK_PNMI_RO, Monitor, 0},
	{OID_SKGE_RLMT_MONITOR_ERRS,
		SK_PNMI_MONITOR_ENTRIES,
		sizeof(SK_PNMI_RLMT_MONITOR),
		SK_PNMI_OFF(RlmtMonitor) + SK_PNMI_MON_OFF(RlmtMonitorErrorCts),
		SK_PNMI_RO, Monitor, 0},
	{OID_SKGE_RLMT_MONITOR_TIMESTAMP,
		SK_PNMI_MONITOR_ENTRIES,
		sizeof(SK_PNMI_RLMT_MONITOR),
		SK_PNMI_OFF(RlmtMonitor) + SK_PNMI_MON_OFF(RlmtMonitorTimestamp),
		SK_PNMI_RO, Monitor, 0},
	{OID_SKGE_RLMT_MONITOR_ADMIN,
		SK_PNMI_MONITOR_ENTRIES,
		sizeof(SK_PNMI_RLMT_MONITOR),
		SK_PNMI_OFF(RlmtMonitor) + SK_PNMI_MON_OFF(RlmtMonitorAdmin),
		SK_PNMI_RW, Monitor, 0},
	{OID_SKGE_MTU,
		1,
		0,
		SK_PNMI_MAI_OFF(MtuSize),
		SK_PNMI_RW, MacPrivateConf, 0},
	{OID_SKGE_VCT_GET,
		0,
		0,
		0,
		SK_PNMI_RO, Vct, 0},
	{OID_SKGE_VCT_SET,
		0,
		0,
		0,
		SK_PNMI_WO, Vct, 0},
	{OID_SKGE_VCT_STATUS,
		0,
		0,
		0,
		SK_PNMI_RO, Vct, 0},
	{OID_SKGE_BOARDLEVEL,
		0,
		0,
		0,
		SK_PNMI_RO, General, 0},
};

