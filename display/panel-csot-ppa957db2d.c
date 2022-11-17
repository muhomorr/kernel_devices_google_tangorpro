// SPDX-License-Identifier: GPL-2.0-only
/*
 * MIPI-DSI based CSOT PPA957DB2-D LCD panel driver.
 *
 * Copyright (c) 2022 Google Inc.
 *
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <video/mipi_display.h>

#include "samsung/panel/panel-samsung-drv.h"

#define PPA957DB2D_WRCTRLD_DD_BIT	0x08
#define PPA957DB2D_WRCTRLD_BL_BIT	0x04
#define PPA957DB2D_WRCTRLD_BCTRL_BIT	0x20
#define PPA957DB2D_PANEL_ID_REG		0x00
#define PPA957DB2D_PANEL_ID_LEN		37

#if PPA957DB2D_PANEL_ID_LEN >= PANEL_ID_MAX
	#error PANEL_ID_MAX should be greater than PPA957DB2D_PANEL_ID_LEN
#endif

static const u32 ppa957db2d_panel_rev[] = {
	PANEL_REV_PROTO1,
	PANEL_REV_PROTO2,
	PANEL_REV_EVT1,
	PANEL_REV_EVT1_1,
	PANEL_REV_EVT2,
	PANEL_REV_DVT1,
	PANEL_REV_PVT,
};

static const struct exynos_dsi_cmd ppa957db2d_init_cmds[] = {
	/* CMD2, Page3 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x23),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* 12 bits PWM */
	EXYNOS_DSI_CMD_SEQ(0x00, 0x80),
	/* PWM freq 3kHz */
	EXYNOS_DSI_CMD_SEQ(0x08, 0x04),

	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x05),
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x20),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x18, 0x40),
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x02),
	EXYNOS_DSI_CMD_SEQ(0xFF, 0xF0),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x3A, 0x08),

	/* CMD2, Page7 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x27),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),

	/* Error flag detection */
	EXYNOS_DSI_CMD_SEQ(0xD0, 0x31),
	EXYNOS_DSI_CMD_SEQ(0xD1, 0x84),
	EXYNOS_DSI_CMD_SEQ(0xD2, 0x30),
	EXYNOS_DSI_CMD_SEQ(0xDE, 0x03),
	EXYNOS_DSI_CMD_SEQ(0xDF, 0x02),

	/* CMD2, Page 6 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x26),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),

	/* Reduce OSC drift */
	EXYNOS_DSI_CMD_SEQ(0x00, 0x81),
	EXYNOS_DSI_CMD_SEQ(0x01, 0xB0),

	/* CMD2, Page 2 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x22),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),

	/* Reduce OSC drift */
	EXYNOS_DSI_CMD_SEQ(0x9F, 0x50),
	EXYNOS_DSI_CMD_SEQ(0xA0, 0x50),
	EXYNOS_DSI_CMD_SEQ(0xA5, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xA6, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xA7, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xA9, 0x50),
	EXYNOS_DSI_CMD_SEQ(0xAA, 0x20),
	EXYNOS_DSI_CMD_SEQ(0xAB, 0x20),
	EXYNOS_DSI_CMD_SEQ(0xAD, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xB0, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xB1, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xB2, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xB3, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xB8, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x84),
	EXYNOS_DSI_CMD_SEQ(0xBA, 0x84),
	EXYNOS_DSI_CMD_SEQ(0xBB, 0x84),
	EXYNOS_DSI_CMD_SEQ(0xB4, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xB5, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0xBE, 0x05),
	EXYNOS_DSI_CMD_SEQ(0xBF, 0x84),
	EXYNOS_DSI_CMD_SEQ(0xC5, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xC6, 0x6A),
	EXYNOS_DSI_CMD_SEQ(0xC7, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xCA, 0x08),
	EXYNOS_DSI_CMD_SEQ(0xCB, 0x6A),
	EXYNOS_DSI_CMD_SEQ(0xCE, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xCF, 0x08),
	EXYNOS_DSI_CMD_SEQ(0xD0, 0x6A),
	EXYNOS_DSI_CMD_SEQ(0xD3, 0x08),
	EXYNOS_DSI_CMD_SEQ(0xD4, 0x6A),
	EXYNOS_DSI_CMD_SEQ(0xD7, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xDC, 0x08),
	EXYNOS_DSI_CMD_SEQ(0xDD, 0x6A),
	EXYNOS_DSI_CMD_SEQ(0x6F, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x70, 0x11),
	EXYNOS_DSI_CMD_SEQ(0x73, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x74, 0x85),
	EXYNOS_DSI_CMD_SEQ(0xC0, 0x05),
	EXYNOS_DSI_CMD_SEQ(0xC1, 0x94),
	EXYNOS_DSI_CMD_SEQ(0xC2, 0x00),

	/* CMD2, Page A */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x2A),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),

	/* Reduce OSC drift */
	EXYNOS_DSI_CMD_SEQ(0x9A, 0x02),

	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Write Primary & Secondary */
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x02),
	EXYNOS_DSI_CMD_SEQ(0x51, 0x0F, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0x53, 0x24),
	/* CABC initial OFF */
	EXYNOS_DSI_CMD_SEQ(0x55, 0x00),

	/* CMD2, Page2 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x22),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Set IE parameter */
	EXYNOS_DSI_CMD_SEQ(0x1A, 0x00),
	EXYNOS_DSI_CMD_SEQ(0x68, 0x00),
	EXYNOS_DSI_CMD_SEQ(0xA2, 0x20),
	EXYNOS_DSI_CMD_SEQ(0x56, 0x77),
	/* Set IE dark fine tune parameter */
	EXYNOS_DSI_CMD_SEQ(0x58, 0x10),
	/* Set IE bright fine tune parameter */
	EXYNOS_DSI_CMD_SEQ(0x59, 0x1F),
	/* Set IE dimming mode */
	EXYNOS_DSI_CMD_SEQ(0x6A, 0x21),

	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* BBh (MIPI via/bypass RAM) */
	EXYNOS_DSI_CMD_SEQ(0xBB, 0x13),
	/* VBP + VFP = 200 + 26 = 226 */
	EXYNOS_DSI_CMD_SEQ(0x3B, 0x03, 0xC8, 0x1A, 0x04, 0x04),

	/* b/201704777: Flip 180 degrees */
	EXYNOS_DSI_CMD_SEQ(0x36, 0x03),

	EXYNOS_DSI_CMD_SEQ_DELAY(120, MIPI_DCS_EXIT_SLEEP_MODE),
	EXYNOS_DSI_CMD_SEQ(MIPI_DCS_SET_DISPLAY_ON)
};
static DEFINE_EXYNOS_CMD_SET(ppa957db2d_init);

