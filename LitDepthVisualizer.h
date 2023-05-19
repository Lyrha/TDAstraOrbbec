// This file is part of the Orbbec Astra SDK [https://orbbec3d.com]
// Copyright (c) 2015-2017 Orbbec 3D
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Be excellent to each other.
#ifndef LITDEPTHVISUALIZER_H
#define LITDEPTHVISUALIZER_H

#include <astra/astra.hpp>
#include <cstring>
#include <algorithm>

class LitDepthVisualizer
{
public:

	static void box_blur(
		const astra::Vector3f* in,
		astra::Vector3f* out,
		const size_t width,
		const size_t height,
		const int blurRadius = 1);

	static void box_blur_fast(
		const astra::Vector3f* in, 
		astra::Vector3f* out,
		const size_t width,
		const size_t height);

	LitDepthVisualizer();

	void set_light_color(const astra::RgbPixel& color);
	void set_light_direction(const astra::Vector3f& direction);
	void set_ambient_color(const astra::RgbPixel& color);
	void set_blur_radius(unsigned int radius);

	void update(const astra::PointFrame& pointFrame);

	astra::RgbPixel* get_output() const;

private:
	using VectorMapPtr = std::unique_ptr<astra::Vector3f[]>;
	VectorMapPtr normalMap{ nullptr };
	VectorMapPtr blurNormalMap{ nullptr };
	size_t normalMapLength{ 0 };

	astra::Vector3f lightVector;
	unsigned int blurRadius{ 1 };
	astra::RgbPixel lightColor;
	astra::RgbPixel ambientColor;

	size_t outputWidth;
	size_t outputHeight;

	using BufferPtr = std::unique_ptr<astra::RgbPixel[]>;
	BufferPtr outputBuffer{ nullptr };

	void prepare_buffer(size_t width, size_t height);
	void calculate_normals(const astra::PointFrame& pointFrame);
};

#endif /* LITDEPTHVISUALIZER_H */
