#include "LitDepthVisualizer.h"

void LitDepthVisualizer::box_blur(const astra::Vector3f* in, astra::Vector3f * out, const size_t width, const size_t height, const int blurRadius)
{
	const size_t maxY = height - blurRadius;
	const size_t maxX = width - blurRadius;

	out += blurRadius * width;

	for (size_t y = blurRadius; y < maxY; ++y){
		out += blurRadius;
		for (size_t x = blurRadius; x < maxX; ++x, ++out){
			astra::Vector3f normAvg;

			size_t index = x - blurRadius + (y - blurRadius) * width;
			const astra::Vector3f* in_row = in + index;

			for (int dy = -blurRadius; dy <= blurRadius; ++dy, in_row += width)
			{
				const astra::Vector3f* in_kernel = in_row;
				for (int dx = -blurRadius; dx <= blurRadius; ++dx, ++in_kernel)
				{
					normAvg += *in_kernel;
				}
			}

			*out = astra::Vector3f::normalize(normAvg);
		}
		out += blurRadius;
	}
}

void LitDepthVisualizer::box_blur_fast(const astra::Vector3f* in, astra::Vector3f * out, const size_t width, const size_t height)
{
	const size_t maxY = height;
	const size_t maxX = width;

	const astra::Vector3f* in_row = in + width;
	astra::Vector3f* out_row = out;

	memset(out, 0, width * height * sizeof(astra::Vector3f));

	for (size_t y = 1; y < maxY; ++y, in_row += width, out_row += width){
		const astra::Vector3f* in_left = in_row - 1;
		const astra::Vector3f* in_mid = in_row + 1;

		astra::Vector3f* out_up = out_row;
		astra::Vector3f* out_mid = out_row + width;

		astra::Vector3f xKernelTotal = *in_left + *in_row;

		for (size_t x = 1; x < maxX; ++x){
			xKernelTotal += *in_mid;

			*out_up += xKernelTotal;
			*out_mid += xKernelTotal;

			xKernelTotal -= *in_left;

			++in_left;
			++in_mid;
			++out_up;
			++out_mid;
		}
	}
}

LitDepthVisualizer::LitDepthVisualizer() :
	lightVector(0.44022f, -0.17609f, 0.88045f)
{
	lightColor = { 210, 210, 210 };
	ambientColor = { 30, 30, 30 };
}

void LitDepthVisualizer::set_light_color(const astra::RgbPixel& color)
{
	lightColor = color;
}

void LitDepthVisualizer::set_light_direction(const astra::Vector3f& direction)
{
	lightVector = direction;
}

void LitDepthVisualizer::set_ambient_color(const astra::RgbPixel& color)
{
	ambientColor = color;
}

void LitDepthVisualizer::set_blur_radius(unsigned int radius)
{
	blurRadius = radius;
}

void LitDepthVisualizer::update(const astra::PointFrame& pointFrame)
{
	calculate_normals(pointFrame);

	const size_t width = pointFrame.width();
	const size_t height = pointFrame.height();

	prepare_buffer(width, height);

	const astra::Vector3f* pointData = pointFrame.data();

	astra_rgb_pixel_t* texturePtr = outputBuffer.get();

	const astra::Vector3f* normMap = blurNormalMap.get();
	const bool useNormalMap = normMap != nullptr;

	for (unsigned y = 0; y < height; ++y)
	{
		for (unsigned x = 0; x < width; ++x, ++pointData, ++normMap, ++texturePtr)
		{
			float depth = (*pointData).z;

			astra::Vector3f norm(1, 0, 0);

			if (useNormalMap)
			{
				norm = astra::Vector3f::normalize(*normMap);
			}

			if (depth != 0)
			{
				const float fadeFactor = static_cast<float>(
					1.0f - 0.6f * std::max(0.0f, std::min(1.0f, ((depth - 400.0f) / 3200.0f))));

				const float diffuseFactor = norm.dot(lightVector);

				astra_rgb_pixel_t diffuseColor;

				if (diffuseFactor > 0)
				{
					//only add diffuse when mesh is facing the light
					diffuseColor.r = static_cast<uint8_t>(lightColor.r * diffuseFactor);
					diffuseColor.g = static_cast<uint8_t>(lightColor.g * diffuseFactor);
					diffuseColor.b = static_cast<uint8_t>(lightColor.b * diffuseFactor);
				}
				else
				{
					diffuseColor.r = 0;
					diffuseColor.g = 0;
					diffuseColor.b = 0;
				}

				texturePtr->r = std::max(0, std::min(255, (int)(fadeFactor*(ambientColor.r + diffuseColor.r))));
				texturePtr->g = std::max(0, std::min(255, (int)(fadeFactor*(ambientColor.g + diffuseColor.g))));
				texturePtr->b = std::max(0, std::min(255, (int)(fadeFactor*(ambientColor.b + diffuseColor.b))));
			}
			else
			{
				texturePtr->r = 0;
				texturePtr->g = 0;
				texturePtr->b = 0;
			}
		}
	}
}

