#include <obs-module.h>
#include <util/platform.h>
#include "codec_api.h"
#include "codec_def.h"

OBS_DECLARE_MODULE();

struct enc_openh264 {
    obs_encoder_t *obs_encoder;
    ISVCEncoder *openh264_encoder;
    SEncParamBase param;
    int width;
    int height;
	os_performance_token_t *performance_token;
};

#define do_log(level, format, ...) \
	blog(level, "[openh264 encoder] " format, ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

static const char *openh264_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "openh264";
}

static void *openh264_create(obs_data_t *settings, obs_encoder_t *encoder)
{
    int rv = 0;
    struct enc_openh264 *enc_oh264 = bzalloc(sizeof(struct enc_openh264));
    enc_oh264->obs_encoder = encoder;
    rv = WelsCreateSVCEncoder(&enc_oh264->openh264_encoder);
    if ( 0 == rv && enc_oh264->openh264_encoder != NULL)
        info("Created openh264 encoder");   
    else
        warn("Could not create encoder");

    memset(&enc_oh264->param, 0, sizeof(SEncParamBase));
    enc_oh264->param.iUsageType = SCREEN_CONTENT_REAL_TIME;
    enc_oh264->param.fMaxFrameRate = 30;
    enc_oh264->param.iPicWidth = (int)obs_encoder_get_width(encoder);
    enc_oh264->param.iPicHeight = (int)obs_encoder_get_height(encoder);
    enc_oh264->width = (int)obs_encoder_get_width(encoder);
    enc_oh264->height = (int)obs_encoder_get_height(encoder);
    //enc_oh264->param.iTargetBitrate = (int)obs_data_get_int(settings, "bitrate");
    enc_oh264->param.iTargetBitrate = 1000;
    //info("Width: %d - Height: %d", enc_oh264->param.iPicWidth, enc_oh264->param.iPicHeight);
    //enc_oh264->openh264_encoder->
    int videoFormat = videoFormatI420;
    int bitrate = 1000;
    (*enc_oh264->openh264_encoder)->SetOption(enc_oh264->openh264_encoder,ENCODER_OPTION_DATAFORMAT, &videoFormat);
    (*enc_oh264->openh264_encoder)->SetOption(enc_oh264->openh264_encoder,ENCODER_OPTION_MAX_BITRATE, &bitrate);
    (*enc_oh264->openh264_encoder)->Initialize(enc_oh264->openh264_encoder, &enc_oh264->param);
    info("Open h264 Encoder created");
	enc_oh264->performance_token =
		os_request_high_performance("openh264 encoding");

    return enc_oh264;
}
static void openh264_destroy(void *data)
{
    struct enc_openh264 *enc_oh264 = data;
    if(enc_oh264){
		os_end_high_performance(enc_oh264->performance_token);
        (*enc_oh264->openh264_encoder)->Uninitialize(enc_oh264->openh264_encoder);
        WelsDestroySVCEncoder(enc_oh264->openh264_encoder);
        bfree(enc_oh264);
    }
}

static void openh264_video_info(void *data, struct video_scale_info *info)
{

}

static bool openh264_sei(void *data, uint8_t **sei, size_t *size)
{
	return true;
}
static bool openh264_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	return true;
}
static bool openh264_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
    //info("Starting frame encode");
    int rv;
    SFrameBSInfo info;
    SSourcePicture pic;
    struct enc_openh264 *enc_oh264 = data;
    int frameSize = enc_oh264->width * enc_oh264->height * 3 / 2;

    if (!frame || !packet || !received_packet)
		return false;

    memset(&info, 0, sizeof (SFrameBSInfo));
    memset(&pic, 0, sizeof (SSourcePicture));

    pic.iPicWidth = enc_oh264->width;
    pic.iPicHeight = enc_oh264->height;
    pic.iColorFormat = videoFormatI420;
    pic.iStride[0] = frame->linesize[0];
    pic.iStride[1] = pic.iStride[2] = frame->linesize[1];
    pic.pData[0] = frame->data[0]; // pointer to frame data?
    pic.pData[1] = frame->data[1];
    pic.pData[2] = frame->data[2];
    rv = (*enc_oh264->openh264_encoder)->EncodeFrame(enc_oh264->openh264_encoder, &pic, &info);
    if(rv != cmResultSuccess)
        warn("Couldn't encode the frame");
    //else
        //info("Encoded a frame with open264");
    packet->data = info.sLayerInfo[info.iLayerNum].pBsBuf;
    packet->size = info.iFrameSizeInBytes;
    packet->pts = info.uiTimeStamp;
    packet->dts = info.uiTimeStamp;
    packet->type = OBS_ENCODER_VIDEO;
    packet->keyframe = 0;

    *received_packet = true;

    return true;
}
static bool openh264_update(void *data, obs_data_t *settings)
{
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
    return true;
}
static obs_properties_t *openh264_props(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_int(props, "bitrate", "bitrate", 50, 10000000, 1);
    return props;
}
static void openh264_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
}
struct obs_encoder_info openh264_encoder = {
	.id             = "openh264",
	.type           = OBS_ENCODER_VIDEO,
	.codec          = "h264",
	.get_name       = openh264_getname,
	.create         = openh264_create,
	.destroy        = openh264_destroy,
	.encode         = openh264_encode,
	.update         = openh264_update,
    .get_properties = openh264_props,
	.get_defaults   = openh264_defaults,
	.get_extra_data = openh264_extra_data,
	.get_sei_data   = openh264_sei,
	.get_video_info = openh264_video_info
};


bool obs_module_load(void)
{
	obs_register_encoder(&openh264_encoder);
	return true;
}