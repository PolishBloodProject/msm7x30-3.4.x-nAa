/* arch/arm/mach-msm/keypad-zeus.c
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright 2013 Michael Bestas (mikeioannina@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/gpio_event.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>

#include "keypad-zeus.h"

#ifdef CONFIG_INPUT_JOYSTICK
static struct gpio_event_input_info keypad_gpio_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_KEY,
	.keymap = keypad_zeus_gpio_map,
	.keymap_size = ARRAY_SIZE(keypad_zeus_gpio_map),
	.debounce_time.tv64 = 10 * NSEC_PER_MSEC,
};
#endif

static struct gpio_event_input_info keypad_pmic_gpio_nwake_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_KEY,
	.keymap = keypad_zeus_pmic_gpio_map_nwake,
	.keymap_size = ARRAY_SIZE(keypad_zeus_pmic_gpio_map_nwake),
	.debounce_time.tv64 = 10 * NSEC_PER_MSEC,
};

static struct gpio_event_input_info keypad_pmic_gpio_wake_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_KEY,
	.keymap = keypad_zeus_pmic_gpio_map_wake,
	.keymap_size = ARRAY_SIZE(keypad_zeus_pmic_gpio_map_wake),
	.debounce_time.tv64 = 10 * NSEC_PER_MSEC,
	.info.no_suspend = true,
};

#ifdef CONFIG_INPUT_JOYSTICK
static struct gpio_event_input_info switch_gpio_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_SW,
	.keymap = switch_zeus_gpio_map,
	.keymap_size = ARRAY_SIZE(switch_zeus_gpio_map),
	.info.no_suspend = true,
};

static struct gpio_event_input_info lidswitch_gpio_info = {
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_ACTIVE_HIGH,
	.type = EV_MSC,
	.keymap = lidswitch_zeus_gpio_map,
	.keymap_size = ARRAY_SIZE(lidswitch_zeus_gpio_map),
	.info.no_suspend = true,
};
#endif

static struct gpio_event_info *keypad_info[] = {
	&keypad_pmic_gpio_wake_info.info,
	&keypad_pmic_gpio_nwake_info.info,
#ifdef CONFIG_INPUT_JOYSTICK
	&switch_gpio_info.info,
	&lidswitch_gpio_info.info,
	&keypad_gpio_info.info,
#endif
};

static int keypad_pmic_gpio_config(int gpio)
{
	int rc;
	struct pm_gpio pm8058_gpio = {
		.direction    = PM_GPIO_DIR_IN,
		.pull         = PM_GPIO_PULL_UP_31P5,
		.vin_sel      = 2,
		.function     = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol  = 0,
		.out_strength = PM_GPIO_STRENGTH_LOW,
	};

	rc = pm8xxx_gpio_config(gpio, &pm8058_gpio);
	if (rc) {
		pr_err("%s: pm8xxx_gpio_config(%d) failed, rc = %d.\n",
				__func__, gpio, rc);
		return rc;
	}

	return 0;
}

static int keypad_power(const struct gpio_event_platform_data *pdata, bool on)
{
#ifdef CONFIG_INPUT_JOYSTICK
	int i;
	int gpio;
	int mgp;
	int rc;

	if (!on) {
		for (i = 0; i < ARRAY_SIZE(keypad_zeus_gpio_map); ++i) {
			gpio = keypad_zeus_gpio_map[i].gpio;

			mgp = GPIO_CFG(gpio,
				       0,
				       GPIO_CFG_INPUT,
				       GPIO_CFG_PULL_DOWN,
				       GPIO_CFG_2MA);
			rc = gpio_tlmm_config(mgp, GPIO_CFG_DISABLE);
			if (rc)
				pr_err("%s: gpio_tlmm_config (gpio=%d), failed\n",
				       __func__, keypad_zeus_gpio_map[i].gpio);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(keypad_zeus_gpio_map); ++i) {
			gpio = keypad_zeus_gpio_map[i].gpio;
			mgp = GPIO_CFG(gpio,
				       0,
				       GPIO_CFG_INPUT,
				       GPIO_CFG_PULL_UP,
				       GPIO_CFG_2MA);
			rc = gpio_tlmm_config(mgp, GPIO_CFG_ENABLE);
			if (rc)
				pr_err("%s: gpio_tlmm_config (gpio=%d), failed\n",
				       __func__, keypad_zeus_gpio_map[i].gpio);
		}
	}
#endif
	return 0;
}

static struct gpio_event_platform_data keypad_data = {
	.name		= "keypad-zeus",
	.info		= keypad_info,
	.info_count	= ARRAY_SIZE(keypad_info),
	.power          = keypad_power,
};

struct platform_device keypad_device_zeus = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &keypad_data,
	},
};

static int __init keypad_device_init(void)
{
	int i;
#ifdef CONFIG_INPUT_JOYSTICK
	int rc;
	int gpio;
	int mgp;
#endif

	keypad_power(&keypad_data,1);

#ifdef CONFIG_INPUT_JOYSTICK
	for (i = 0; i < ARRAY_SIZE(switch_zeus_gpio_map); ++i) {
		gpio = switch_zeus_gpio_map[i].gpio;
		mgp = GPIO_CFG(gpio,
			       0,
			       GPIO_CFG_INPUT,
			       GPIO_CFG_NO_PULL,
			       GPIO_CFG_2MA);
		rc = gpio_tlmm_config(mgp, GPIO_CFG_ENABLE);
		if (rc)
			pr_err("%s: gpio_tlmm_config (gpio=%d), failed\n",
					__func__, switch_zeus_gpio_map[i].gpio);
	}
#endif

	for (i = 0; i < ARRAY_SIZE(keypad_zeus_pmic_gpio_map_nwake); ++i) {
		keypad_pmic_gpio_config(
				PM8058_GPIO_SYS_TO_PM(
				keypad_zeus_pmic_gpio_map_nwake[i].gpio));
	}

	for (i = 0; i < ARRAY_SIZE(keypad_zeus_pmic_gpio_map_wake); ++i) {
		keypad_pmic_gpio_config(
				PM8058_GPIO_SYS_TO_PM(
				keypad_zeus_pmic_gpio_map_wake[i].gpio));
	}

	return platform_device_register(&keypad_device_zeus);
}

static void __exit keypad_device_exit(void)
{
	platform_device_unregister(&keypad_device_zeus);
}

module_init(keypad_device_init);
module_exit(keypad_device_exit);
