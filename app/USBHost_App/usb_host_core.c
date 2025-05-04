#include "usb_host_core.h"
#include "ch32v20x_usb.h"
#include "ch32v20x_usbfs_host.h"
#include "usb_host_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

RootHubDevice rootHubDev;
struct __HOST_CTL HostCtl[DEF_TOTAL_ROOT_HUB * DEF_ONE_USB_SUP_DEV_TOTAL];

uint8_t Com_Buf[DEF_COM_BUF_LEN]; // General Buffer

static void PrintData(ssize_t size, uint8_t *data) {
  for (ssize_t i = 0; i < 18; i++) {
    printf("%02x ", data[i]);
  }
  printf("\r\n");
}

static void CheckDevType(void) {
  switch (rootHubDev.type) {
  case USB_DEV_CLASS_HID:
    printf("Enumerating HID device. ");
    rootHubDev.status = ROOT_DEV_SUCCESS;
    break;
  default:
    printf("Root device is of unsupported class ");
    switch (rootHubDev.type) {
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
    rootHubDev.status = ROOT_DEV_FAILED;
    break;
  }
  printf("Ending Enumeration.\r\n");
}

void USBH_HostInit(void) {
  USBFS_RCC_Init();
  USBFS_Host_Init(ENABLE);
  memset(&rootHubDev.status, 0, sizeof(RootHubDevice));
  /* memset( */
  /*     &HostCtl[DEF_USBFS_PORT_INDEX *
   * DEF_ONE_USB_SUP_DEV_TOTAL].InterfaceNum, */
  /*     0, DEF_ONE_USB_SUP_DEV_TOTAL * sizeof(HOST_CTL)); */
}

void USBH_AnalyseType(USB_DEV_DESCR *dev, USB_ITF_DESCR *itf, uint8_t *ptype) {
  uint8_t dv_cls, if_cls;

  dv_cls = dev->bDeviceClass;
  if_cls = itf->bInterfaceClass;
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

uint8_t USBH_EnumDevice(Device *dev) {
  uint8_t status;
  uint8_t enum_cnt = 0;
  uint16_t i = 0;
  uint16_t len = 0;
  uint8_t DevDesc_Buf[18]; // Device Descriptor Buffer

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
    if (USBFSH_EnableRootHubPort(&rootHubDev.speed) == ERR_SUCCESS) {
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
  status = USBFSH_GetDeviceDescr(&rootHubDev.ep0MaxPks, DevDesc_Buf);
  if (status == ERR_SUCCESS) {
    memcpy(&dev->devDescriptor, DevDesc_Buf, 18);
    PrintData(18, DevDesc_Buf);
    printf("\tVendor ID: 0x%04x\r\n", dev->devDescriptor.idVendor);
    printf("\tProduct ID: 0x%04x\r\n", dev->devDescriptor.idProduct);

  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_DEV_DESCR_GETFAIL;
  }

  /* Set Address of device */
  rootHubDev.address = (uint8_t)(USB_DEVICE_ADDR);
  status = USBFSH_SetUsbAddress(rootHubDev.ep0MaxPks, rootHubDev.address);
  if (status == ERR_SUCCESS) {
    printf("Device address successfully set: %02x\r\n", rootHubDev.address);

    rootHubDev.address = USB_DEVICE_ADDR;
  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_DEV_ADDR_SETFAIL;
  }
  Delay_Ms(5);

  /* Get configuration descriptor */
  printf("Configuration descriptor:\r\n\t");
  status = USBFSH_GetConfigDescr(rootHubDev.ep0MaxPks, Com_Buf, DEF_COM_BUF_LEN,
                                 &len);
  uint8_t cfg_val;
  if (status == ERR_SUCCESS) {
    cfg_val = dev->cfgDescriptor.bConfigurationValue;

    /* TODO: Upgrade enumeration to get the correct amount of descriptors, not
     * just the first one. And this code may crash if no interface descriptor
     * are sent but GetConfigDescr dosn't return an error somehow. More of a
     * reason to upgrade it.*/
    memcpy(&dev->cfgDescriptor, ((USB_CFG_DESCR *)Com_Buf),
           sizeof(USB_CFG_DESCR));
    memcpy(&dev->itfDescritor, &((USB_CFG_DESCR_LONG *)Com_Buf)->itf_descr,
           sizeof(USB_ITF_DESCR));

    PrintData(len, Com_Buf);

    /* Analyze USB device type  */
    USBH_AnalyseType(&dev->devDescriptor, &dev->itfDescritor, &rootHubDev.type);
    printf("\tDevice type: %02x\r\n", rootHubDev.type);
  } else {
    printf("Err(%02x)\r\n", status);
    if (enum_cnt <= ENUM_MAX_TRIES)
      goto ENUM_START;
    return DEF_CFG_DESCR_GETFAIL;
  }

  /* Set chosen configuration */
  status = USBFSH_SetUsbConfig(rootHubDev.ep0MaxPks, cfg_val);
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

uint8_t USBFSH_CoreRootHubPortStatus() {
  if (USBFSH->INT_FG &
      USBFS_UIF_DETECT) // Check that there is a device connection or
                        // disconnection event on the port
  {
    USBFSH->INT_FG = USBFS_UIF_DETECT;         // Clear flag
    if (USBFSH->MIS_ST & USBFS_UMS_DEV_ATTACH) // Check that there is a device
                                               // connection to the port
    {
      if (USBFSH_CheckRootHubPortEnable() == 0x00) {
        return ROOT_DEV_CONNECTED;
      } else {
        return ROOT_DEV_FAILED;
      }
    } else // Check that there is no device connection to the port
    {
      return ROOT_DEV_DISCONNECT;
    }
  } else {
    return ROOT_DEV_FAILED;
  }
}

void USBH_Core(USBH_AppCb cb) {
  uint8_t index;
  uint8_t status;
  Device dev;

  /* Check USB port for connection */
  status = USBFSH_CoreRootHubPortStatus();
  switch (rootHubDev.status) {
  case ROOT_DEV_DISCONNECT:
    if (status == ROOT_DEV_CONNECTED) {
      printf("USB device detected on host port\r\n");
      rootHubDev.status = ROOT_DEV_CONNECTED;
      rootHubDev.deviceIndex = DEF_USBFS_PORT_INDEX * DEF_ONE_USB_SUP_DEV_TOTAL;

    } else {
      printf("No USB device detected on host port\r\n");
      index = rootHubDev.deviceIndex;
      memset(&rootHubDev.status, 0, sizeof(RootHubDevice));
      memset(&HostCtl[index].InterfaceNum, 0, sizeof(HOST_CTL));
    }
    break;
  case ROOT_DEV_CONNECTED:
    if (status == ROOT_DEV_DISCONNECT) {
      rootHubDev.status = status;
      break;
    }
    /* Enumerate device */
    status = USBH_EnumDevice(&dev);
    if (status == ERR_SUCCESS) {
      CheckDevType();
    } else if (status != ERR_USB_DISCON) {
      printf("Enumeration failed with error code:%x\r\n", status);
      rootHubDev.status = ROOT_DEV_FAILED;
    }
    break;
  case ROOT_DEV_FAILED:
    if (status == ROOT_DEV_DISCONNECT) {
      rootHubDev.status = status;
      break;
    }
    printf("USB host failed\r\n");
    break;
  case ROOT_DEV_SUCCESS:
    if (status == ROOT_DEV_DISCONNECT) {
      rootHubDev.status = status;
      break;
    }
    /* TODO: Run driver */
    break;
  }
}
