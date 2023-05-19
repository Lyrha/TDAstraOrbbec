#include "astraframelistener.h"

AstraFrameListener::AstraFrameListener()
{
}

void AstraFrameListener::connectSensor(const char* device)
{
	if (connected)
		return;
	
	streamSet = astra::StreamSet(device);

	reader = streamSet.create_reader();

	reader.stream<astra::PointStream>().start();

	auto depthStream = configure_depth(reader);
	depthStream.start();

	auto colorStream = configure_color(reader);
	colorStream.start();

	auto irStream = configure_ir(reader, false);

	setStreamType(DEPTH);

	reader.add_listener(*this);

	connected = true;
}

void AstraFrameListener::disconnectSensor()
{
	connected = false;
}

void AstraFrameListener::setStreamType(AstraFrameListener::StreamType type)
{
    streamType = type;
}

int AstraFrameListener::getStreamWidth()
{
	int width = 0;

	switch (streamType) {
	case DEPTH:
		width = depthStream.width;
		break;
	default:
		width = colorStream.width;
		break;
	}

	return width;
}

int AstraFrameListener::getStreamHeight()
{
	int height = 0;

	switch (streamType) {
	case DEPTH:
		height = depthStream.height;
		break;
	default:
		height = colorStream.height;
		break;
	}

	return height;
}

void AstraFrameListener::on_frame_ready(astra::StreamReader &reader, astra::Frame &frame)
{
    switch(streamType){
    case DEPTH:
		updateDepth(frame);
        break;
    case COLOR:
		updateColor(frame);
        break;
	case IR_16:
		updateIR_16(frame);
		break;
	case IR_RGB:
		updateIR_RGB(frame);
		break;
    default:
        break;
    }
}

astra::DepthStream AstraFrameListener::configure_depth(astra::StreamReader & reader)
{
	auto depthStream = reader.stream<astra::DepthStream>();

	auto oldMode = depthStream.mode();

	//We don't have to set the mode to start the stream, but if you want to here is how:
	astra::ImageStreamMode depthMode;

	depthMode.set_width(640);
	depthMode.set_height(480);
	depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
	depthMode.set_fps(30);

	depthStream.set_mode(depthMode);

	auto newMode = depthStream.mode();
	return depthStream;
}

astra::InfraredStream AstraFrameListener::configure_ir(astra::StreamReader & reader, bool useRGB)
{
	auto irStream = reader.stream<astra::InfraredStream>();

	auto oldMode = irStream.mode();

	astra::ImageStreamMode irMode;
	irMode.set_width(640);
	irMode.set_height(480);

	if (useRGB)
		irMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_RGB888);
	else
		irMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_GRAY16);

	irMode.set_fps(30);
	irStream.set_mode(irMode);

	auto newMode = irStream.mode();
	return irStream;
}

astra::ColorStream AstraFrameListener::configure_color(astra::StreamReader & reader)
{
	auto colorStream = reader.stream<astra::ColorStream>();

	auto oldMode = colorStream.mode();

	astra::ImageStreamMode colorMode;
	colorMode.set_width(640);
	colorMode.set_height(480);
	colorMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_RGB888);
	colorMode.set_fps(30);

	colorStream.set_mode(colorMode);

	auto newMode = colorStream.mode();
	return colorStream;
}

void AstraFrameListener::updateDepth(astra::Frame &frame)
{
	const astra::PointFrame pointFrame = frame.get<astra::PointFrame>();

	if (!pointFrame.is_valid()){
		clearStream(depthStream);
		return;
	}

	const int depthWidth = pointFrame.width();
	const int depthHeight = pointFrame.height();

	prepareStream(depthWidth, depthHeight, depthStream);

	visualizer.update(pointFrame);

	astra::RgbPixel* depth = visualizer.get_output();
	uint8_t* buffer = &depthStream.buffer[0];

	for (int i = 0; i < depthWidth * depthHeight; i++){
		const int rgbaOffset = i * 4;

		//Flip the image vertically
		int j = (depthWidth * depthHeight - 1) - i;

		buffer[rgbaOffset] = depth[j].r;
		buffer[rgbaOffset + 1] = depth[j].g;
		buffer[rgbaOffset + 2] = depth[j].b;
		buffer[rgbaOffset + 3] = 255;
	}
}

void AstraFrameListener::updateColor(astra::Frame &frame)
{
	const astra::ColorFrame colorFrame = frame.get<astra::ColorFrame>();
	if (!colorFrame.is_valid()) {
		clearStream(colorStream);
		return;
	}

	const int colorWidth = colorFrame.width();
	const int colorHeight = colorFrame.height();

	prepareStream(colorWidth, colorHeight, colorStream);

	const astra::RgbPixel* color = colorFrame.data();
	uint8_t* buffer = &colorStream.buffer[0];

	for(int i = 0; i < colorWidth * colorHeight; i++){
		const int rgbaOffset = i * 4;

		//Flip the image vertically
		int j = (colorWidth * colorHeight - 1) - i;

		buffer[rgbaOffset] = color[j].r;
		buffer[rgbaOffset + 1] = color[j].g;
		buffer[rgbaOffset + 2] = color[j].b;
		buffer[rgbaOffset + 3] = 255;
	}
}

void AstraFrameListener::updateIR_16(astra::Frame& frame)
{
	const astra::InfraredFrame16 irFrame = frame.get<astra::InfraredFrame16>();

	if (!irFrame.is_valid()){
		clearStream(colorStream);
		return;
	}

	const int irWidth = irFrame.width();
	const int irHeight = irFrame.height();

	prepareStream(irWidth, irHeight, colorStream);

	const uint16_t* ir_values = irFrame.data();
	uint8_t* buffer = &colorStream.buffer[0];
	for (int i = 0; i < irWidth * irHeight; i++){
		const int rgbaOffset = i * 4;
		const uint16_t value = ir_values[i];
		const uint8_t red = static_cast<uint8_t>(value >> 2);
		const uint8_t blue = 0x66 - red / 2;
		buffer[rgbaOffset] = red;
		buffer[rgbaOffset + 1] = 0;
		buffer[rgbaOffset + 2] = blue;
		buffer[rgbaOffset + 3] = 255;
	}
}

void AstraFrameListener::updateIR_RGB(astra::Frame& frame)
{
	const astra::InfraredFrameRgb irFrame = frame.get<astra::InfraredFrameRgb>();

	if (!irFrame.is_valid()){
		clearStream(colorStream);
		return;
	}

	int irWidth = irFrame.width();
	int irHeight = irFrame.height();

	prepareStream(irWidth, irHeight, colorStream);

	const astra::RgbPixel* irRGB = irFrame.data();
	uint8_t* buffer = &colorStream.buffer[0];
	for (int i = 0; i < irWidth * irHeight; i++){
		const int rgbaOffset = i * 4;
		buffer[rgbaOffset] = irRGB[i].r;
		buffer[rgbaOffset + 1] = irRGB[i].g;
		buffer[rgbaOffset + 2] = irRGB[i].b;
		buffer[rgbaOffset + 3] = 255;
	}
}

void AstraFrameListener::prepareStream(int width, int height, Stream& stream)
{
	if (stream.buffer == nullptr || width != stream.width || height != stream.height){
		stream.width = width;
		stream.height = height;

		const int byteLength = width * height * 4;

		stream.buffer = BufferPtr(new uint8_t[byteLength]);
		clearStream(stream);
	}
}

void AstraFrameListener::clearStream(Stream& stream)
{
	const int byteLength = stream.width * stream.height * 4;
	std::fill(&stream.buffer[0], &stream.buffer[0] + byteLength, 0);
}
