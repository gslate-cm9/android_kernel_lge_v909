/*
 * arch/arm/mach-tegra/board-star.c
 *
 * Copyright (c) 2010, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/memblock.h>
#include <linux/tegra_uart.h>
#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>

#include <linux/mfd/tps6586x.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm_backlight.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/spi-tegra.h>
#include <linux/nct1008.h>
#include <linux/i2c-gpio.h>
#include <linux/tegra_audio.h>
#include <linux/suspend.h>
#include <mach/hardware.h>
#include <asm/setup.h>
#include <mach/tegra_wm8994_pdata.h>

#if defined(CONFIG_STARTABLET_REBOOT_REASON)
#include "nv_data.h"
#endif

#include "board.h"
#include "clock.h"
#include "board-star.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "sleep.h"
#include "wakeups-t2.h"
#include "pm.h"

#if defined (CONFIG_MACH_STARTABLET)
#include "star_i2c_device_address.h"
#include "star_devices.h"
#endif

static __initdata struct tegra_clk_init_table star_clk_init_table[] = {
	/* name		parent		rate		enabled */
	//clock setting for spi1 and spi2 port
	{ "sbc1",	"pll_p",	96000000,	true},
	//{ "sbc2",	"pll_p",	96000000,	true}, //disables spi secondary port
	//clock setting for spi1 and spi2 port
	{ "blink",	"clk_32k",	32768,		true},
	{ "pll_p_out4",	"pll_p",	24000000,	true },
	{ "pwm",	"clk_32k",	32768,		false},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s2",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ NULL,		NULL,		0,		0},
};

int muic_path = 0;
int muic_status = 0x0B;

/**************************************************************************************
 *                                       I2C platform_data
 **************************************************************************************/
static struct tegra_i2c_platform_data star_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.slave_addr = 0x00FC,
	.scl_gpio		= {TEGRA_GPIO_PC4, 0},
	.sda_gpio		= {TEGRA_GPIO_PC5, 0},
	.arb_recovery = arb_lost_recovery,
};

static const struct tegra_pingroup_config i2c2_ddc = {
	.pingroup	= TEGRA_PINGROUP_DDC,
	.func		= TEGRA_MUX_I2C2,
};

static const struct tegra_pingroup_config i2c2_gen2 = {
	.pingroup	= TEGRA_PINGROUP_PTA,
	.func		= TEGRA_MUX_I2C2,
};

static struct tegra_i2c_platform_data star_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 2,
	.bus_clk_rate	= { 400000, 100000 },
//	.bus_clk_rate	= { 100000, 10000 },
	.bus_mux	= { &i2c2_gen2, &i2c2_ddc },
	.bus_mux_len	= { 1, 1 },
	.slave_addr = 0x00FC,
	.scl_gpio		= {0, TEGRA_GPIO_PT5},
	.sda_gpio		= {0, TEGRA_GPIO_PT6},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data star_i2c3_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.slave_addr = 0x00FC,
	.scl_gpio		= {TEGRA_GPIO_PBB2, 0},
	.sda_gpio		= {TEGRA_GPIO_PBB3, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data star_dvc_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
//	.bus_clk_rate	= { 400000, 0 },
	.is_dvc		= true,
	.scl_gpio		= {TEGRA_GPIO_PZ6, 0},
	.sda_gpio		= {TEGRA_GPIO_PZ7, 0},
	.arb_recovery = arb_lost_recovery,
};

#define STAR_GPIO_HP_DET	TEGRA_GPIO_PW3
#define STAR_GPIO_MIC_BIAS 	TEGRA_GPIO_PX6

static struct tegra_wm8994_platform_data star_audio_pdata = {
	.gpio_hp_det		= STAR_GPIO_HP_DET,
	.gpio_spk_orientation	= -1,
	.gpio_mic_bias		= STAR_GPIO_MIC_BIAS,
};

static struct platform_device star_audio_device = {
	.name	= "tegra-snd-wm8994",
	.id	= 0,
	.dev	= {
		.platform_data  = &star_audio_pdata,
	},
};

#define STAR_ECHO_SCL_PIN TEGRA_GPIO_PI5
#define STAR_ECHO_SDA_PIN TEGRA_GPIO_PH1

static struct i2c_gpio_platform_data i2c_gpio_data = {
	.sda_pin		= STAR_ECHO_SDA_PIN,
	.scl_pin		= STAR_ECHO_SCL_PIN,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 5, 	/* cloase to 100kHz */
	.timeout		= HZ/10,
};

