#ifndef ASTRAFRAMEPOLLER_H
#define ASTRAFRAMEPOLLER_H

#include <vector>

#include <astra/astra.hpp>

class AstraFramePoller
{
public:
    typedef enum FrameType{
        COLOR,
        DEPTH,
    }
    FrameType;

    AstraFramePoller();

    void connectSensor(std::string device);
    void disconnectSensor();

    void setFrameType(AstraFramePoller::FrameType type);

    virtual void pollFrame(unsigned int timeout);

    const std::vector<astra_rgb_pixel_t> getColorFrame();
    const std::vector<int16_t>& getDepthFrame();

	unsigned int getWidth();
	unsigned int getHeight();

protected:
    void connectColorStream();
    void connectDepthStream();

    void fillColorFrame(const astra_reader_frame_t& frame);
    void fillDepthFrame(const astra_reader_frame_t& frame);

	std::string name{"device/default"};

    FrameType frameType{DEPTH};

	bool connected{ false };

    std::vector<astra_rgb_pixel_t> colorBuffer;
    std::vector<int16_t> depthBuffer;

    unsigned int width{0};
    unsigned int height{0};

	astra_depthstream_t depthStream;
	astra_colorstream_t colorStream;

    astra_streamsetconnection_t sensor;
    astra_reader_t reader;
};

#endif // ASTRAFRAMEPOLLER_H