static const struct exynos_dsi_cmd ppa957db2d_off_cmds[] = {
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ_DELAY(20, MIPI_DCS_SET_DISPLAY_OFF),
	EXYNOS_DSI_CMD_SEQ_DELAY(100, MIPI_DCS_ENTER_SLEEP_MODE),
};
static DEFINE_EXYNOS_CMD_SET(ppa957db2d_off);

/**
 * struct ppa957db2d_panel - panel specific info
 * This struct maintains ppa957db2d panel specific information, any fixed details about
 * panel should most likely go into struct exynos_panel or exynos_panel_desc
 */
struct ppa957db2d_panel {
	/** @base: base panel struct */
	struct exynos_panel base;

	/** @i2c_pwr: i2c power supply */
	struct regulator *i2c_pwr;

	/** @avdd: avdd regulator for TDDI */
	struct regulator *avdd;

	/** @avee: avee regulator for TDDI */
	struct regulator *avee;

	/** @avdd_uV: microVolt of avdd */
	u32 avdd_uV;

	/** @avee_uV: microVolt of avee */
	u32 avee_uV;
};

#define to_spanel(ctx) container_of(ctx, struct ppa957db2d_panel, base)

static void ppa957db2d_reset(struct exynos_panel *ctx)
{
	dev_dbg(ctx->dev, "%s +\n", __func__);

	if (ctx->panel_state == PANEL_STATE_BLANK) {
		gpiod_set_value(ctx->reset_gpio, 0);
		usleep_range(1000, 1100);
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(1000, 1100);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(1000, 1100);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000, 10100);

	dev_dbg(ctx->dev, "%s -\n", __func__);
}

static int ppa957db2d_prepare(struct drm_panel *panel)
{
	struct exynos_panel *ctx =
		container_of(panel, struct exynos_panel, panel);

	dev_dbg(ctx->dev, "%s +\n", __func__);

	exynos_panel_set_power(ctx, true);
	usleep_range(18500, 18600);
	ppa957db2d_reset(ctx);

	dev_dbg(ctx->dev, "%s -\n", __func__);

	return 0;
}