static struct platform_device i2c_gpio_controller = {
	.name	= "i2c-gpio",
	.id	= 7,
	.dev	= {
	  .platform_data = &i2c_gpio_data,
	},
};

#define STAR_3DCAM_SCL_PIN TEGRA_GPIO_PH0
#define STAR_3DCAM_SDA_PIN TEGRA_GPIO_PG1

static struct i2c_gpio_platform_data stereocam_i2c_gpio_data = {
	.sda_pin		= STAR_3DCAM_SDA_PIN,
	.scl_pin		= STAR_3DCAM_SCL_PIN,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 2, 	/* cloase to 100kHz */
	.timeout		= 2000,
};

static struct platform_device stereocam_i2c_gpio_controller = {
	.name	= "i2c-gpio",
	.id	= 6,
	.dev	= {
	  .platform_data = &stereocam_i2c_gpio_data,
	},
};

static void mpuirq_init(void)
{
	int ret = 0;

	pr_info("*** MPU START *** mpuirq_init...\n");

#if	MPU_ACCEL_IRQ_GPIO
	/* ACCEL-IRQ assignment */
	tegra_gpio_enable(MPU_ACCEL_IRQ_GPIO);
	ret = gpio_request(MPU_ACCEL_IRQ_GPIO, MPU_ACCEL_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_ACCEL_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_ACCEL_IRQ_GPIO);
		return;
	}
#endif

	/* MPU-IRQ assignment */
	tegra_gpio_enable(MPU_GYRO_IRQ_GPIO);
	ret = gpio_request(MPU_GYRO_IRQ_GPIO, MPU_GYRO_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_GYRO_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_GYRO_IRQ_GPIO);
		return;
	}
	pr_info("*** MPU END *** mpuirq_init...\n");
}

// #4
static void star_i2c_init(void)
{
	hw_rev board_rev = get_hw_rev();
	int TOUCH_INT		= TEGRA_GPIO_PW0;
	int TOUCH_MAIN_PWR	= TEGRA_GPIO_PK4;
	int TOUCH_IO_PWR	= TEGRA_GPIO_PP3;
	int TOUCH_RESET		= TEGRA_GPIO_PZ3;

	if (board_rev == REV_C)
		TOUCH_IO_PWR	= TEGRA_GPIO_PK3;
	if (board_rev == REV_C || board_rev == REV_E) {
		TOUCH_INT	= TEGRA_GPIO_PB4;
		TOUCH_RESET	= TEGRA_GPIO_PD1;
	}

	if (board_rev < REV_G)
		pr_warn("Touchscreen data is probably off due to wrong"
			" orientation\n");

	gpio_request(TOUCH_INT, "touch_int_n");
	tegra_gpio_enable(TOUCH_INT);
	gpio_direction_input(TOUCH_INT);

	gpio_request(TOUCH_RESET, "touch_reset");
	tegra_gpio_enable(TOUCH_RESET);
	gpio_direction_output(TOUCH_RESET,0);

	gpio_request(TOUCH_MAIN_PWR, "touch_main_pwr");
	tegra_gpio_enable(TOUCH_MAIN_PWR);
	gpio_direction_output(TOUCH_MAIN_PWR,0);

	gpio_request(TOUCH_IO_PWR, "touch_io_pwr");
	tegra_gpio_enable(TOUCH_IO_PWR);
	gpio_direction_output(TOUCH_IO_PWR,0);

	mdelay(5);
	gpio_set_value(TOUCH_MAIN_PWR,1);
	mdelay(10);
	gpio_set_value(TOUCH_IO_PWR,1);
	gpio_set_value(TOUCH_RESET,1);
	mdelay(200);

	mpuirq_init();

	tegra_i2c_device1.dev.platform_data = &star_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &star_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &star_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &star_dvc_platform_data;

	platform_device_register(&tegra_i2c_device4);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device1);

	/* enable gpios and register gpio i2c controller */
	platform_device_register(&i2c_gpio_controller);
	tegra_gpio_enable(STAR_ECHO_SCL_PIN);
	tegra_gpio_enable(STAR_ECHO_SDA_PIN);

	if( board_rev>=REV_1_2) {
		platform_device_register(&stereocam_i2c_gpio_controller);
		tegra_gpio_enable(STAR_3DCAM_SCL_PIN);
		tegra_gpio_enable(STAR_3DCAM_SDA_PIN);
	}

	//register i2c devices attached to I2C1
	star_register_numbered_i2c_devices(0, star_i2c_bus1_devices_info,
				ARRAY_SIZE(star_i2c_bus1_devices_info));
	//register i2c devices attached to I2C2
	star_register_numbered_i2c_devices(1, star_i2c_bus2_devices_info,
				ARRAY_SIZE(star_i2c_bus2_devices_info));
	//register i2c devices attached to I2C3
	star_register_numbered_i2c_devices(3, star_i2c_bus3_devices_info,
				ARRAY_SIZE(star_i2c_bus3_devices_info));
	//register i2c devices attached to I2C4(PWR_I2C)
	star_register_numbered_i2c_devices(4, star_i2c_bus4_devices_info,
				ARRAY_SIZE(star_i2c_bus4_devices_info));

	//register i2c devices attached to STEREOCAM-I2C-GPIO
	if (board_rev >= REV_1_2)
		star_register_numbered_i2c_devices(6,
				star_i2c_stereo_camera_info,
				ARRAY_SIZE(star_i2c_stereo_camera_info));
	else
		star_register_numbered_i2c_devices(4,
				star_i2c_stereo_camera_info,
				ARRAY_SIZE(star_i2c_stereo_camera_info));

	//register i2c devices attached to I2C-GPIO (Echo canceller)
	if (board_rev >= REV_G)
		star_register_numbered_i2c_devices(7, star_i2c_bus7_echo_info,
					ARRAY_SIZE(star_i2c_bus7_echo_info));
	else
		star_register_numbered_i2c_devices(1, star_i2c_bus7_echo_info,
					ARRAY_SIZE(star_i2c_bus7_echo_info));
}

