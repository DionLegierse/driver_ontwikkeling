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
#define STREAM ((char)0x80)

#define PUMP_COMMAND ((char)0x8D)
#define ADDRESS_MODE_COMMAND ((char)0x20)
#define ENABLE_SCREEN_COMMAND ((char)0xAE)

#define PUMP_SETTING ((char)0x14)
#define ADDRESS_MODE_SETTING ((char)0x02)

#define SCREEN_WIDTH ((size_t)128)
#define SCREEN_HEIGHT ((size_t)32)
#define BUFFER_SIZE (((SCREEN_WIDTH * SCREEN_HEIGHT) / 8) + 1)

/***********************************************************/
/************************* TYPES ***************************/
/***********************************************************/

typedef unsigned int uin32_t;

/***********************************************************/
/****************** FUNCTION PROTOTYPES ********************/
/***********************************************************/

static void initialize_screen(struct i2c_client *);
static void write_buffer_to_screen(struct i2c_client *);

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

static unsigned char screen_buffer[BUFFER_SIZE];

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
    .store = NULL /*store_display_lcd*/,
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

/***********************************************************/
/*********************** DRIVER INIT ***********************/
/***********************************************************/

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

/***********************************************************/
/****************** FUNCTION DEFINITIONS *******************/
/***********************************************************/

#pragma region utilities
static void initialize_screen(struct i2c_client *client)
{
    char charge_pump_enable[] = {COMMAND, PUMP_COMMAND, PUMP_SETTING};
    char address_mode[] = {COMMAND, ADDRESS_MODE_COMMAND, ADDRESS_MODE_SETTING};

    i2c_master_send(lcd_i2c_client, charge_pump_enable, sizeof(charge_pump_enable));
    i2c_master_send(lcd_i2c_client, address_mode, sizeof(address_mode));
}

static void write_buffer_to_screen(struct i2c_client *client)
{
    screen_buffer[0] = STREAM | DATA;
    i2c_master_send(lcd_i2c_client, screen_buffer, sizeof(screen_buffer));
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
    char option = buffer[0];
    char send_buffer[2] = {COMMAND, ENABLE_SCREEN_COMMAND};
    const char enable_address = 0xAE;
    ssize_t result = -1;

    if (option == '0')
    {
        lcd_display_state = 0;
    }
    else if (option == '1')
    {
        lcd_display_state = 1;
    }
    else
    {
        return -1;
    }

    send_buffer[2] |= lcd_display_state;

    if (send_buffer[2] != 0)
    {
        result = i2c_master_send(lcd_i2c_client, send_buffer, 2);
    }

    return size;
}
#pragma endregion

#pragma region lcd_display

static ssize_t store_display_lcd(struct device_driver *device, const char *buffer, size_t size)
{
    int i;

    for (i = 1; i < BUFFER_SIZE; i++)
    {
        screen_buffer[i] = 0xFF;
    }

    write_buffer_to_screen(lcd_i2c_client);
    return -1;
}

#pragma endregion