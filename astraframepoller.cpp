#include "astraframepoller.h"

AstraFramePoller::AstraFramePoller()
{

}

void AstraFramePoller::connectSensor(std::string device)
{
	if (connected)
		return;

    astra_streamset_open(device.c_str(), &sensor);

    astra_reader_create(sensor, &reader);

    switch (frameType)
    {
    case FrameType::DEPTH:
        connectDepthStream();
        break;
    case FrameType::COLOR:
        connectColorStream();
        break;
    default:
        break;
    }

	connected = true;
}

void AstraFramePoller::disconnectSensor()
{
	connected = false;
}

void AstraFramePoller::setFrameType(AstraFramePoller::FrameType type)
{
    frameType = type;
}

void AstraFramePoller::pollFrame(unsigned int timeout)
{
    astra_reader_frame_t frame;
    astra_status_t rc = astra_reader_open_frame(reader, timeout, &frame);

    if (rc != ASTRA_STATUS_SUCCESS)
        return;

    switch (frameType)
    {
    case FrameType::DEPTH:
        fillDepthFrame(frame);
        break;
    case FrameType::COLOR:
        fillColorFrame(frame);
        break;
    default:
        break;
    }

    astra_reader_close_frame(&frame);
}

const std::vector<astra_rgb_pixel_t> AstraFramePoller::getColorFrame()
{
    return colorBuffer;
}

const std::vector<int16_t> &AstraFramePoller::getDepthFrame()
{
    return depthBuffer;
}

unsigned int AstraFramePoller::getWidth()
{
	return width;
}

unsigned int AstraFramePoller::getHeight()
{
	return height;
}

void AstraFramePoller::connectColorStream()
{
    astra_reader_get_colorstream(reader, &colorStream);

    astra_stream_start(colorStream);
}

void AstraFramePoller::connectDepthStream()
{
    astra_reader_get_depthstream(reader, &depthStream);

    astra_stream_start(depthStream);
}

void AstraFramePoller::fillColorFrame(const astra_reader_frame_t& frame)
{
	astra_colorframe_t colorFrame;
	astra_rgb_pixel_t* colorFrame_rgb = colorBuffer.data();
    astra_image_metadata_t metadata;

    astra_frame_get_colorframe(frame, &colorFrame);
    astra_colorframe_get_metadata(colorFrame, &metadata);

    width = metadata.width;
    height = metadata.height;

    uint32_t length;
	
    astra_colorframe_get_data_rgb_ptr(colorFrame, &colorFrame_rgb, &length);
	
    colorBuffer.resize(length);
}

void AstraFramePoller::fillDepthFrame(const astra_reader_frame_t& frame)
{
	astra_depthframe_t depthFrame;
    astra_image_metadata_t metadata;

    astra_frame_get_depthframe(frame, &depthFrame);
    astra_depthframe_get_metadata(depthFrame, &metadata);

    width = metadata.width;
    height = metadata.height;

    uint32_t length;

    astra_depthframe_get_data_byte_length(depthFrame, &length);

    depthBuffer.resize(length);
    astra_depthframe_copy_data(depthFrame, depthBuffer.data());
}