#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}

static struct gpio_keys_button star_keys[] = {
	[0] = GPIO_KEY(KEY_VOLUMEDOWN, PG3, 0),
	[1] = GPIO_KEY(KEY_VOLUMEUP, PG2, 0),
//	[2] = GPIO_KEY(KEY_POWER, PV2, 1),
};

#define PMC_WAKE_STATUS 0x14

#if 0 // sync with ventana, but donot use it. star has powerkey driver
static int star_wakeup_key(void)
{
	unsigned long status =
		readl(IO_ADDRESS(TEGRA_PMC_BASE) + PMC_WAKE_STATUS);

	return status & TEGRA_WAKE_GPIO_PV2 ? KEY_POWER : KEY_RESERVED;
}
#endif

static struct gpio_keys_platform_data star_keys_platform_data = {
	.buttons	= star_keys,
	.nbuttons	= ARRAY_SIZE(star_keys),
//	.wakeup_key	= star_wakeup_key,
};

struct platform_device star_keys_device = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &star_keys_platform_data,
	},
};

static struct resource star_bcm4329_rfkill_resources[] = {
	{
		.name   = "bcm4329_nreset_gpio",
		.start  = TEGRA_GPIO_PP0,
		.end    = TEGRA_GPIO_PP0,
		.flags  = IORESOURCE_IO,
	},
/*	{
		.name   = "bcm4329_nshutdown_gpio",
		.start  = TEGRA_GPIO_PQ2,
		.end    = TEGRA_GPIO_PQ2,
		.flags  = IORESOURCE_IO,
	},
*/
};

// resouce for previous hw rev ( <= 1.2)
static struct resource star_1_2_bcm4329_rfkill_resources[] = {
	{
		.name   = "bcm4329_nreset_gpio",
		.start  = TEGRA_GPIO_PQ2,
		.end    = TEGRA_GPIO_PQ2,
		.flags  = IORESOURCE_IO,
	},
/*	{
		.name   = "bcm4329_nshutdown_gpio",
		.start  = TEGRA_GPIO_PQ2,
		.end    = TEGRA_GPIO_PQ2,
		.flags  = IORESOURCE_IO,
	},
*/
};

static struct platform_device star_bcm4329_rfkill_device = {
	.name = "bcm4329_rfkill",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(star_bcm4329_rfkill_resources),
	.resource       = star_bcm4329_rfkill_resources,
};

static noinline void __init star_bt_rfkill(void)
{
        if (get_hw_rev() <= REV_1_2)
        {
            star_bcm4329_rfkill_device.num_resources = ARRAY_SIZE(star_1_2_bcm4329_rfkill_resources);
            star_bcm4329_rfkill_device.resource = star_1_2_bcm4329_rfkill_resources;
        }

	/*Add Clock Resource*/
	clk_add_alias("bcm4329_32k_clk", star_bcm4329_rfkill_device.name, \
		      "blink", NULL);

        platform_device_register(&star_bcm4329_rfkill_device);

        return;
}

