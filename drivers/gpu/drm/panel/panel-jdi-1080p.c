#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

static const char * const regulator_names[] = {
	"vddp",
	"iovcc"
};

struct jdi_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];

	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *dcdc_en_gpio;
	//struct backlight_device *backlight;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

static inline struct jdi_panel *to_jdi_panel(struct drm_panel *panel)
{
	return container_of(panel, struct jdi_panel, base);
}

static int jdi_panel_init(struct jdi_panel *jdi)
{
	struct mipi_dsi_device *dsi = jdi->dsi;
	struct device *dev = &jdi->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0)
		return ret;

	usleep_range(10000, 20000);

	ret = mipi_dsi_generic_write(dsi, (u8[]){ 0xb0, 0x04 }, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 1: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]){0x00, 0x00}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 2: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]){0x00, 0x00}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xb3, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00}, 7);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_generic_write(dsi, (u8[]){0xb6, 0x3a, 0xd3}, 3);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc1,
                                  0x84, 0x60, 0x50, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x0c, 0x01, 0x58,
                                  0x73, 0xae, 0x31, 0x20, 0x06, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
                                  0x10, 0x10, 0x10, 0x00, 0x00, 0x00,
                                  0x22, 0x02, 0x02, 0x00}, 35);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc2,
                                  0x30, 0xf7, 0x80, 0x0a, 0x08, 0x00,
                                  0x00}, 8);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc4,
                                  0x70, 0x00, 0x00, 0x00, 0x00, 0x04,
                                  0x00, 0x00, 0x00, 0x11, 0x06, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
                                  0x00, 0x00, 0x11, 0x06}, 23);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc6,
                                  0x06, 0x6d, 0x06, 0x6d, 0x06, 0x6d,
                                  0x00, 0x00, 0x00, 0x00, 0x06, 0x6d,
                                  0x06, 0x6d, 0x06, 0x6d, 0x15, 0x19,
                                  0x07, 0x00, 0x01, 0x06, 0x6d, 0x06,
                                  0x6d, 0x06, 0x6d, 0x00, 0x00, 0x00,
                                  0x00, 0x06, 0x6d, 0x06, 0x6d, 0x06,
                                  0x6d, 0x15, 0x19, 0x07}, 41);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc7,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}, 25);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc8,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}, 25);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xc9,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
                                  0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
                                  0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}, 25);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xcc, 0x09}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xd0,
                                  0x00, 0x00, 0x19, 0x18, 0x99, 0x99,
                                  0x19, 0x01, 0x89, 0x00, 0x55, 0x19,
                                  0x99, 0x01}, 15);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xd3,
                                  0x1b, 0x33, 0xbb, 0xcc, 0xc4, 0x33,
                                  0x33, 0x33, 0x00, 0x01, 0x00, 0xa0,
                                  0xd8, 0xa0, 0x0d, 0x37, 0x33, 0x44,
                                  0x22, 0x70, 0x02, 0x37, 0x03, 0x3d,
                                  0xbf, 0x00}, 27);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_generic_write(dsi, (u8[]){0xd5,
                                  0x06, 0x00, 0x00, 0x01, 0x4a, 0x01,
                                  0x4a}, 8);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_generic_write(dsi, (u8[]){
                                  0xd5, 0x06, 0x00, 0x00, 0x01, 0x4a,
                                  0x01, 0x4a}, 8);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]){0x36, 0xc0}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]){0x29, 0x00}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}
	ret = mipi_dsi_dcs_write_buffer(dsi, (u8[]){0x11, 0x00}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to run seq 3: %d\n", ret);
		return ret;
	}

	return ret;
}

static int jdi_panel_on(struct jdi_panel *jdi)
{
	struct mipi_dsi_device *dsi = jdi->dsi;
	struct device *dev = &jdi->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display on: %d\n", ret);


	return ret;
}

static void jdi_panel_off(struct jdi_panel *jdi)
{
	struct mipi_dsi_device *dsi = jdi->dsi;
	struct device *dev = &jdi->dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		dev_err(dev, "failed to enter sleep mode: %d\n", ret);

	msleep(100);
}

static int jdi_panel_disable(struct drm_panel *panel)
{
	struct jdi_panel *jdi = to_jdi_panel(panel);

	if (!jdi->enabled)
		return 0;

	//TODO: backlight
	//jdi->backlight->props.power = fb_blank_powerdown;
	//backlight_update_status(jdi->backlight);

	jdi->enabled = false;

	return 0;
}


