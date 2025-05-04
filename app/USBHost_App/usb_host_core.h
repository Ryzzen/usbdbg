#ifndef __USB_HOST_CORE_H
#define __USB_HOST_CORE_H

#include "ch32v20x_usb.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "usb_host_config.h"
#include <stdint.h>

#define DEF_COM_BUF_LEN 1024
#define ENUM_MAX_TRIES 5

typedef void (*USBH_AppCb)(void);

typedef struct _HubDevice {
  uint8_t status;
  uint8_t type;
  uint8_t address;
  uint8_t speed;
  uint8_t ep0MaxPks;
  uint8_t deviceIndex;
} HubDevice;

typedef struct _RootHubDevice {
  uint8_t status;
  uint8_t type;
  uint8_t address;
  uint8_t speed;
  uint8_t ep0MaxPks;
  uint8_t deviceIndex;
  uint8_t portNum;
  HubDevice device[DEF_NEXT_HUB_PORT_NUM_MAX];
} RootHubDevice;

/* TODO: Add support for multiple descriptors and configurations */
typedef struct _Device {
  USB_DEV_DESCR devDescriptor;
  USB_CFG_DESCR cfgDescriptor;
  USB_ITF_DESCR itfDescritor;
} Device;

void USBH_HostInit(void);
void USBH_Core(USBH_AppCb cb);

extern RootHubDevice RootHubDev;
/* extern __HOST_CTL HostCtl[]; */

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST_CORE_H */