#define STAR_GPIO_BT_WAKE	TEGRA_GPIO_PX4
#define STAR_GPIO_BT_HOST_WAKE	TEGRA_GPIO_PC7

static struct resource star_bluesleep_resources[] = {
	[0] = {
		.name	= "gpio_host_wake",
		.start	= STAR_GPIO_BT_HOST_WAKE,
		.end	= STAR_GPIO_BT_HOST_WAKE,
		.flags	= IORESOURCE_IO,
	},
	[1] = {
		.name	= "gpio_ext_wake",
		.start	= STAR_GPIO_BT_WAKE,
		.end	= STAR_GPIO_BT_WAKE,
		.flags	= IORESOURCE_IO,
	},
	[2] = {
		.name = "host_wake",
		.start  = TEGRA_GPIO_TO_IRQ(STAR_GPIO_BT_HOST_WAKE),
		.end    = TEGRA_GPIO_TO_IRQ(STAR_GPIO_BT_HOST_WAKE),
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct platform_device star_bluesleep_device = {
	.name           = "bluesleep",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(star_bluesleep_resources),
	.resource       = star_bluesleep_resources,
};

static void __init star_setup_bluesleep(void)
{
	platform_device_register(&star_bluesleep_device);
	tegra_gpio_enable(STAR_GPIO_BT_WAKE);
	tegra_gpio_enable(STAR_GPIO_BT_HOST_WAKE);
	return;
}

#ifdef CONFIG_STARTABLET_GPS_BCM4751
//LGE_UPDATE_S jayeong.im@lge.com 2010-11-30 star_gps_init
static int __init star_gps_init(void)
{
/* Always turn on 32k clock
             struct clk *clk32 = clk_get_sys(NULL, "blink");
             if (!IS_ERR(clk32)) {
                           clk_set_rate(clk32,clk32->parent->rate);
                           clk_enable(clk32);
             }
*/
             return 0;
}
//LGE_UPDATE_E jayeong.im@lge.com 2010-11-30 star_gps_init
#endif

static struct platform_device *star_devices[] __initdata = {
	&star_powerkey,
	&tegra_gps_gpio,
	&tegra_misc,
	&tegra_wdt_device,
	&tegra_gart_device,
	&tegra_aes_device,
	&star_keys_device,
	&tegra_leds, //2010.08.11 ch.han@lge.com for leds drives
	&star_headset_device,
	//&tegra_rtc_device,
	&tegra_camera,
/* standard tegra audio devices */
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&tegra_spdif_device,
	&tegra_das_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tegra_pcm_device,
/* end of standard tegra audio devices */
	&wm8994_fixed_voltage0,
	&wm8994_fixed_voltage1,
	&tegra_avp_device,
	&tegra_displaytest,
	&tegra_camera_flash, //2010.12.08 hyungmoo.huh@lge.com for camera flash LED
};

static int star_audio_init(void)
{
	int orientation_gpio = TEGRA_GPIO_PK5;
	// set speak change gpio
	if (get_hw_rev() < REV_F)
		orientation_gpio = TEGRA_GPIO_PV7;

	star_audio_pdata.gpio_spk_orientation = orientation_gpio;

	tegra_gpio_enable(STAR_GPIO_HP_DET);
	gpio_request(STAR_GPIO_MIC_BIAS, "hs_mic_bias");
	tegra_gpio_enable(STAR_GPIO_MIC_BIAS);
	gpio_direction_output(STAR_GPIO_MIC_BIAS, 0);

	return platform_device_register(&star_audio_device);
}


static void star_keys_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(star_keys); i++)
		tegra_gpio_enable(star_keys[i].gpio);
}


