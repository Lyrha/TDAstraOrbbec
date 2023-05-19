#ifndef ASTRAFRAMELISTENER_H
#define ASTRAFRAMELISTENER_H

#include <astra/astra.hpp>
#include "LitDepthVisualizer.h"

#include <cstdio>
#include <chrono>
#include <iostream>
#include <iomanip>

class AstraFrameListener : public astra::FrameListener
{  
public:
	using BufferPtr = std::unique_ptr<uint8_t[]>;

    typedef enum StreamType{
        DEPTH,
		COLOR,
		IR_16,
		IR_RGB,
    }
    StreamType;

	typedef struct Stream {
		int width{ 0 };
		int height{ 0 };
		BufferPtr buffer;
	}
	Stream;

	AstraFrameListener();

	void connectSensor(const char* device);
	void disconnectSensor();

    void setStreamType(AstraFrameListener::StreamType type);

	int getStreamWidth();
	int getStreamHeight();

    virtual void on_frame_ready(astra::StreamReader& reader,
                                astra::Frame& frame) override;
protected:
	astra::DepthStream configure_depth(astra::StreamReader& reader);
	astra::InfraredStream configure_ir(astra::StreamReader& reader, bool useRGB);
	astra::ColorStream configure_color(astra::StreamReader& reader);

    virtual void updateDepth(astra::Frame& frame);
	virtual void updateColor(astra::Frame& frame);
	virtual void updateIR_16(astra::Frame& frame);
	virtual void updateIR_RGB(astra::Frame& frame);

	virtual void prepareStream(int width, int height, Stream& stream);
	virtual void clearStream(Stream& stream);

    StreamType streamType{COLOR};

	std::string name{ "device/default" };

	bool connected{ false };

	astra::StreamSet streamSet;
	astra::StreamReader reader;

	Stream depthStream;
	Stream colorStream;

	LitDepthVisualizer visualizer;
};

#endif // ASTRAFRAMELISTENER_H
