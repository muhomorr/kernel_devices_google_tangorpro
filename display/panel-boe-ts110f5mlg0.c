// SPDX-License-Identifier: GPL-2.0-only
/*
 * MIPI-DSI based BOE TS110F5M-LG0 LCD panel driver.
 *
 * Copyright (c) 2021 Google Inc.
 *
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <video/mipi_display.h>

#include "samsung/panel/panel-samsung-drv.h"

#define TS110F5MLG0_WRCTRLD_DD_BIT	0x08
#define TS110F5MLG0_WRCTRLD_BL_BIT	0x04
#define TS110F5MLG0_WRCTRLD_BCTRL_BIT	0x20
#define TS110F5MLG0_PANEL_ID_REG	0x00
#define TS110F5MLG0_PANEL_ID_LEN	37

#if TS110F5MLG0_PANEL_ID_LEN >= PANEL_ID_MAX
	#error PANEL_ID_MAX should be greater than TS110F5MLG0_PANEL_ID_LEN
#endif

static const u8 display_on[] = { 0x29 };
static const u8 display_off[] = { 0x28 };
static const u8 sleep_out[] = { 0x11 };
static const u8 sleep_in[] = { 0x10 };

static const u32 ts110f5mlg0_panel_rev[] = {
	PANEL_REV_PROTO1,
	PANEL_REV_PROTO2,
	PANEL_REV_EVT1,
	PANEL_REV_EVT1_1,
	PANEL_REV_EVT2,
	PANEL_REV_DVT1,
	PANEL_REV_PVT,
};

static const struct exynos_dsi_cmd ts110f5mlg0_init_cmds[] = {
	/* CMD2, Page0 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x20),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x5A, 0x14),

	/* CMD2, Page3 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x23),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* 12 bits PWM */
	EXYNOS_DSI_CMD_SEQ(0x00, 0x80),
	/* PWM freq 3kHz */
	EXYNOS_DSI_CMD_SEQ(0x08, 0x04),
	/* APL_THD */
	EXYNOS_DSI_CMD_SEQ(0x11, 0x02),
	EXYNOS_DSI_CMD_SEQ(0x12, 0x80),
	/* APL_COMP */
	EXYNOS_DSI_CMD_SEQ(0x15, 0x83),
	EXYNOS_DSI_CMD_SEQ(0x16, 0x0C),

	/* CMD2, Page5 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x25),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x13, 0x02),
	EXYNOS_DSI_CMD_SEQ(0x14, 0x41),

	/* CMD2, Page6 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x26),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x01, 0xB0),
	EXYNOS_DSI_CMD_SEQ(0x02, 0x31),
	EXYNOS_DSI_CMD_SEQ(0x32, 0x9F),

	/* CMD2, Page7 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x27),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x5B, 0x02),

	/* CMD2, Page4 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x24),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0xC2, 0xDA),
	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Only Write Secondary */
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x05),
	/* CMD2, Page4 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x24),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0xC2, 0xDF),
	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Write Primary & Secondary */
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x02),

	/* CMD3, PageA */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0xE0),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* VCOM Driving Ability */
	EXYNOS_DSI_CMD_SEQ(0x14, 0x60),
	EXYNOS_DSI_CMD_SEQ(0x16, 0xC0),

	/* CMD3, PageB */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0xF0),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Secondary osc workaround */
	EXYNOS_DSI_CMD_SEQ(0x3A, 0x08),

	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Only Write Secondary */
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x05),

	/* CMD2, Page0 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x20),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD_SEQ(0x18, 0x40),

	/* CMD1 */
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	/* Write Primary & Secondary */
	EXYNOS_DSI_CMD_SEQ(0xB9, 0x02),
	EXYNOS_DSI_CMD_SEQ(0x51, 0x0F, 0xFF),
	EXYNOS_DSI_CMD_SEQ(0x53, 0x24),
	/* CABC initial OFF */
	EXYNOS_DSI_CMD_SEQ(0x55, 0x00),
	/* BBh (MIPI via/bypass RAM) */
	EXYNOS_DSI_CMD_SEQ(0xBB, 0x13),
	/* VBP + VFP = 200 + 26 = 226 */
	EXYNOS_DSI_CMD_SEQ(0x3B, 0x03, 0xC8, 0x1A, 0x04, 0x04),

	/* b/201704777: Flip 180 degrees */
	EXYNOS_DSI_CMD_SEQ(0x36, 0x03),

	EXYNOS_DSI_CMD(sleep_out, 120),
	EXYNOS_DSI_CMD(display_on, 0)
};
static DEFINE_EXYNOS_CMD_SET(ts110f5mlg0_init);

static const struct exynos_dsi_cmd ts110f5mlg0_off_cmds[] = {
	EXYNOS_DSI_CMD_SEQ(0xFF, 0x10),
	EXYNOS_DSI_CMD_SEQ(0xFB, 0x01),
	EXYNOS_DSI_CMD(display_off, 20),
	EXYNOS_DSI_CMD(sleep_in, 100),
};
static DEFINE_EXYNOS_CMD_SET(ts110f5mlg0_off);

static void ts110f5mlg0_reset(struct exynos_panel *ctx)
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