static int jdi_panel_unprepare(struct drm_panel *panel)
{
	struct jdi_panel *jdi = to_jdi_panel(panel);
	struct device *dev = &jdi->dsi->dev;
	int ret;

	if (!jdi->prepared)
		return 0;

	jdi_panel_off(jdi);

	ret = regulator_bulk_disable(ARRAY_SIZE(jdi->supplies), jdi->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(jdi->enable_gpio, 0);

	gpiod_set_value(jdi->reset_gpio, 1);

	gpiod_set_value(jdi->dcdc_en_gpio, 0);

	jdi->prepared = false;

	return 0;
}


static int jdi_panel_prepare(struct drm_panel *panel)
{
	struct jdi_panel *jdi = to_jdi_panel(panel);
	struct device *dev = &jdi->dsi->dev;
	int ret;

	if (jdi->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(jdi->supplies), jdi->supplies);
	if (ret < 0) {
		dev_err(dev, "regulator enable failed, %d\n", ret);
		return ret;
	}

	msleep(20);

	gpiod_set_value(jdi->dcdc_en_gpio, 1);
	usleep_range(10, 20);

	gpiod_set_value(jdi->reset_gpio, 0);
	usleep_range(10, 20);

	gpiod_set_value(jdi->enable_gpio, 1);
	usleep_range(10, 20);

	ret = jdi_panel_init(jdi);
	if (ret < 0) {
		dev_err(dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	ret = jdi_panel_on(jdi);
	if (ret < 0) {
		dev_err(dev, "failed to set panel on: %d\n", ret);
		goto poweroff;
	}

	jdi->prepared = true;

	return 0;

poweroff:
	ret = regulator_bulk_disable(ARRAY_SIZE(jdi->supplies), jdi->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(jdi->enable_gpio, 0);

	gpiod_set_value(jdi->reset_gpio, 1);

	gpiod_set_value(jdi->dcdc_en_gpio, 0);

	return ret;
}


static int jdi_panel_enable(struct drm_panel *panel)
{
	struct jdi_panel *jdi = to_jdi_panel(panel);

	if (jdi->enabled)
		return 0;

	//TODO
	//jdi->backlight->props.power = FB_BLANK_UNBLANK;
	//backlight_update_status(jdi->backlight);

	jdi->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
		.clock = 148876, //148875.6
		.hdisplay = 1080,
		.hsync_start = 1080 + 100,
		.hsync_end = 1080 + 100 + 4,
		.htotal = 1080 + 100 + 4 + 95,
		.vdisplay = 1920,
		.vsync_start = 1920 + 10,
		.vsync_end = 1920 + 10 + 1,
		.vtotal = 1920 + 10 + 1 + 9,
		.vrefresh = 30,
		.flags = 0
};

static int jdi_panel_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct jdi_panel *jdi = to_jdi_panel(panel);
	struct device *dev = &jdi->dsi->dev;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(panel->connector, mode);

	//TODO
	//panel->connector->display_info.width_mm = 95;
	//panel->connector->display_info.height_mm = 151;

	return 1;
}

static const struct drm_panel_funcs jdi_panel_funcs = {
	.disable = jdi_panel_disable,
	.unprepare = jdi_panel_unprepare,
	.prepare = jdi_panel_prepare,
	.enable = jdi_panel_enable,
	.get_modes = jdi_panel_get_modes,
};

static const struct of_device_id jdi_of_match[] = {
	{ .compatible = "jdi,1080p", },
	{ }
};
MODULE_DEVICE_TABLE(of, jdi_of_match);

static int jdi_panel_add(struct jdi_panel *jdi)
{
	struct device *dev = &jdi->dsi->dev;
	int ret;
	unsigned int i;

	jdi->mode = &default_mode;

	for (i = 0; i < ARRAY_SIZE(jdi->supplies); i++)
		jdi->supplies[i].supply = regulator_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(jdi->supplies),
				      jdi->supplies);
	if (ret < 0) {
		dev_err(dev, "failed to init regulator, ret=%d\n", ret);
		return ret;
	}

	jdi->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(jdi->enable_gpio)) {
		ret = PTR_ERR(jdi->enable_gpio);
		dev_err(dev, "cannot get enable-gpio %d\n", ret);
		return ret;
	}

	jdi->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(jdi->reset_gpio)) {
		ret = PTR_ERR(jdi->reset_gpio);
		dev_err(dev, "cannot get reset-gpios %d\n", ret);
		return ret;
	}

	jdi->dcdc_en_gpio = devm_gpiod_get(dev, "dcdc-en", GPIOD_OUT_LOW);
	if (IS_ERR(jdi->dcdc_en_gpio)) {
		ret = PTR_ERR(jdi->dcdc_en_gpio);
		dev_err(dev, "cannot get dcdc-en-gpio %d\n", ret);
		return ret;
	}

	//TODO
	/*
	jdi->backlight = drm_panel_create_dsi_backlight(jdi->dsi);
	if (IS_ERR(jdi->backlight)) {
		ret = PTR_ERR(jdi->backlight);
		dev_err(dev, "failed to register backlight %d\n", ret);
		return ret;
	}
	*/

	drm_panel_init(&jdi->base);
	jdi->base.funcs = &jdi_panel_funcs;
	jdi->base.dev = &jdi->dsi->dev;

	ret = drm_panel_add(&jdi->base);

	return ret;
}

