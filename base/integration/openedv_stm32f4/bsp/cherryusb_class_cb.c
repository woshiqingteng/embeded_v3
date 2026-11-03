#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "usbd_core.h"
#include "usbd_cdc_acm.h"
#include "usbd_msc.h"
#include "mw_cherryusb.h"
#include "bsp_sd.h"

#define CDC_IN_EP   0x81U
#define CDC_OUT_EP  0x01U
#define CDC_INT_EP  0x82U
#define MSC_IN_EP   0x83U
#define MSC_OUT_EP  0x02U

#define USBD_VID    0x0483U
#define USBD_PID    0x5740U
#define USBD_MAX_POWER 100U
#define CDC_MAX_MPS 64U
#define MSC_MAX_MPS 64U

#define USB_CONFIG_SIZE (9U + CDC_ACM_DESCRIPTOR_LEN + MSC_DESCRIPTOR_LEN)

static const uint8_t device_descriptor[] =
{
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01,
                               USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor[] =
{
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01,
                               USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP,
                            CDC_MAX_MPS, 0x02),
    MSC_DESCRIPTOR_INIT(0x02, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x00)
};

static const uint8_t device_quality_descriptor[] =
{
    0x0A, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00
};

static char serial_string[25];
static const char langid_string[] = { (char)0x09, (char)0x04 };

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    switch (index)
    {
        case 0: return langid_string;
        case 1: return "ALIENTEK";
        case 2: return "STM32F429 MSC + CDC";
        case 3:
            (void)snprintf(serial_string, sizeof(serial_string), "%08lX%08lX",
                           (unsigned long)(*(const uint32_t *)0x1FFF7A10U),
                           (unsigned long)(*(const uint32_t *)0x1FFF7A14U));
            return serial_string;
        default: return 0;
    }
}

static const struct usb_descriptor composite_descriptor =
{
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = 0,
    .string_descriptor_callback = string_descriptor_callback,
};

static volatile uint8_t g_event;
static volatile bool    g_msc_active;
volatile uint32_t g_msc_read_calls;
volatile uint32_t g_msc_read_ok;
volatile uint32_t g_msc_read_fail;
volatile uint32_t g_msc_last_lba;
volatile uint32_t g_msc_last_len;
volatile uint32_t g_msc_write_calls;
volatile uint32_t g_msc_write_ok;
volatile uint32_t g_msc_write_fail;
static mw_cherryusb_msc_prepare_cb_t g_msc_prepare;
static mw_cherryusb_cdc_rx_cb_t      g_cdc_rx_cb;
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_cdc_rx_buf[512];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_cdc_tx_buf[512];
static volatile bool g_cdc_tx_busy;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
        case USBD_EVENT_RESET:
        case USBD_EVENT_DISCONNECTED:
        case USBD_EVENT_CONFIGURED:
            g_event = event;
            break;
        default:
            break;
    }

    if (event == USBD_EVENT_CONFIGURED)
    {
        g_cdc_tx_busy = false;
        (void)usbd_ep_start_read(busid, CDC_OUT_EP, g_cdc_rx_buf, sizeof(g_cdc_rx_buf));
    }
}

static void cdc_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;
    if ((g_cdc_rx_cb != 0) && (nbytes > 0U))
    {
        g_cdc_rx_cb(g_cdc_rx_buf, nbytes);
    }
    (void)usbd_ep_start_read(busid, CDC_OUT_EP, g_cdc_rx_buf, sizeof(g_cdc_rx_buf));
}

static void cdc_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if ((nbytes != 0U) && ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U))
    {
        (void)usbd_ep_start_write(busid, CDC_IN_EP, 0, 0);
    }
    else
    {
        g_cdc_tx_busy = false;
    }
}

static struct usbd_endpoint cdc_out_ep =
{
    .ep_addr = CDC_OUT_EP,
    .ep_cb = cdc_bulk_out,
};

static struct usbd_endpoint cdc_in_ep =
{
    .ep_addr = CDC_IN_EP,
    .ep_cb = cdc_bulk_in,
};

static struct usbd_interface intf_cdc0;
static struct usbd_interface intf_cdc1;
static struct usbd_interface intf_msc;

void mw_cherryusb_set_msc_prepare_cb(mw_cherryusb_msc_prepare_cb_t cb)
{
    g_msc_prepare = cb;
}

void mw_cherryusb_set_cdc_rx_cb(mw_cherryusb_cdc_rx_cb_t cb)
{
    g_cdc_rx_cb = cb;
}

uint8_t mw_cherryusb_take_event(void)
{
    uint8_t event = g_event;
    g_event = 0U;
    return event;
}

void mw_cherryusb_msc_set_active(bool active)
{
    g_msc_active = active;
}

bool mw_cherryusb_cdc_write(const uint8_t *data, uint32_t len)
{
    if (g_cdc_tx_busy || (data == 0) || (len == 0U) || (len > sizeof(g_cdc_tx_buf)))
    {
        return false;
    }
    memcpy(g_cdc_tx_buf, data, len);
    g_cdc_tx_busy = true;
    if (usbd_ep_start_write(0, CDC_IN_EP, g_cdc_tx_buf, len) != 0)
    {
        g_cdc_tx_busy = false;
        return false;
    }
    return true;
}

int mw_cherryusb_init(void)
{
    usbd_desc_register(0, &composite_descriptor);

    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf_cdc0));
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf_cdc1));
    usbd_add_endpoint(0, &cdc_out_ep);
    usbd_add_endpoint(0, &cdc_in_ep);
    usbd_add_interface(0, usbd_msc_init_intf(0, &intf_msc, MSC_OUT_EP, MSC_IN_EP));

    return usbd_initialize(0, USB_OTG_FS_PERIPH_BASE, usbd_event_handler);
}

/* STM32 MTP/CDC weak hooks. */
void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    (void)dtr;
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf,
                                  struct cdc_line_coding *line_coding)
{
    (void)busid;
    (void)intf;
    (void)line_coding;
}

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num,
                      uint32_t *block_size)
{
    sd_card_info_t info;

    (void)busid;
    (void)lun;

    *block_num = 0U;
    *block_size = 512U;

    if (bsp_sd_get_info(&info) == 0)
    {
        *block_num = info.block_count;
        *block_size = info.block_size;
    }
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector,
                         uint8_t *buffer, uint32_t length)
{
    int ret;

    (void)busid;
    (void)lun;

    g_msc_read_calls++;
    g_msc_last_lba = sector;
    g_msc_last_len = length;

    if (g_msc_prepare != 0 && !g_msc_prepare())
    {
        g_msc_read_fail++;
        return -1;
    }
    if ((length % 512U) != 0U)
    {
        g_msc_read_fail++;
        return -1;
    }
    ret = bsp_sd_read(buffer, sector, length / 512U);
    if (ret == 0)
    {
        g_msc_read_ok++;
    }
    else
    {
        g_msc_read_fail++;
    }
    return ret;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector,
                          uint8_t *buffer, uint32_t length)
{
    int ret;

    (void)busid;
    (void)lun;

    g_msc_write_calls++;
    g_msc_last_lba = sector;
    g_msc_last_len = length;

    if (g_msc_prepare != 0 && !g_msc_prepare())
    {
        g_msc_write_fail++;
        return -1;
    }
    if ((length % 512U) != 0U)
    {
        g_msc_write_fail++;
        return -1;
    }
    ret = bsp_sd_write(buffer, sector, length / 512U);
    if (ret == 0)
    {
        g_msc_write_ok++;
    }
    else
    {
        g_msc_write_fail++;
    }
    return ret;
}