static int ts110f5mlg0_prepare(struct drm_panel *panel)
{
	struct exynos_panel *ctx =
		container_of(panel, struct exynos_panel, panel);

	dev_dbg(ctx->dev, "%s +\n", __func__);

	exynos_panel_set_power(ctx, true);
	usleep_range(18500, 18600);
	ts110f5mlg0_reset(ctx);

	dev_dbg(ctx->dev, "%s -\n", __func__);

	return 0;
}

static int ts110f5mlg0_enable(struct drm_panel *panel)
{
	struct exynos_panel *ctx;

	ctx = container_of(panel, struct exynos_panel, panel);

	exynos_panel_init(ctx);
	exynos_panel_send_cmd_set(ctx, &ts110f5mlg0_init_cmd_set);
	ctx->enabled = true;

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
		mode = 0x03;
		break;
	default:
		mode = 0x00;
	}
	EXYNOS_DCS_WRITE_SEQ(ctx, 0x55, mode);

	dev_dbg(ctx->dev, "%s CABC state: %u\n", __func__, mode);
}

static int ts110f5mlg0_read_id(struct exynos_panel *ctx)
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
	for (i = 0; i < TS110F5MLG0_PANEL_ID_LEN; ++i) {
		read_bytes = mipi_dsi_dcs_read(dsi, TS110F5MLG0_PANEL_ID_REG + i,
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

	ctx->panel_id[TS110F5MLG0_PANEL_ID_LEN] = '\0';

	return 0;
}

static void ts110f5mlg0_update_wrctrld(struct exynos_panel *ctx)
{
	u8 val = TS110F5MLG0_WRCTRLD_BCTRL_BIT |
			TS110F5MLG0_WRCTRLD_BL_BIT;

	if (ctx->dimming_on)
		val |= TS110F5MLG0_WRCTRLD_DD_BIT;

	dev_dbg(ctx->dev,
		"%s(wrctrld:0x%x, dimming: %s)\n",
		__func__, val, ctx->dimming_on ? "on" : "off");

	EXYNOS_DCS_WRITE_SEQ(ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY, val);
}

static void ts110f5mlg0_set_dimming_on(struct exynos_panel *ctx,
					bool dimming_on)
{
	ctx->dimming_on = dimming_on;
	ts110f5mlg0_update_wrctrld(ctx);
}

static void ts110f5mlg0_get_panel_rev(struct exynos_panel *ctx, u32 id)
{
	/* extract command 0xDB */
	u8 build_code = (id & 0xFF00) >> 8;
	u8 rev = build_code >> 4;

	if (rev >= ARRAY_SIZE(ts110f5mlg0_panel_rev)) {
		ctx->panel_rev = PANEL_REV_LATEST;
		dev_warn(ctx->dev,
			"unknown rev from panel (0x%x), default to latest\n",
			rev);
	} else {
		ctx->panel_rev = ts110f5mlg0_panel_rev[rev];
		dev_info(ctx->dev, "panel_rev: 0x%x\n", ctx->panel_rev);
	}
}

static const struct exynos_panel_mode ts110f5mlg0_modes[] = {
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

static const struct drm_panel_funcs ts110f5mlg0_drm_funcs = {
	.disable = exynos_panel_disable,
	.unprepare = exynos_panel_unprepare,
	.prepare = ts110f5mlg0_prepare,
	.enable = ts110f5mlg0_enable,
	.get_modes = exynos_panel_get_modes,
};

static const struct exynos_panel_funcs ts110f5mlg0_exynos_funcs = {
	.read_id = ts110f5mlg0_read_id,
	.panel_reset = ts110f5mlg0_reset,
	.set_dimming_on = ts110f5mlg0_set_dimming_on,
	.set_brightness = exynos_panel_set_brightness,
	.set_cabc_mode = ts110f5mlg0_set_cabc_mode,
	.get_panel_rev = ts110f5mlg0_get_panel_rev,
};

const struct brightness_capability ts110f5mlg0_brightness_capability = {
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

static const struct exynos_panel_desc boe_ts110f5mlg0 = {
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
	.brt_capability = &ts110f5mlg0_brightness_capability,
	.modes = ts110f5mlg0_modes,
	.num_modes = 1,
	.off_cmd_set = &ts110f5mlg0_off_cmd_set,
	.panel_func = &ts110f5mlg0_drm_funcs,
	.exynos_panel_func = &ts110f5mlg0_exynos_funcs,
};

static const struct of_device_id exynos_panel_of_match[] = {
	{ .compatible = "boe,ts110f5mlg0", .data = &boe_ts110f5mlg0 },
	{ }
};
MODULE_DEVICE_TABLE(of, exynos_panel_of_match);

static struct mipi_dsi_driver exynos_panel_driver = {
	.probe = exynos_panel_probe,
	.remove = exynos_panel_remove,
	.driver = {
		.name = "panel-boe-ts110f5mlg0",
		.of_match_table = exynos_panel_of_match,
	},
};
module_mipi_dsi_driver(exynos_panel_driver);

MODULE_AUTHOR("Ken Huang <kenbshuang@google.com>");
MODULE_DESCRIPTION("MIPI-DSI based BOE ts110f5mlg0 panel driver");
MODULE_LICENSE("GPL");
