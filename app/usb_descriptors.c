#include <tusb.h>

#define CONFIG_TOTAL_LEN		(TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_HID_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_HID_KEYBOARD		0x81
#define EPNUM_HID_MOUSE			0x82
#define EPNUM_HID_GENERIC		0x83

#define EPNUM_VENDOR_IN			0x84
#define EPNUM_VENDOR_OUT		0x02

#define EPNUM_CDC_CMD			0x85
#define EPNUM_CDC_IN			0x86
#define EPNUM_CDC_OUT			0x03

#define CDC_CMD_MAX_SIZE		8
#define CDC_IN_OUT_MAX_SIZE		64

static uint16_t temp_string[32];

char const *string_descriptors[] =
{
	(const char[]) { 0x09, 0x04 },	// 0: is supported language is English (0x0409)
	"Solder Party",					// 1: Manufacturer
	USB_PRODUCT,					// 2: Product
	"123456",						// 3: Serials, should use chip ID
	"Keyboard Interface",			// 4: Interface 1 String
	"Mouse Interface",				// 5: Interface 2 String
	"HID Interface",				// 6: Interface 3 String
	"CDC Interface",				// 7: Interface 4 String
};

tusb_desc_device_t const device_descriptor =
{
	.bLength			= sizeof(tusb_desc_device_t),
	.bDescriptorType	= TUSB_DESC_DEVICE,
	.bcdUSB				= 0x0200,
	.bDeviceClass		= 0x00,
	.bDeviceSubClass	= 0x00,
	.bDeviceProtocol	= 0x00,
	.bMaxPacketSize0	= CFG_TUD_ENDPOINT0_SIZE,

	.idVendor			= USB_VID,
	.idProduct			= USB_PID,
	.bcdDevice			= 0x1000,

	.iManufacturer		= 0x01,
	.iProduct			= 0x02,
	.iSerialNumber		= 0x03,

	.bNumConfigurations	= 0x01
};

uint8_t const hid_keyboard_descriptor[] =
{
	TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const hid_mouse_descriptor[] =
{
	TUD_HID_REPORT_DESC_MOUSE()
};

uint8_t const config_descriptor[] =
{
	TUD_CONFIG_DESCRIPTOR(1, USB_ITF_MAX, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

	TUD_HID_DESCRIPTOR(USB_ITF_KEYBOARD,    4, HID_ITF_PROTOCOL_NONE, sizeof(hid_keyboard_descriptor), EPNUM_HID_KEYBOARD, CFG_TUD_HID_EP_BUFSIZE, 10),
	TUD_HID_DESCRIPTOR(USB_ITF_MOUSE,       5, HID_ITF_PROTOCOL_NONE, sizeof(hid_mouse_descriptor),    EPNUM_HID_MOUSE,    CFG_TUD_HID_EP_BUFSIZE, 10),

	TUD_VENDOR_DESCRIPTOR(USB_ITF_VENDOR,   7, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, CFG_TUD_VENDOR_EPSIZE),

	TUD_CDC_DESCRIPTOR(USB_ITF_CDC, 7, EPNUM_CDC_CMD, CDC_CMD_MAX_SIZE, EPNUM_CDC_OUT, EPNUM_CDC_IN, CDC_IN_OUT_MAX_SIZE),
};

uint8_t const *tud_descriptor_device_cb(void)
{
	return (uint8_t const*)&device_descriptor;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf)
{
	if (itf == USB_ITF_KEYBOARD)
		return hid_keyboard_descriptor;

	if (itf == USB_ITF_MOUSE)
		return hid_mouse_descriptor;

	return NULL;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void) index;

	return config_descriptor;
}

uint16_t const *tud_descriptor_string_cb(uint8_t idx, uint16_t langid)
{
	(void) langid;

	if (idx == 0) {
		temp_string[0] = (TUSB_DESC_STRING << 8 ) | (2 * sizeof(uint16_t));
		memcpy(&temp_string[1], string_descriptors[0], 2);
		return temp_string;
	}

	if (!(idx < sizeof(string_descriptors) / sizeof(string_descriptors[0])))
		return NULL;

	const char *str = string_descriptors[idx];
	uint8_t size = strlen(str);
	if (size > 31)
		size = 31;

	// Convert ASCII string into UTF-16
	for(uint8_t i = 0; i < size; ++i)
		temp_string[1 + i] = str[i];

	temp_string[0] = (TUSB_DESC_STRING << 8 ) | ((size + 1) * sizeof(uint16_t));

	return temp_string;
}