static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg	= true,
	.has_hostpc	= false,
	.phy_intf	= TEGRA_USB_PHY_INTF_UTMI,
	.op_mode	= TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev	= {
		.vbus_pmu_irq			= 0,
		.vbus_gpio			= -1,
		.charging_supported		= false,
		.remote_wakeup_supported	= false,
	},
	.u_cfg.utmi	= {
		.hssync_start_delay	= 0,
		.elastic_limit		= 16,
		.idle_wait_delay	= 17,
		.term_range_adj		= 6,
		.xcvr_setup		= 15,
		.xcvr_lsfslew		= 2,
		.xcvr_lsrslew		= 2,
		.xcvr_setup_offset	= 0,
		.xcvr_use_fuses		= 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg	= true,
	.has_hostpc	= false,
	.phy_intf	= TEGRA_USB_PHY_INTF_UTMI,
	.op_mode	= TEGRA_USB_OPMODE_HOST,
	.u_data.host	= {
		.vbus_gpio			= TEGRA_GPIO_PP1,
		.vbus_reg			= NULL,
		.hot_plug			= true,
		.remote_wakeup_supported	= false,
		.power_off_on_suspend		= false,
	},
	.u_cfg.utmi	= {
		.hssync_start_delay	= 0,
		.elastic_limit		= 16,
		.idle_wait_delay	= 17,
		.term_range_adj		= 6,
		.xcvr_setup		= 15,
		.xcvr_lsfslew		= 2,
		.xcvr_lsrslew		= 2,
	},
};

#ifdef CONFIG_USB_TEGRA_OTG
static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device	= &tegra_ehci1_device,
	.ehci_pdata	= &tegra_ehci1_utmi_pdata,
};
#endif

static void star_usb_init(void)
{
	if (get_hw_rev() < REV_G)
		tegra_otg_pdata.ehci_pdata->u_data.host.vbus_gpio = TEGRA_GPIO_PH0;

	/* OTG should be the first to be registered */
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;
	platform_device_register(&tegra_udc_device);
}

static int __init star_muic_path_setup(char *line)
{
	if (sscanf(line, "%1d:%02x", &muic_path, &muic_status) != 2) {
		muic_path = 0;
		muic_status = 0x0B;
	}
	return 1;
}

__setup("muic_path=", star_muic_path_setup);

extern char boot_command_line[COMMAND_LINE_SIZE];

static void star_nct1008_init(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PK2);
	gpio_request(TEGRA_GPIO_PK2, "temp_alert");
	gpio_direction_input(TEGRA_GPIO_PK2);
}


/*
 *   Startablet SPI devices
 */
static struct platform_device *star_spi_devices[] __initdata = {
	&tegra_spi_slave_device1,
};

static struct spi_board_info star_spi_board_devices[] __initdata = {
#ifdef CONFIG_SPI_MDM6600
	{
		.modalias = "mdm6600",
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_1,
		.max_speed_hz = 24000000,
		.controller_data = &tegra_spi_slave_device1,
		.irq = 0,
		//	.platform_data = &mdm6600
	},
#else /* CONFIG_SPI_MDM6600 */
{
		.modalias = "ifxn721",
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_1,
		.max_speed_hz = 24000000,
		//.controller_data	= &tegra_spi_slave_device1,
		.irq = 277,//0,//GPIO_IRQ(TEGRA_GPIO_PO5),
		//	.platform_data = &ifxn721
	},
#endif /* CONFIG_SPI_MDM6600 */
};

struct spi_clk_parent spi_parent_clk[] = {
	[0] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[1] = {.name = "pll_m"},
	[2] = {.name = "clk_m"},
#else
	[1] = {.name = "clk_m"},
#endif
};

static struct tegra_spi_platform_data star_spi_pdata = {
	.is_dma_based		= true,
	.max_dma_buffer		= (16 * 1024),
	.is_clkon_always	= false,
	.max_rate		= 24000000,
};

static void __init star_spi_init(void)
{
	int i;
	struct clk *c;
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	for (i = 0; i < ARRAY_SIZE(spi_parent_clk); ++i) {
		c = tegra_get_clock_by_name(spi_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						spi_parent_clk[i].name);
			continue;
		}
		spi_parent_clk[i].parent_clk = c;
		spi_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	star_spi_pdata.parent_clk_list = spi_parent_clk;
	star_spi_pdata.parent_clk_count = ARRAY_SIZE(spi_parent_clk);

	tegra_spi_slave_device1.dev.platform_data = &star_spi_pdata;

	platform_add_devices(star_spi_devices, ARRAY_SIZE(star_spi_devices));

	spi_register_board_info(star_spi_board_devices,
				ARRAY_SIZE(star_spi_board_devices));
}

static struct platform_device *star_uart_devices[] __initdata = {
	&tegra_uartb_device,
	&tegra_uartc_device,
#ifdef CONFIG_STARTABLET_GPS_BCM4751
	&tegra_uartd_device, //jayeong.im@lge.com 2010-11-30 star_gps_init
#endif
};

static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "pll_p"},
	[1] = {.name = "pll_m"},