static void jdi_panel_del(struct jdi_panel *jdi)
{
	if (jdi->base.dev)
		drm_panel_remove(&jdi->base);
}

static int jdi_panel_probe(struct mipi_dsi_device *dsi)
{
	struct jdi_panel *jdi;
	int ret;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO |
			   MIPI_DSI_CLOCK_NON_CONTINUOUS;

	jdi = devm_kzalloc(&dsi->dev, sizeof(*jdi), GFP_KERNEL);
	if (!jdi)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, jdi);

	jdi->dsi = dsi;

	ret = jdi_panel_add(jdi);
	if (ret < 0)
		return ret;

	return mipi_dsi_attach(dsi);
}

static int jdi_panel_remove(struct mipi_dsi_device *dsi)
{
	struct jdi_panel *jdi = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = jdi_panel_disable(&jdi->base);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n",
			ret);

	drm_panel_detach(&jdi->base);
	jdi_panel_del(jdi);

	return 0;
}

static void jdi_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct jdi_panel *jdi = mipi_dsi_get_drvdata(dsi);

	jdi_panel_disable(&jdi->base);
}

static struct mipi_dsi_driver jdi_panel_driver = {
	.driver = {
		.name = "panel-jdi-lt070me05000",
		.of_match_table = jdi_of_match,
	},
	.probe = jdi_panel_probe,
	.remove = jdi_panel_remove,
	.shutdown = jdi_panel_shutdown,
};
module_mipi_dsi_driver(jdi_panel_driver);

MODULE_AUTHOR("Bhushan Shah <bshah@kde.org>");
MODULE_DESCRIPTION("JDI 1080P");
MODULE_LICENSE("GPL v2");

