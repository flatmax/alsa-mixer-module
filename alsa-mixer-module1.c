#include <linux/module.h>
#include <sound/soc.h>
#include <sound/tlv.h>

/* SRC attenuation */
// static const DECLARE_TLV_DB_SCALE(vol_tlv, -12750, 50, 0);

#define USE_MIXER_VOLUME_LEVEL_MIN	-50
#define USE_MIXER_VOLUME_LEVEL_MAX	100
static int mixer_volume_level_min = USE_MIXER_VOLUME_LEVEL_MIN;
static int mixer_volume_level_max = USE_MIXER_VOLUME_LEVEL_MAX;

#define MIXER_ADDR_MASTER	0
#define MIXER_ADDR_LAST		0

struct snd_dummy {
	// struct snd_card *card;
	// const struct dummy_model *model;
	// struct snd_pcm *pcm;
	// struct snd_pcm_hardware pcm_hw;
	spinlock_t mixer_lock;
	int mixer_volume[MIXER_ADDR_LAST+1][2];
	int capture_source[MIXER_ADDR_LAST+1][2];
	// int iobox;
	// struct snd_kcontrol *cd_volume_ctl;
	// struct snd_kcontrol *cd_switch_ctl;
};

#define DUMMY_VOLUME(xname, xindex, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
  .access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ, \
  .name = xname, .index = xindex, \
  .info = snd_dummy_volume_info, \
  .get = snd_dummy_volume_get, .put = snd_dummy_volume_put, \
  .private_value = addr, \
  .tlv = { .p = db_scale_dummy } }

static int snd_dummy_volume_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = mixer_volume_level_min;
	uinfo->value.integer.max = mixer_volume_level_max;
	return 0;
}

static int snd_dummy_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_dummy *dummy = snd_soc_component_get_drvdata(component);
	int addr = kcontrol->private_value;

	spin_lock_irq(&dummy->mixer_lock);
	ucontrol->value.integer.value[0] = dummy->mixer_volume[addr][0];
	ucontrol->value.integer.value[1] = dummy->mixer_volume[addr][1];
	spin_unlock_irq(&dummy->mixer_lock);
	return 0;
}

static int snd_dummy_volume_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_dummy *dummy = snd_soc_component_get_drvdata(component);
	int change, addr = kcontrol->private_value;
	int left, right;

	left = ucontrol->value.integer.value[0];
	if (left < mixer_volume_level_min)
		left = mixer_volume_level_min;
	if (left > mixer_volume_level_max)
		left = mixer_volume_level_max;
	right = ucontrol->value.integer.value[1];
	if (right < mixer_volume_level_min)
		right = mixer_volume_level_min;
	if (right > mixer_volume_level_max)
		right = mixer_volume_level_max;
	spin_lock_irq(&dummy->mixer_lock);
	change = dummy->mixer_volume[addr][0] != left ||
	         dummy->mixer_volume[addr][1] != right;
	dummy->mixer_volume[addr][0] = left;
	dummy->mixer_volume[addr][1] = right;
	spin_unlock_irq(&dummy->mixer_lock);
	return change;
}

static const DECLARE_TLV_DB_SCALE(db_scale_dummy, -4500, 30, 0);

static const struct snd_kcontrol_new amm_controls[] = {
	// SOC_SINGLE_TLV("Volume", -1, 0, 255, 0, vol_tlv),
// 	static const struct snd_kcontrol_new snd_dummy_controls[] = {
DUMMY_VOLUME("Master Volume", 0, MIXER_ADDR_MASTER),

};

SND_SOC_DAILINK_DEFS(mixer,
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
//	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()));

static struct snd_soc_dai_link amm_dailink = {
	.name = "amm",
	.stream_name = "amm Mixer",
	.no_pcm = 1,
	.dpcm_playback = 1,
	// .codec_name = "wm8960-hifi",
	.dai_fmt = SND_SOC_DAIFMT_I2S	| SND_SOC_DAIFMT_NB_NF	| SND_SOC_DAIFMT_CBS_CFS,
	SND_SOC_DAILINK_REG(mixer),
};

static struct snd_soc_card amm_card = {
	.name = "amm dummy",
	.owner = THIS_MODULE,
	.dai_link = &amm_dailink,
	.num_links = 1,

	.controls = amm_controls,
	.num_controls = ARRAY_SIZE(amm_controls),

	// .dapm_widgets = atmel_asoc_wm8904_dapm_widgets,
	// .num_dapm_widgets = ARRAY_SIZE(atmel_asoc_wm8904_dapm_widgets),
	.fully_routed = true,
};

static int amm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct snd_soc_card *card = &amm_card;
	struct snd_soc_dai_link *codec_link;
	int ret;
	dev_info(dev, "%s : enter\n", __func__);
	card->dev = dev;

	struct snd_dummy *dummy;
	dummy = devm_kzalloc(dev, sizeof(*dummy), GFP_KERNEL);
	if (!dummy)
		return -ENOMEM;
	spin_lock_init(&dummy->mixer_lock);
	dev_set_drvdata(dev, dummy);

	// codec = of_get_child_by_name(dev->of_node, "codec");
	codec_link = &card->dai_link[0];


	ret = snd_soc_register_card(card);
	if (ret)
		dev_err(dev, "snd_soc_register_card failed\n");
	else
		dev_info(dev, "alsa-mixer-module probe ok %d\n", ret);
	return ret;
  // ret = devm_snd_soc_register_component(dev, &amm_driver,
	// 		amm_dai, ARRAY_SIZE(amm_dai));
	// if (ret == 0)
	// 	dev_info(dev, "alsa-mixer-module probe ok %d\n", ret);
  // return ret;
}

#define SND_MIXER_DRIVER	"snd_mixer_module"

static struct platform_driver amm_codec_driver = {
	.driver = {
			.name = SND_MIXER_DRIVER,
	},
	.probe = amm_probe,
};

static struct platform_device *amm_device;

static int __init amm_init(void)
{
	int err;

	err = platform_driver_register(&amm_codec_driver);
	if (err < 0){
		pr_err("%s: dmic-codec device registration failed\n", __func__);
		return err;
	}

	amm_device = platform_device_register_simple(SND_MIXER_DRIVER, 0, NULL, 0);
	if (IS_ERR(amm_device)){
		pr_err("%s: dmic-codec device registration failed\n", __func__);
		platform_driver_unregister(&amm_codec_driver);
		return PTR_ERR(amm_device);
	}

	return 0;
}

static void __exit amm_exit(void)
{
	platform_device_unregister(amm_device);
	platform_driver_unregister(&amm_codec_driver);
}

module_init(amm_init)
module_exit(amm_exit)

// module_platform_driver(amm_codec_driver);

MODULE_DESCRIPTION("ASoC mixer driver");
MODULE_AUTHOR("Matt Flax <flatmax>");
MODULE_LICENSE("GPL");