static void ts110f5mlg0_set_cabc_mode(struct exynos_panel *ctx,
					enum exynos_cabc_mode cabc_mode)
{
	u8 mode;

	switch (cabc_mode) {
	case CABC_UI_MODE:
		mode = 0x01;
		break;
	case CABC_STILL_MODE:
		mode = 0x02;
		break;
	case CABC_MOVIE_MODE:
		/* CABC MOVING MODE & IE */
		mode = 0x83;
		break;
	default:
		mode = 0x00;
	}
	EXYNOS_DCS_WRITE_SEQ(ctx, 0x55, mode);

	dev_dbg(ctx->dev, "%s CABC state: %u\n", __func__, mode);
}

static int ppa957db2d_enable(struct drm_panel *panel)
{
	struct exynos_panel *ctx;

	ctx = container_of(panel, struct exynos_panel, panel);

	exynos_panel_init(ctx);
	exynos_panel_send_cmd_set(ctx, &ppa957db2d_init_cmd_set);
	ctx->enabled = true;

	return 0;
}

static int ppa957db2d_read_id(struct exynos_panel *ctx)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int read_bytes = 0;
	u8 i;

	if (ctx->panel_rev < PANEL_REV_EVT2) {
		/* hardcode 0 as reading id is not supported in this panel_rev */
		dev_info(ctx->dev, "read_id is not supported in panel_rev: 0x%x\n", ctx->panel_rev);
		strlcpy(ctx->panel_id, "0", PANEL_ID_MAX);
		return 0;
	}

	/* Change to CMD2, Page2 */
	EXYNOS_DCS_WRITE_SEQ(ctx, 0xFF, 0x22);
	EXYNOS_DCS_WRITE_SEQ(ctx, 0xFB, 0x01);

	/* Serial number is stored in different registers, use loop to read it. */
	for (i = 0; i < PPA957DB2D_PANEL_ID_LEN; ++i) {
		read_bytes = mipi_dsi_dcs_read(dsi, PPA957DB2D_PANEL_ID_REG + i,
				ctx->panel_id + i, 1);
		if (read_bytes != 1)
			break;
	}

	/* Switch back to CMD1 */
	EXYNOS_DCS_WRITE_SEQ(ctx, 0xFF, 0x10);
	EXYNOS_DCS_WRITE_SEQ(ctx, 0xFB, 0x01);

	if (read_bytes != 1) {
		dev_warn(ctx->dev, "Unable to read panel id (%d)\n", read_bytes);
		strlcpy(ctx->panel_id, "0", PANEL_ID_MAX);
		return -EIO;
	}

	ctx->panel_id[PPA957DB2D_PANEL_ID_LEN] = '\0';

	return 0;
}

static void ppa957db2d_update_wrctrld(struct exynos_panel *ctx)
{
	u8 val = PPA957DB2D_WRCTRLD_BCTRL_BIT |
			PPA957DB2D_WRCTRLD_BL_BIT;

	if (ctx->dimming_on)
		val |= PPA957DB2D_WRCTRLD_DD_BIT;

	dev_dbg(ctx->dev,
		"%s(wrctrld:0x%x, dimming: %s)\n",
		__func__, val, ctx->dimming_on ? "on" : "off");

	EXYNOS_DCS_WRITE_SEQ(ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY, val);
}

static void ppa957db2d_set_dimming_on(struct exynos_panel *ctx,
					bool dimming_on)
{
	ctx->dimming_on = dimming_on;
	ppa957db2d_update_wrctrld(ctx);
}

static void ppa957db2d_get_panel_rev(struct exynos_panel *ctx, u32 id)
{
	/* extract command 0xDB */
	u8 build_code = (id & 0xFF00) >> 8;
	u8 rev = build_code >> 4;

	if (rev >= ARRAY_SIZE(ppa957db2d_panel_rev)) {
		ctx->panel_rev = PANEL_REV_LATEST;
		dev_warn(ctx->dev,
			"unknown rev from panel (0x%x), default to latest\n",
			rev);
	} else {
		ctx->panel_rev = ppa957db2d_panel_rev[rev];
		dev_info(ctx->dev, "panel_rev: 0x%x\n", ctx->panel_rev);
	}
}