astra::RgbPixel* LitDepthVisualizer::get_output() const { 
	return outputBuffer.get(); 
}

void LitDepthVisualizer::prepare_buffer(size_t width, size_t height)
{
	if (outputBuffer == nullptr || width != outputWidth || height != outputHeight)
	{
		outputWidth = width;
		outputHeight = height;
		outputBuffer = astra::make_unique<astra::RgbPixel[]>(outputWidth * outputHeight);
	}

	std::fill(outputBuffer.get(), outputBuffer.get() + outputWidth * outputHeight, astra::RgbPixel(0, 0, 0));
}

void LitDepthVisualizer::calculate_normals(const astra::PointFrame& pointFrame)
{
	const astra::Vector3f* positionMap = pointFrame.data();

	const int width = pointFrame.width();
	const int height = pointFrame.height();

	const size_t numPixels = width * height;

	if (normalMap == nullptr || normalMapLength != numPixels)
	{
		normalMap = astra::make_unique<astra::Vector3f[]>(numPixels);
		blurNormalMap = astra::make_unique<astra::Vector3f[]>(numPixels);

		std::fill(blurNormalMap.get(), blurNormalMap.get() + numPixels, astra::Vector3f::zero());

		normalMapLength = numPixels;
	}

	astra::Vector3f* normMap = normalMap.get();

	//top row
	for (int x = 0; x < width; ++x)
	{
		*normMap = astra::Vector3f::zero();
		++normMap;
	}

	const int maxY = height - 1;
	const int maxX = width - 1;

	for (int y = 1; y < maxY; ++y)
	{
		//first pixel at start of row
		*normMap = astra::Vector3f::zero();
		++normMap;

		//Initialize pointer arithmetic for the x=0 position
		const astra::Vector3f* p_point = positionMap + y * width;
		const astra::Vector3f* p_pointLeft = p_point - 1;
		const astra::Vector3f* p_pointRight = p_point + 1;
		const astra::Vector3f* p_pointUp = p_point - width;
		const astra::Vector3f* p_pointDown = p_point + width;

		for (int x = 1; x < maxX; ++x)
		{
			++p_pointLeft;
			++p_point;
			++p_pointRight;
			++p_pointUp;
			++p_pointDown;

			const astra::Vector3f& point = *p_point;
			const astra::Vector3f& pointLeft = *p_pointLeft;
			const astra::Vector3f& pointRight = *p_pointRight;
			const astra::Vector3f& pointUp = *p_pointUp;
			const astra::Vector3f& pointDown = *p_pointDown;

			if (point.z != 0 &&
				pointRight.z != 0 &&
				pointDown.z != 0 &&
				pointLeft.z != 0 &&
				pointUp.z != 0
				)
			{
				astra::Vector3f vr = pointRight - point;
				astra::Vector3f vd = pointDown - point;
				astra::Vector3f vl = pointLeft - point;
				astra::Vector3f vu = pointUp - point;

				astra::Vector3f normAvg = vd.cross(vr);
				normAvg += vl.cross(vd);
				normAvg += vu.cross(vl);
				normAvg += vr.cross(vu);

				*normMap = astra::Vector3f::normalize(normAvg);

			}
			else
			{
				*normMap = astra::Vector3f::zero();
			}

			++normMap;
		}

		//last pixel at end of row
		*normMap = astra::Vector3f::zero();
		++normMap;
	}

	//bottom row
	for (int x = 0; x < width; ++x)
	{
		*normMap = astra::Vector3f::zero();
		++normMap;
	}

	//box_blur(normalMap_.get(), blurNormalMap_.get(), width, height, blurRadius_);
	LitDepthVisualizer::box_blur_fast(normalMap.get(), blurNormalMap.get(), width, height);
}