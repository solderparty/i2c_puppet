import usb


_REG_VER = 0x01  # fw version
_REG_CFG = 0x02  # config
_REG_INT = 0x03  # interrupt status
_REG_KEY = 0x04  # key status
_REG_BKL = 0x05  # backlight
_REG_DEB = 0x06  # debounce cfg
_REG_FRQ = 0x07  # poll freq cfg
_REG_RST = 0x08  # reset
_REG_FIF = 0x09  # fifo
_REG_BK2 = 0x0A  # backlight 2
_REG_DIR = 0x0B  # gpio direction
_REG_PUE = 0x0C  # gpio input pull enable
_REG_PUD = 0x0D  # gpio input pull direction
_REG_GIO = 0x0E  # gpio value
_REG_GIC = 0x0F  # gpio interrupt config
_REG_GIN = 0x10  # gpio interrupt status
_REG_HLD = 0x11  # key hold time cfg (in 10ms units)
_REG_ADR = 0x12  # i2c puppet address
_REG_IND = 0x13  # interrupt pin assert duration
_REG_CF2 = 0x14  # config 2
_REG_TOX = 0x15  # touch delta x since last read, at most (-128 to 127)
_REG_TOY = 0x16  # touch delta y since last read, at most (-128 to 127)

_WRITE_MASK      = 1 << 7

CFG_OVERFLOW_ON  = 1 << 0
CFG_OVERFLOW_INT = 1 << 1
CFG_CAPSLOCK_INT = 1 << 2
CFG_NUMLOCK_INT  = 1 << 3
CFG_KEY_INT      = 1 << 4
CFG_PANIC_INT    = 1 << 5
CFG_REPORT_MODS  = 1 << 6
CFG_USE_MODS     = 1 << 7

CF2_TOUCH_INT    = 1 << 0
CF2_USB_KEYB_ON  = 1 << 1
CF2_USB_MOUSE_ON = 1 << 2

INT_OVERFLOW     = 1 << 0
INT_CAPSLOCK     = 1 << 1
INT_NUMLOCK      = 1 << 2
INT_KEY          = 1 << 3
INT_PANIC        = 1 << 4
INT_GPIO         = 1 << 5
INT_TOUCH        = 1 << 6

KEY_CAPSLOCK     = 1 << 5
KEY_NUMLOCK      = 1 << 6
KEY_COUNT_MASK   = 0x1F

DIR_OUTPUT       = 0
DIR_INPUT        = 1

PUD_DOWN         = 0
PUD_UP           = 1


class I2CPuppet:
    def __init__(self, vid=0x1209, pid=0xB182):
        self._buffer = bytearray(2)
        self._dev = usb.core.find(idVendor=vid, idProduct=pid)

        if self._dev is None:
            raise Exception('Device with vid:pid %04X:%04X not found!' % (vid, pid))

        conf = self._dev.get_active_configuration()
        itf = usb.util.find_descriptor(conf, bInterfaceClass=usb.CLASS_VENDOR_SPEC)

        self._ep_out = usb.util.find_descriptor(itf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
        self._ep_in = usb.util.find_descriptor(itf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)

        if (self._ep_out is None) or (self._ep_in is None):
            raise Exception('Vendor IN or OUT endpoint not found!')

    @property
    def version(self):
        ver = self._read_register(_REG_VER)
        return (ver >> 4, ver & 0x0F)

    @property
    def status(self):
        return self._read_register(_REG_KEY)

    @property
    def backlight(self):
        return self._read_register(_REG_BKL) / 255

    @backlight.setter
    def backlight(self, value):
        self._write_register(_REG_BKL, int(255 * value))

    @property
    def address(self):
        return self._read_register(_REG_ADR)

    @address.setter
    def address(self, value):
        self._write_register(_REG_ADR, value)

    def _read_register(self, reg):
        self._buffer[0] = reg
        self._dev.write(self._ep_out, self._buffer[:1])

        return self._dev.read(self._ep_in, 1)[0]

    def _write_register(self, reg, value):
        self._buffer[0] = reg | _WRITE_MASK
        self._buffer[1] = value
        self._dev.write(self._ep_out, self._buffer)

    def _update_register_bit(self, reg, bit, value):

        reg_val = self._read_register(reg)
        old_val = reg_val

        if value:
            reg_val |= (1 << bit)
        else:
            reg_val &= ~(1 << bit)

        if reg_val != old_val:
            self._write_register(reg, reg_val)

    def _get_register_bit(self, reg, bit):
        return self._read_register(reg) & (1 << bit) != 0

    keyboard_backlight = backlight