static int ppa957db2d_parse_regualtors(struct exynos_panel *ctx)
{
	struct device *dev = ctx->dev;
	struct ppa957db2d_panel *spanel = to_spanel(ctx);
	int count, i, ret;

	ctx->vddi = devm_regulator_get(dev, "vddi");
	if (IS_ERR(ctx->vddi)) {
		dev_err(ctx->dev, "failed to get panel vddi\n");
		return -EPROBE_DEFER;
	}

	/* The i2c power source and backlight enable (BL_EN) use the same hardware pin.
	 * We should be cautious when controlling this hardware pin (b/244526124).
	 */
	spanel->i2c_pwr = devm_regulator_get_optional(dev, "i2c-pwr");
	if (PTR_ERR_OR_ZERO(spanel->i2c_pwr)) {
		dev_err(ctx->dev, "failed to get display i2c-pwr\n");
		return -EPROBE_DEFER;
	}

	/* log the device tree status for every display bias source */
	count = of_property_count_elems_of_size(dev->of_node, "disp_bias", sizeof(u32));
	if (count <= 0) {
		dev_err(ctx->dev, "failed to parse disp_bias entry\n");
		return -EINVAL;
	}
	for (i = 0; i < count; ++i) {
		struct device_node *dev_node = of_parse_phandle(dev->of_node, "disp_bias", i);

		if (of_device_is_available(dev_node))
			dev_info(ctx->dev, "%s is enabled by bootloader\n", dev_node->full_name);
		else
			dev_dbg(ctx->dev, "%s is disabled by bootloader\n", dev_node->full_name);
	}

	spanel->avdd = devm_regulator_get_optional(dev, "disp_avdd");
	if (PTR_ERR_OR_ZERO(spanel->avdd)) {
		dev_err(ctx->dev, "failed to get disp_avdd provider\n");
		return -EPROBE_DEFER;
	}

	spanel->avee = devm_regulator_get_optional(dev, "disp_avee");
	if (PTR_ERR_OR_ZERO(spanel->avee)) {
		dev_err(ctx->dev, "failed to get disp_avee provider\n");
		return -EPROBE_DEFER;
	}

	ret = of_property_read_u32(dev->of_node, "avdd-microvolt", &spanel->avdd_uV);
	if (ret) {
		dev_err(ctx->dev, "failed to parse avdd-microvolt: %d\n", ret);
		return ret;
	}
	dev_dbg(ctx->dev, "use avdd-microvolt: %d uV\n", spanel->avdd_uV);

	ret = of_property_read_u32(dev->of_node, "avee-microvolt", &spanel->avee_uV);
	if (ret) {
		dev_err(ctx->dev, "failed to parse avee-microvolt: %d\n", ret);
		return ret;
	}
	dev_dbg(ctx->dev, "use avee-microvolt: %d uV\n", spanel->avee_uV);

	return 0;
}