/**


                       P
                       A
 D                     Y
 T  L        W    D    L
 Y  A     A  A    L    O
 P  S  V  C  I    E    A
 E  T  C  K  T    N    D

29 01 00 00 00 00 02 b0 04  // mipi_dsi_generic_write(dsi, (u8[]{ 0xb0, 0x04 }), 2);
05 01 00 00 00 00 02 00 00  // mipi_dsi_dcs_write_buffer(dsi, (u8[]{0x00, 0x00}), 2);
05 01 00 00 00 00 02 00 00  // mipi_dsi_dcs_write_buffer(dsi, (u8[]{0x00, 0x00}), 2);
29 01 00 00 00 00 07 b3	    // mipi_dsi_generic_write(dsi, (u8[]{0xb3, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00}), 7);
14 00 00 00 00 00
29 01 00 00 00 00 03 b6     // mipi_dsi_generic_write(dsi, (u8[]{0xb6, 0x3a, 0xd3}), 3);
3a d3
29 01 00 00 00 00 23 c1	    // mipi_dsi_generic_write(dsi, (u8[]{0xc1,
84 60 50 00 00 00           //                                   0x84, 0x60, 0x50, 0x00, 0x00, 0x00,
00 00 00 0c 01 58           //                                   0x00, 0x00, 0x00, 0x0c, 0x01, 0x58,
73 ae 31 20 06 00           //                                   0x73, 0xae, 0x31, 0x20, 0x06, 0x00,
00 00 00 00 00 10           //                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
10 10 10 00 00 00           //                                   0x10, 0x10, 0x10, 0x00, 0x00, 0x00,
22 02 02 00                 //                                   0x22, 0x02, 0x02, 0x00}), 35);
29 01 00 00 00 00 08 c2     // mipi_dsi_generic_write(dsi, (u8[]{0xc2,
30 f7 80 0a 08 00           //                                   0x30, 0xf7, 0x80, 0x0a, 0x08, 0x00,
00                          //                                   0x00}), 8);
29 01 00 00 00 00 17 c4     // mipi_dsi_generic_write(dsi, (u8[]{0xc4,
70 00 00 00 00 04           //                                   0x70, 0x00, 0x00, 0x00, 0x00, 0x04,
00 00 00 11 06 00           //                                   0x00, 0x00, 0x00, 0x11, 0x06, 0x00,
00 00 00 00 04 00           //                                   0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
00 00 11 06                 //                                   0x00, 0x00, 0x11, 0x06}), 23);
29 01 00 00 00 00 29 c6     // mipi_dsi_generic_write(dsi, (u8[]{0xc6,
06 6d 06 6d 06 6d           //                                   0x06, 0x6d, 0x06, 0x6d, 0x06, 0x6d,
00 00 00 00 06 6d           //                                   0x00, 0x00, 0x00, 0x00, 0x06, 0x6d
06 6d 06 6d 15 19           //                                   0x06, 0x6d, 0x06, 0x6d, 0x15, 0x19,
07 00 01 06 6d 06           //                                   0x07, 0x00, 0x01, 0x06, 0x6d, 0x06,
6d 06 6d 00 00 00           //                                   0x6d, 0x06, 0x6d, 0x00, 0x00, 0x00,
00 06 6d 06 6d 06           //                                   0x00, 0x06, 0x6d, 0x06, 0x6d, 0x06,
6d 15 19 07                 //                                   0x6d, 0x15, 0x19, 0x07}), 41);
29 01 00 00 00 00 19 c7     // mipi_dsi_generic_write(dsi, (u8[]{0xc7,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}), 25);
29 01 00 00 00 00 19 c8     // mipi_dsi_generic_write(dsi, (u8[]{0xc8,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}), 25);
29 01 00 00 00 00 19 c9     // mipi_dsi_generic_write(dsi, (u8[]{0xc9,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70,
00 09 14 26 32 49           //                                   0x00, 0x09, 0x14, 0x26, 0x32, 0x49,
3b 52 5f 67 6b 70           //                                   0x3b, 0x52, 0x5f, 0x67, 0x6b, 0x70}), 25);
29 01 00 00 00 00 02 cc 09  // mipi_dsi_generic_write(dsi, (u8[]{0xcc, 0x09}), 2);
29 01 00 00 00 00 0f d0     // mipi_dsi_generic_write(dsi, (u8[]{0xd0,
00 00 19 18 99 99           //                                   0x00, 0x00, 0x19, 0x18, 0x99, 0x99,
19 01 89 00 55 19           //                                   0x19, 0x01, 0x89, 0x00, 0x55, 0x19,
99 01                       //                                   0x99, 0x01}), 15);
29 01 00 00 00 00 1b d3     // mipi_dsi_generic_write(dsi, (u8[]{0xd3,
1b 33 bb cc c4 33           //                                   0x1b, 0x33, 0xbb, 0xcc, 0xc4, 0x33,
33 33 00 01 00 a0           //                                   0x33, 0x33, 0x00, 0x01, 0x00, 0xa0,
d8 a0 0d 37 33 44           //                                   0xd8, 0xa0, 0x0d, 0x37, 0x33, 0x44,
22 70 02 37 03 3d           //                                   0x22, 0x70, 0x02, 0x37, 0x03, 0x3d,
bf 00                       //                                   0xbf, 0x00}), 27);
29 01 00 00 00 00 08 d5     // mipi_dsi_generic_write(dsi, (u8[]{0xd5,
06 00 00 01 4a 01           //                                   0x06, 0x00, 0x00, 0x01, 0x4a, 0x01,
4a                          //                                   0x4a}), 8);
// Changed it to 15
29 01 00 00 00 00 08        // mipi_dsi_generic_write(dsi, (u8[]{
d5 06 00 00 01 4a           //                                   0xd5, 0x06, 0x00, 0x00, 0x01, 0x4a,
01 4a                       //                                   0x01, 0x4a}), 8);
15 01 00 00 00 00 02 36 C0  // mipi_dsi_dcs_write_buffer(dsi, (u8[]{0x36, 0xc0}), 2);

05 01 00 00 00 00 02 29 00  // mipi_dsi_dcs_write_buffer(dsi, (u8[]{0x29, 0x00}), 2);
05 01 00 00 78 00 02 11 00];// mipi_dsi_dcs_write_buffer(dsi, (u8[]{0x11, 0x00}), 2);

PANEL_OFF

05 01 00 00 14 00 02 28 00
05 01 00 00 64 00 02 10 00


*/
