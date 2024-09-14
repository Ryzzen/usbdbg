#include "usb_host_app.h"
#include "usb_host_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct _ROOT_HUB_DEVICE RootHubDev;
struct __HOST_CTL HostCtl[DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL];

uint8_t DevDesc_Buf[18];          // Device Descriptor Buffer
uint8_t Com_Buf[DEF_COM_BUF_LEN]; // General Buffer

/**
 * @brief Print content of a data buffer as hex value
 *
 * @param size [Size of the buffer]
 * @param data [Data to print]
 */
void PrintData(ssize_t size, uint8_t *data) {
  for (ssize_t i = 0; i < 18; i++) {
    printf("%02x ", data[i]);
  }
  printf("\r\n");
}

void USBH_AnalyseType(uint8_t *pdev_buf, uint8_t *pcfg_buf, uint8_t *ptype) {
  uint8_t dv_cls, if_cls;

  dv_cls = ((USB_DEV_DESCR *)pdev_buf)->bDeviceClass;
  if_cls = ((USB_CFG_DESCR_LONG *)pcfg_buf)->itf_descr.bInterfaceClass;
  if ((dv_cls == USB_DEV_CLASS_STORAGE) || (if_cls == USB_DEV_CLASS_STORAGE)) {
    *ptype = USB_DEV_CLASS_STORAGE;
  } else if ((dv_cls == USB_DEV_CLASS_PRINTER) ||
             (if_cls == USB_DEV_CLASS_PRINTER)) {
    *ptype = USB_DEV_CLASS_PRINTER;
  } else if ((dv_cls == USB_DEV_CLASS_HID) || (if_cls == USB_DEV_CLASS_HID)) {
    *ptype = USB_DEV_CLASS_HID;
  } else if ((dv_cls == USB_DEV_CLASS_HUB) || (if_cls == USB_DEV_CLASS_HUB)) {
    *ptype = USB_DEV_CLASS_HUB;
  } else {
    *ptype = DEF_DEV_TYPE_UNKNOWN;
  }
}

uint8_t USBH_EnumDevice(void) {
  uint8_t status;
  uint8_t enum_cnt = 0;
  uint16_t i = 0;
  uint16_t len = 0;

  printf("Starting device enumration.\r\n");

ENUM_START:
  /* Delay and wait for the device to stabilize */
  Delay_Ms(100);
  enum_cnt++;
  Delay_Ms(8 << enum_cnt);

  /* Reset USB port and set USB speed */
  printf("Reset USB port and set USB speed.\r\n");
  USBFSH_ResetRootHubPort(0);
  for (i = 0, status = 0; i < DEF_RE_ATTACH_TIMEOUT; i++) {
    if (USBFSH_EnableRootHubPort(&RootHubDev.bSpeed) == ERR_SUCCESS) {
      i = 0;
      status++;
      if (status > 6) {
        break;
      }
    }
    Delay_Ms(1);
  }
  if (i) {
    /* Determine whether the maximum number of retries has been reached, and
     * retry if not reached */
    if (enum_cnt <= ENUM_MAX_TRIES) {
      goto ENUM_START;
    }
    return ERR_USB_DISCON;
  }

  /* Get device descriptor */
  printf("Device descriptor:\r\n\t");
  status = USBFSH_GetDeviceDescr(&RootHubDev.bEp0MaxPks, DevDesc_Buf);
  if (status == ERR_SUCCESS)
    PrintData(18, DevDesc_Buf);
  else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_DEV_DESCR_GETFAIL;
  }

  /* Set Address of device */
  RootHubDev.bAddress = (uint8_t)(USB_DEVICE_ADDR);
  status = USBFSH_SetUsbAddress(RootHubDev.bEp0MaxPks, RootHubDev.bAddress);
  if (status == ERR_SUCCESS) {
    printf("Device address successfully set: %02x\r\n", RootHubDev.bAddress);

    RootHubDev.bAddress = USB_DEVICE_ADDR;
  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_DEV_ADDR_SETFAIL;
  }
  Delay_Ms(5);

  /* Get configuration descriptor */
  printf("Configuration descriptor:\r\n\t");
  status = USBFSH_GetConfigDescr(RootHubDev.bEp0MaxPks, Com_Buf,
                                 DEF_COM_BUF_LEN, &len);
  uint8_t cfg_val;
  if (status == ERR_SUCCESS) {
    cfg_val = ((PUSB_CFG_DESCR)Com_Buf)->bConfigurationValue;

    PrintData(len, Com_Buf);

    /* Analyze USB device type  */
    USBH_AnalyseType(DevDesc_Buf, Com_Buf, &RootHubDev.bType);
    printf("\tDevice type: %02x\r\n", RootHubDev.bType);
  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_CFG_DESCR_GETFAIL;
  }

  /* Set chosen configuration */
  status = USBFSH_SetUsbConfig(RootHubDev.bEp0MaxPks, cfg_val);
  if (status == ERR_SUCCESS) {
    printf("Chosen configuration successfully set: %02x\r\n", cfg_val);
  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return ERR_USB_UNSUPPORT;
  }
  return ERR_SUCCESS;
}

/**
 * @brief USB Host initialisation
 */
void USBH_HostInit(void) {
  USBFS_RCC_Init();
  USBFS_Host_Init(ENABLE);
  memset(&RootHubDev.bStatus, 0, sizeof(ROOT_HUB_DEVICE));
  memset(
      &HostCtl[DEF_USBFS_PORT_INDEX * DEF_ONE_USB_SUP_DEV_TOTAL].InterfaceNum,
      0, DEF_ONE_USB_SUP_DEV_TOTAL * sizeof(HOST_CTL));
}

void USBH_App(void) {
  uint8_t status;
  uint8_t index;

  /* Detect device */
  status = USBFSH_CheckRootHubPortStatus(RootHubDev.bStatus);

  if (status == ROOT_DEV_CONNECTED) {
    printf("USB device detected.\r\n");

    RootHubDev.bStatus = ROOT_DEV_CONNECTED;
    RootHubDev.DeviceIndex = DEF_USBFS_PORT_INDEX * DEF_ONE_USB_SUP_DEV_TOTAL;

    /* Root device enumeration */
    status = USBH_EnumDevice();

    if (status == ERR_SUCCESS) {
      switch (RootHubDev.bType) {
      case USB_DEV_CLASS_HID:
        printf("Enumerating HID device.");
        break;
      default:
        printf("Root device is of unsupported class ");
        switch (RootHubDev.bType) {
        case USB_DEV_CLASS_STORAGE:
          printf("Storage. ");
          break;
        case USB_DEV_CLASS_PRINTER:
          printf("Printer. ");
          break;
        case DEF_DEV_TYPE_UNKNOWN:
          printf("Unknown. ");
          break;
        }
        printf("Ending Enumeration.\r\n");

        RootHubDev.bStatus = ROOT_DEV_SUCCESS;
        break;
      }
    }
  } else if (status == ROOT_DEV_DISCONNECT) {
    printf("USB device unplugged.");

    index = RootHubDev.DeviceIndex;
    memset(&RootHubDev.bStatus, 0, sizeof(ROOT_HUB_DEVICE));
    memset(&HostCtl[index].InterfaceNum, 0, sizeof(HOST_CTL));
  }
  return;
}
