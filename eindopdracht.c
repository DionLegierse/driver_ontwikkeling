#pragma GCC optimize("O0")

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h> // struct of_device_id
#include <linux/of.h>              // of_match_ptr macro
#include <linux/ioport.h>
#include <linux/device/driver.h> // struct resource
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");

/***********************************************************/
/************************ DEFINES **************************/
/***********************************************************/
#define COMMAND ((char)0x00)
#define DATA ((char)0x40)

#define PUMP_COMMAND ((char)0x8D)
#define ENABLE_SCREEN_COMMAND ((char)0xAE)
#define SET_PAGE_COMMAND ((char)0xB0)

#define PUMP_SETTING ((char)0x14)

#define CHARACTER_BYTES ((size_t)5)
#define CHARACTER_SPACE ((size_t)6)

/***********************************************************/
/************************* TYPES ***************************/
/***********************************************************/

typedef unsigned int uin32_t;

/***********************************************************/
/****************** FUNCTION PROTOTYPES ********************/
/***********************************************************/

static void initialize_screen(struct i2c_client *);

static int lcd_driver_init(void);
static void lcd_driver_exit(void);

static int lcd_driver_probe(struct i2c_client *, const struct i2c_device_id *);
static int lcd_driver_remove(struct i2c_client *);

static ssize_t show_enable_lcd(struct device_driver *, char *);
static ssize_t store_enable_lcd(struct device_driver *, const char *, size_t);

static ssize_t store_display_lcd(struct device_driver *, const char *, size_t);

/***********************************************************/
/******************** GLOBAL VARIABLES *********************/
/***********************************************************/
static char lcd_display_state = 0;

static struct i2c_client *lcd_i2c_client = NULL;

static const struct i2c_device_id i2c_ids[] = {
    {"lcd-driver", 0},
    {} // ends with empty; MUST be last member
};

static const struct of_device_id ids[] = {
    {
        .compatible = "lcd-driver",
    },
    {} // ends with empty; MUST be last member
};

MODULE_DEVICE_TABLE(i2c, i2c_ids);

static struct i2c_driver i2c_driver = {
    .probe = lcd_driver_probe,   // obliged
    .remove = lcd_driver_remove, // obliged
    //  .shutdown            // optional
    //  .suspend             // optional
    //  .suspend_late        // optional
    //  .resume_early        // optional
    //  .resume              // optional
    .id_table = i2c_ids,
    .driver = {
        .name = "lcd-driver", // name of the driver
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(ids),
    },
};

struct driver_attribute display_attribute = {
    .show = NULL,
    .store = store_display_lcd /*store_display_lcd*/,
    .attr = {
        .name = "display",
        .mode = 00222}};

struct driver_attribute brightness_attribute = {
    .show = NULL,
    .store = NULL,
    .attr = {
        .name = "brightness",
        .mode = 00666}};

struct driver_attribute enable_attribute = {
    .show = show_enable_lcd,
    .store = store_enable_lcd,
    .attr = {
        .name = "enable",
        .mode = 00666}};

static struct i2c_board_info lcd_i2c_board_info = {
    I2C_BOARD_INFO("lcd-driver", 0x3C)};

const char characters[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // space
    0x00, 0x00, 0x2f, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7f, 0x14, 0x7f, 0x14, // #
    0x24, 0x2a, 0x7f, 0x2a, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1c, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1c, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x00, 0xA0, 0x60, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x59, 0x51, 0x3E, // @
    0x7C, 0x12, 0x11, 0x12, 0x7C, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x55, 0xAA, 0x55, 0xAA, 0x55, // Backslash (Checker pattern)
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x03, 0x05, 0x00, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x18, 0xA4, 0xA4, 0xA4, 0x7C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x40, 0x80, 0x84, 0x7D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0xFC, 0x24, 0x24, 0x24, 0x18, // p
    0x18, 0x24, 0x24, 0x18, 0xFC, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x1C, 0xA0, 0xA0, 0xA0, 0x7C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x10, 0x7C, 0x82, 0x00, // {
    0x00, 0x00, 0xFF, 0x00, 0x00, // |
    0x00, 0x82, 0x7C, 0x10, 0x00, // }
    0x00, 0x06, 0x09, 0x09, 0x06, // ~ (Degrees)
};

/***********************************************************/
/*********************** DRIVER INIT ***********************/
/***********************************************************/

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

/***********************************************************/
/****************** FUNCTION DEFINITIONS *******************/
/***********************************************************/

#pragma region helpers
static void initialize_screen(struct i2c_client *client)
{
    char charge_pump_enable[] = {COMMAND, PUMP_COMMAND, PUMP_SETTING};
    char set_page[] = {COMMAND, (SET_PAGE_COMMAND + 4)};

    i2c_master_send(lcd_i2c_client, charge_pump_enable, sizeof(charge_pump_enable));
    i2c_master_send(lcd_i2c_client, set_page, sizeof(set_page));
}
#pragma endregion

#pragma region driver_init
static int lcd_driver_init(void)
{
    printk(KERN_ALERT "eindopracht init");

    struct i2c_adapter *lcd_i2c_adapter = i2c_get_adapter(2);
    i2c_new_client_device(lcd_i2c_adapter, &lcd_i2c_board_info);

    i2c_add_driver(&i2c_driver);

    return 0;
}

static void lcd_driver_exit(void)
{
    printk(KERN_ALERT "eindopdracht exit");

    i2c_del_driver(&i2c_driver);
    i2c_unregister_device(lcd_i2c_client);
}
#pragma endregion

#pragma region platform_driver_init
static int lcd_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    lcd_i2c_client = client;

    initialize_screen(lcd_i2c_client);

    printk(KERN_ALERT "eindopdracht inserting attributes");
    driver_create_file(&(i2c_driver.driver), &display_attribute);
    driver_create_file(&(i2c_driver.driver), &brightness_attribute);
    driver_create_file(&(i2c_driver.driver), &enable_attribute);

    return 0;
}

static int lcd_driver_remove(struct i2c_client *client)
{
    printk(KERN_ALERT "eindopracht removing attributes");
    driver_remove_file(&(i2c_driver.driver), &display_attribute);
    driver_remove_file(&(i2c_driver.driver), &brightness_attribute);
    driver_remove_file(&(i2c_driver.driver), &enable_attribute);
    return 0;
}
#pragma endregion

#pragma region enable_lcd
static ssize_t show_enable_lcd(struct device_driver *device, char *buffer)
{
    return sprintf(buffer, "%u\n", lcd_display_state);
}

static ssize_t store_enable_lcd(struct device_driver *device, const char *buffer, size_t size)
{
    char send_buffer[2] = {COMMAND, ENABLE_SCREEN_COMMAND};
    ssize_t result = -1;

    if (buffer[0] == '0')
    {
        lcd_display_state = 0;
    }
    else if (buffer[0] == '1')
    {
        lcd_display_state = 1;
    }
    else
    {
        return -1;
    }

    send_buffer[1] |= lcd_display_state;

    result = i2c_master_send(lcd_i2c_client, send_buffer, sizeof(send_buffer));

    return size;
}
#pragma endregion

#pragma region lcd_display

static ssize_t store_display_lcd(struct device_driver *device, const char *buffer, size_t size)
{
    int i;
    char current_char;
    size_t character_offset;

    for (i = 0; i < size; i++)
    {
        current_char = buffer[i];
        if (current_char >= ' ' && current_char <= '~')
        {
            character_offset = (current_char - ' ') * CHARACTER_BYTES;
            i2c_master_send(lcd_i2c_client, characters + character_offset, CHARACTER_BYTES);
        }
    }
    return size;
}

#pragma endregion