static int ppa957db2d_set_power(struct exynos_panel *ctx, bool on)
{
	struct ppa957db2d_panel *spanel = to_spanel(ctx);
	int ret;

	if (on) {
		/* Case 1. set_power when handoff from bootloader.
		 *    1. i2c_pwr (BL_EN) is left on (use_count = 0)
		 *    2. ppa957db2d_set_power +
		 *    3. ppa957db2d_set_power -
		 *    4. i2c_pwr (BL_EN) is left on (use_count = 0)
		 *    5. backlight driver turn on i2c_pwr (BL_EN) (use_count = 1)
		 *
		 * Case 2. system resume (tap to check tablet is disabled)
		 *    1. i2c_pwr (BL_EN) is off (use_count = 0)
		 *    2. ppa957db2d_set_power +
		 *    3. ppa957db2d_set_power -
		 *    4. i2c_pwr (BL_EN) is off (use_count = 0)
		 *    5. backlight driver turn on i2c_pwr (BL_EN) (use_count = 1)
		 *
		 * Case 3. system resume (tap to check tablet is enabled)
		 *    1. i2c_pwr (BL_EN) is off (use_count = 0)
		 *    2. backlight driver turn on i2c_pwr (BL_EN) (use_count = 1)
		 */
		bool i2c_pwr_already_on;

		/* VDDI power */
		ret = regulator_enable(ctx->vddi);
		if (ret) {
			dev_err(ctx->dev, "vddi enable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "vddi enable successfully\n");
		usleep_range(2000, 3000);

		i2c_pwr_already_on = regulator_is_enabled(spanel->i2c_pwr);
		if (!i2c_pwr_already_on) {
			/* For case 1, the i2c_pwr (BL_EN) should be turned on manually to
			 *     configure the AVDD/AVEE voltage level via i2c.
			 * For case 2, the i2c_pwr (BL_EN) is already turned on (used_count = 0)
			 *     and should not turned on here. Otherwise, it need to be turned off
			 *     later to reset the use_count to zero. However turning off will
			 *     affect the continuous splash feature (black flicker).
			 */
			ret = regulator_enable(spanel->i2c_pwr);
			if (ret) {
				dev_err(ctx->dev, "i2c_pwr enable failed\n");
				return ret;
			}
			dev_dbg(ctx->dev, "i2c_pwr enable successfully\n");
			usleep_range(2000, 2500);
		}

		/* AVDD power */
		ret = regulator_enable(spanel->avdd);
		if (ret) {
			dev_err(ctx->dev, "avdd enable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "avdd enable successfully\n");

		/* set voltage twice to fix the problem from tps65132_enable: it doesn't
		 * restore the voltage register value via regmap_write (SW value and HW value
		 * are inconsistent). At this time, set the voltage to target value directly
		 * will not take effect because the direct return condition in
		 * regulator_set_voltage_unlocked:
		 * `voltage->min_uV == min_uV && voltage->max_uV == max_uV`
		 */
		if (regulator_set_voltage(spanel->avdd, spanel->avdd_uV - 100000,
			spanel->avdd_uV - 100000) || regulator_set_voltage(spanel->avdd,
			spanel->avdd_uV, spanel->avdd_uV)) {
			dev_err(ctx->dev, "avdd set voltage failed\n");
			/* If regulator_set_voltage fail, the display can still be light on
			 * with default voltage level, should not return here.
			 */
		} else {
			dev_dbg(ctx->dev, "avdd set voltage successfully\n");
		}
		usleep_range(1000, 1100);

		/* AVEE power */
		ret = regulator_enable(spanel->avee);
		if (ret) {
			dev_err(ctx->dev, "avee enable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "avee enable successfully\n");

		/* set voltage twice as AVDD */
		if (regulator_set_voltage(spanel->avee, spanel->avee_uV - 100000,
			spanel->avee_uV - 100000) || regulator_set_voltage(spanel->avee,
			spanel->avee_uV, spanel->avee_uV)) {
			dev_err(ctx->dev, "avee set voltage failed\n");
			/* If regulator_set_voltage fail, the display can still be light on
			 * with default voltage level, should not return here.
			 */
		} else {
			dev_dbg(ctx->dev, "avee set voltage successfully\n");
		}
		usleep_range(1000, 1100);

		if (!i2c_pwr_already_on) {
			/* For case 2, the i2c_pwr (BL_EN) should be reset to use_count 0.
			 * Such that the backlight driver can fully control on the BL_EN.
			 */
			if (regulator_disable(spanel->i2c_pwr))
				dev_err(ctx->dev, "i2c_pwr disable failed\n");
			else
				dev_dbg(ctx->dev, "i2c_pwr disable successfully\n");
		}
	} else {
		/* Case 1. system suspend (tap to check tablet is disabled)
		 *    1. i2c_pwr (BL_EN) is on (use_count = 1)
		 *    2. backlight driver turn off i2c_pwr (BL_EN) (use_count = 0)
		 *    3. ppa957db2d_set_power +
		 *    4. only turn off DISP_PMIC_ENABLE gpio pin, no i2c access here.
		 *    5. ppa957db2d_set_power -
		 *
		 * Case 2. system suspend (tap to check tablet is enabled)
		 *    1. i2c_pwr (BL_EN) is on (use_count = 1)
		 *    2. backlight driver turn off i2c_pwr (BL_EN) (use_count = 0)
		 */
		gpiod_set_value(ctx->reset_gpio, 0);

		ret = regulator_disable(spanel->avee);
		if (ret) {
			dev_err(ctx->dev, "avee disable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "avee disable successfully\n");
		usleep_range(1000, 1100);

		ret = regulator_disable(spanel->avdd);
		if (ret) {
			dev_err(ctx->dev, "avdd disable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "avdd disable successfully\n");
		usleep_range(6000, 7000);

		ret = regulator_disable(ctx->vddi);
		if (ret) {
			dev_err(ctx->dev, "vddi disable failed\n");
			return ret;
		}
		dev_dbg(ctx->dev, "vddi disable successfully\n");
	}

	return 0;
}

static int ppa957db2d_panel_probe(struct mipi_dsi_device *dsi)
{
	struct ppa957db2d_panel *spanel;

	spanel = devm_kzalloc(&dsi->dev, sizeof(*spanel), GFP_KERNEL);
	if (!spanel)
		return -ENOMEM;

	return exynos_panel_common_init(dsi, &spanel->base);
}

static const struct exynos_panel_mode ppa957db2d_modes[] = {
	{
		/* 1600x2560 @ 60 */
		.mode = {
			.clock = 309246,
			.hdisplay = 1600,
			.hsync_start = 1600 + 92, // add hfp
			.hsync_end = 1600 + 92 + 66, // add hsa
			.htotal = 1600 + 92 + 66 + 92, // add hbp
			.vdisplay = 2560,
			.vsync_start = 2560 + 26, // add vfp
			.vsync_end = 2560 + 26 + 4, // add vsa
			.vtotal = 2560 + 26 + 4 + 196, // add vbp
			.flags = 0,
			.width_mm = 147,
			.height_mm = 236,
		},
		.exynos_mode = {
			.mode_flags = MIPI_DSI_MODE_VIDEO,
			.bpc = 8,
			.dsc = {
				.enabled = false,
			},
		},
	},
};

static const struct drm_panel_funcs ppa957db2d_drm_funcs = {
	.disable = exynos_panel_disable,
	.unprepare = exynos_panel_unprepare,
	.prepare = ppa957db2d_prepare,
	.enable = ppa957db2d_enable,
	.get_modes = exynos_panel_get_modes,
};

static const struct exynos_panel_funcs ppa957db2d_exynos_funcs = {
	.read_id = ppa957db2d_read_id,
	.panel_reset = ppa957db2d_reset,
	.set_dimming_on = ppa957db2d_set_dimming_on,
	.set_brightness = exynos_panel_set_brightness,
	.set_cabc_mode = ts110f5mlg0_set_cabc_mode,
	.get_panel_rev = ppa957db2d_get_panel_rev,
	.parse_regulators = ppa957db2d_parse_regualtors,
	.set_power = ppa957db2d_set_power,
};

const struct brightness_capability ppa957db2d_brightness_capability = {
	.normal = {
		.nits = {
			.min = 2,
			.max = 500,
		},
		.level = {
			.min = 16,
			.max = 4095,
		},
		.percentage = {
			.min = 0,
			.max = 100,
		},
	},
};

static const struct exynos_panel_desc csot_ppa957db2d = {
	.data_lane_cnt = 4,
	.max_brightness = 4095,
	.min_brightness = 16,
	.lower_min_brightness = 4,
	.dft_brightness = 1146,
	/* supported HDR format bitmask : 1(DOLBY_VISION), 2(HDR10), 3(HLG) */
	.hdr_formats = BIT(2) | BIT(3),
	.max_luminance = 5000000,
	.max_avg_luminance = 1200000,
	.min_luminance = 5,
	.brt_capability = &ppa957db2d_brightness_capability,
	.modes = ppa957db2d_modes,
	.num_modes = 1,
	.off_cmd_set = &ppa957db2d_off_cmd_set,
	.panel_func = &ppa957db2d_drm_funcs,
	.exynos_panel_func = &ppa957db2d_exynos_funcs,
};

static const struct of_device_id exynos_panel_of_match[] = {
	{ .compatible = "csot,ppa957db2d", .data = &csot_ppa957db2d },
	{ }
};
MODULE_DEVICE_TABLE(of, exynos_panel_of_match);

static struct mipi_dsi_driver exynos_panel_driver = {
	.probe = ppa957db2d_panel_probe,
	.remove = exynos_panel_remove,
	.driver = {
		.name = "panel-csot-ppa957db2d",
		.of_match_table = exynos_panel_of_match,
	},
};
module_mipi_dsi_driver(exynos_panel_driver);

MODULE_AUTHOR("Ken Huang <kenbshuang@google.com>");
MODULE_DESCRIPTION("MIPI-DSI based CSOT ppa957db2d panel driver");
MODULE_LICENSE("GPL");