#ifdef CONFIG_STARTABLET_GPS_BCM4751
	[2] = {.name = "clk_m"},
#endif
};

static struct tegra_uart_platform_data star_uart_pdata;

static void __init uart_debug_init(void)
{
	unsigned long rate;
	struct clk *c;

	/* UARTB is the debug port. */
	pr_info("Selecting UARTB as the debug console\n");
	star_uart_devices[0] = &debug_uartd_device;
	debug_uart_port_base = ((struct plat_serial8250_port *)(
			debug_uartb_device.dev.platform_data))->mapbase;
	debug_uart_clk = clk_get_sys("serial8250.0", "uartb");

	/* Clock enable for the debug channel */
	if (!IS_ERR_OR_NULL(debug_uart_clk)) {
		rate = ((struct plat_serial8250_port *)(
			debug_uartb_device.dev.platform_data))->uartclk;
		pr_info("The debug console clock name is %s\n",
						debug_uart_clk->name);
		c = tegra_get_clock_by_name("pll_p");
		if (IS_ERR_OR_NULL(c))
			pr_err("Not getting the parent clock pll_p\n");
		else
			clk_set_parent(debug_uart_clk, c);

		clk_enable(debug_uart_clk);
		clk_set_rate(debug_uart_clk, rate);
	} else {
		pr_err("Not getting the clock %s for debug console\n",
					debug_uart_clk->name);
	}
}

static void __init star_uart_init(void)
{
	int i;
	struct clk *c;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	star_uart_pdata.parent_clk_list = uart_parent_clk;
	star_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	tegra_uartb_device.dev.platform_data = &star_uart_pdata;
	tegra_uartc_device.dev.platform_data = &star_uart_pdata;
#ifdef CONFIG_STARTABLET_GPS_BCM4751
	&tegra_uartd_device.dev.platform_data = &star_uart_pdata;
#endif

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs() &&
	    (strstr(boot_command_line, "ttyS0") != NULL))
		uart_debug_init();

	platform_add_devices(star_uart_devices,
				ARRAY_SIZE(star_uart_devices));
}

#if defined(CONFIG_STARTABLET_REBOOT_REASON)
static void startablet_pm_restart(char mode, const char *cmd)
{
	if (cmd && write_rr_into_nv(cmd))
		pr_err("%s: Fail to write reboot reason!!!%d-%s\n",
			 __func__, mode, cmd);

	arm_machine_restart(mode, cmd);
}

static void __init tegra_star_init(void)
{
	arm_pm_restart = startablet_pm_restart;
#else
static void __init tegra_star_init(void)
{
#endif
	tegra_clk_init_from_table(star_clk_init_table);
	star_pinmux_init();

	star_i2c_init();
	star_regulator_init();

	star_spi_init();

	star_uart_init();

	platform_add_devices(star_devices, ARRAY_SIZE(star_devices));

	tegra_ram_console_debug_init();

	star_audio_init();

	// star_i2c_init();
	star_sdhci_init();
	// star_regulator_init();
	star_keys_init();

	star_usb_init();
	star_panel_init();
	star_bt_rfkill();
	save_hw_rev();
#ifdef CONFIG_STARTABLET_GPS_BCM4751
	printk("star_gps_init BCM4751 \n") ;
	star_gps_init(); // jayeong.im@lge.com 2010-11-30 star_gps_init
#endif
	star_emc_init();
	star_nct1008_init();//Thermal IC enable

	star_setup_bluesleep();

	tegra_release_bootloader_fb();
}

int __init tegra_star_protected_aperture_init(void)
{
    tegra_protected_aperture_init(tegra_grhost_aperture);
    return 0;
}
late_initcall(tegra_star_protected_aperture_init);

void __init tegra_star_reserve(void)
{
    //long ret;
    if (memblock_reserve(0x0, 4096) < 0)
            pr_warn("Cannot reserve first 4K of memory for safety\n");

    tegra_reserve(SZ_256M, SZ_8M + SZ_1M, SZ_16M);
    tegra_ram_console_debug_reserve(SZ_1M);
}

MACHINE_START(STARTABLET, "startablet")
      .boot_params  = 0x00000100,
      .init_early     = tegra_init_early,
      .init_irq       = tegra_init_irq,
      .init_machine   = tegra_star_init,
      .map_io         = tegra_map_common_io,
      .reserve        = tegra_star_reserve,
      .timer          = &tegra_timer,
MACHINE_END
